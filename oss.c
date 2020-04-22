#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>
#include <fcntl.h>

#include "lock.h"
#include "child.h"
#include "oss.h"


static queue_t rq;

static unsigned int child_count = 0;
static unsigned int child_max = 100;
static unsigned int child_done = 0;  //child_done children

static unsigned int child_releases = 0;
static unsigned int child_requests = 0;

static unsigned int allow_count = 0;
static unsigned int delay_count = 0;
static unsigned int deny_count = 0;

static unsigned int deadlocks_count = 0;
static unsigned int kill_count = 0;

static unsigned int line_count = 0;

static struct DATA *data = NULL;
static int qid = -1, shmid = -1, semid = -1;
static const char * logfile = "output.txt";

static int is_signaled = 0; /* 1 if we were signaled */
static int is_verbose = 0;  /* 1 if verbose */

static int attach(){

 	key_t k = ftok("oss.c", 1);

 	shmid = shmget(k, sizeof(struct DATA), IPC_CREAT | S_IRWXU);
 	if (shmid == -1) {
 		perror("shmget");
 		return -1;
 	}

 	data = (struct DATA*) shmat(shmid, (void *)0, 0);
 	if (data == (void *)-1) {
 		perror("shmat");
 		shmctl(shmid, IPC_RMID, NULL);
 		return -1;
 	}

	k = ftok("oss.c", 2);
	qid = msgget(k, IPC_CREAT | S_IRWXU);
	if(qid == -1){
		perror("msgget");
		return -1;
	}

  k = ftok("oss.c", 3);
  semid = semget(k, 1, IPC_CREAT | S_IRWXU);
  if(semid == -1){
    perror("semget");
    return -1;
  }

  union semun un;
  un.val = 1;
  if(semctl(semid, 0, SETVAL, un) == -1){
    perror("semctl");
    return -1;
  }

	return 0;
}

static int detach(){

  shmdt(data);
  shmctl(shmid, IPC_RMID,NULL);
  semctl(semid, 0, IPC_RMID, NULL);
	msgctl(qid, IPC_RMID, NULL);

  q_dealloc(&rq);

  return 0;
}

static void list_messages(){

  printf("[%li:%li] Queued requests\n", data->clock.sec, data->clock.nsec);

  const int n = q_len(&rq);

  int i;
  for(i=0; i < n; i++){
    struct qitem * msg = q_at(&rq, i);
    printf("P%d: R%d=%d\n", msg->id, msg->rid, msg->val);
  }
	line_count += n;
}

static void list_resources(){

  printf("[%li:%li] Current resources\n", data->clock.sec, data->clock.nsec);
	print_rids();
  print_r("TOT ", data->res.max);
  print_r("OSS ", data->res.cur);
	line_count += 4;

	//show what the users have
  int i;
  char buf[10];
	for(i=0; i < CHILDREN; i++){
    struct child * child = &data->children[i];
		if(child->state != CHILD_DONE){
      snprintf(buf, 10,"P%02d ", child->cid);
  	  print_r(buf, child->res.cur);
			line_count++;
    }
	}
}

static int finished_check(const int done[CHILDREN], const int n){
  int i;
	for(i=0; i < n; i++){
    if(!done[i]){
      return 1;
    }
  }
  return 0;
}

static int find_victim(const int done[CHILDREN], const int n){

  int i;

  //id and amount of resources for the process to be killed
  int most_res = 0, most_id=0;

	for(i=0; i < n; i++){
		if(!done[i]){

      struct qitem * msg = q_at(&rq, i);
      struct child * child = &data->children[msg->id];

      //count the number of resources user holds, and compare with most
      const int res_count = count_r(child->res.cur);
      if(res_count > most_res){
        most_id = i;
        most_res = res_count;
      }
    }
  }

  if(is_verbose){
    printf("Users in deadlock: ");
    for(i=0; i < n; i++){
  		if(!done[i]){
        struct qitem * msg = q_at(&rq, i);
        printf("P%d ", msg->id);
      }
    }
    printf("\n");
  }

	return most_id;
}

static int deadlock_check(void){

	int cur[RESOURCES];
  int done[CHILDREN];

  memset(done, 0, sizeof(int)*CHILDREN);
  memcpy(cur, data->res.cur, sizeof(int)*RESOURCES);

	const int n = q_len(&rq);

  int i=0;
	while(i != n){

		for(i=0; i < n; i++){

			if(done[i] == 1)
				continue;

      struct qitem * msg = q_at(&rq, i);
			struct child * child = &data->children[msg->id];

      if(child->state == CHILD_DONE){
        done[i] = 1;
        continue;
      }

      if(	(msg->val < 0 ) || (msg->val <= cur[msg->rid])){

				done[i] = 1;
        add_r(cur, child->res.cur);

				break;
			}
		}
	}

  int victim = -1;
  if(finished_check(done, n) == 1){
    victim = find_victim(done, n);
  }
	return victim;
}

static void clear_victim(const int victim_id){
	int i;

  struct qitem * msg = q_at(&rq, victim_id);
	struct child * child = &data->children[msg->id];

	if(is_verbose){
    printf("Resolving deadlock by removing user P%d with most resources ...\n", msg->id);
    printf("Killing victim P%i deadlocked for R%d:%d\n", child->cid, msg->rid, msg->val);
		line_count += 2;

    printf("Released resources:");

    for(i=0; i < RESOURCES; i++){
      if(child->res.cur[i] > 0){
  		    printf("R%i:%d ", i, child->res.cur[i]);
      }
    }
  	printf("\n");
		line_count++;

    list_messages();
    list_resources();
  }

	kill_count++;
  deny_count++;

	//remove form queue
	q_drop(&rq, victim_id);

	/* tell victim request is denied */
  struct msgbuf buf;
	buf.mtype = child->pid;
	buf.msg.result = DENY;
	msgsnd(qid, (void*)&buf, MSG_LEN, 0);

}

static void escape_deadlock(){
  if(is_verbose){
    line_count++;
    printf("[%li:%li] Master running deadlock detection\n", data->clock.sec, data->clock.nsec);
  }

  int victim = -1, nvictims=0;
  while((victim = deadlock_check()) >= 0){
		clear_victim(victim);
    nvictims++;
  }

  if(nvictims){
		deadlocks_count++;

		if(is_verbose){
    	printf("System is no longer in deadlock, after removing %d users\n", nvictims);
			line_count++;
		}
	}
}

static void return_decision(struct child * child, struct qitem* msg){
  child_releases++;
  msg->result = ALLOW;

  child->res.cur[msg->rid] -= msg->val;
  data->res.cur[msg->rid]  += msg->val;

  printf("[%li:%li] Master has acknowledged Process P%d releasing R%d=%d\n",
    data->clock.sec, data->clock.nsec, child->cid, msg->rid, msg->val);
}

static void deny_decision(struct child * child, struct qitem* msg){
  msg->result = DENY;
  deny_count++;
  printf("[%li:%li] Master denied P%d invalid request R%d=%d\n",
        data->clock.sec, data->clock.nsec, child->cid, msg->rid, msg->val);
}

static void take_receive_decision(struct child * child, struct qitem* msg){
  enum oss_decision old_result = msg->result;

  if(data->res.cur[msg->rid] >= msg->val){
    allow_count++;

    child->res.cur[msg->rid] += msg->val;
    data->res.cur[msg->rid] -= msg->val;

    msg->result = ALLOW;

    if(old_result == DELAY){
      printf("[%li:%li] Master unblocking P%d and granting it R%d=%d\n",
        data->clock.sec, data->clock.nsec, child->cid, msg->rid, msg->val);

    }else if(old_result == DENY){ //new requests have DENY as a result

       printf("[%li:%li] Master granting P%d request R%d=%d\n",
         data->clock.sec, data->clock.nsec, child->cid, msg->rid, msg->val);
    }

  }else if(msg->result != DELAY){
    msg->result = DELAY;
    delay_count++;
    printf("[%li:%li] Master blocking P%d for requesting R%d=%d\n",
      data->clock.sec, data->clock.nsec, child->cid, msg->rid, msg->val);
  }
}

static int dispatch_q(){
	int i, count=0;
  struct oss_time t;
	struct msgbuf buf;

  t.sec = 0;

	const int n = q_len(&rq);

  printf("[%li:%li] Master dispatching queue of size %d\n", data->clock.sec, data->clock.nsec, n);

	for(i=0; i < n; i++){

		struct qitem * msg = q_pop(&rq);	/* get index of process who has a request */
		struct child * child = &data->children[msg->id];

		if(child->state != CHILD_DONE){	//process is running

      if((msg->decision == RETURN) && (msg->rid == -1)){
    		printf("[%li:%li] Master has detected P%d quits\n", data->clock.sec, data->clock.nsec, child->cid);

        //return all of the user resources
        add_r(data->res.cur, child->res.cur);
        child_reset(data->children, msg->id);
  			child_done++;

        continue; //don't send reply

      }else if((msg->result != DELAY) && (msg->decision == RECEIVE)){

				if(is_verbose){
          child_requests++;
					line_count++;
					printf("[%li:%li] Master has detected P%d requesting R%d=%d\n",
						data->clock.sec, data->clock.nsec, child->cid, msg->rid, msg->val);
				}
			}

      if(msg->decision == RETURN){
        return_decision(child, msg);
    	}else if(msg->decision == RECEIVE){
        take_receive_decision(child, msg);
    	}else{
        deny_decision(child, msg);
    	}

			if(msg->result == DELAY){	//if request was accepted or denied
        //return message to queue, for later dispatching
				q_push(&rq, msg);
      }else{
				count++;

				//send message to user to unblock
				buf.mtype = child->pid;
        buf.msg = *msg;
				if(msgsnd(qid, (void*)&buf, MSG_LEN, 0) == -1){
					break;
				}
			}

      const int n = (child_requests + delay_count + deny_count);
    	if((is_verbose == 1) && ((n % 20) == 0))
    		list_resources();
		}

    //add request processing to clock
    t.nsec = rand() % 100;
    oss_time_update(&data->clock, &t);
	}

	return count;	//return number of dispatched procs
}

static int collect_requests(){

  int n=0, nretry = 20; //return after two empty queues
  struct msgbuf buf;

  memset(&buf, 0, sizeof(struct msgbuf));

	while(1){
    if(msgrcv(qid, (void*)&buf, MSG_LEN, getpid(), IPC_NOWAIT) == -1){
			if(errno == ENOMSG){
        if(--nretry >= 0){  //if we can retry
          //usleep(20);
          continue;
        }else{
				  break;	   //stop
        }
			}else{
        perror("msgrcv");
				return -1;
			}
		}

		if(q_push(&rq, &buf.msg) == -1)
			break;	//stop
    n++;
	}

	if(is_verbose && (n > 0)){
		printf("[%li:%li] Master received %d new requests\n", data->clock.sec, data->clock.nsec, n);
	}

  return 0;
}

static void check_lines(){
  if(line_count >= 100000){
    printf("[%li:%li] OSS: Too many lines ...\n", data->clock.sec, data->clock.nsec);
    stdout = freopen("/dev/null", "w", stdout);
  }
}

static void alarm_children(){
  int i;
  struct msgbuf mb;

  bzero(&mb, sizeof(mb));
	mb.msg.decision = DENY;

  for(i=0; i < CHILDREN; i++){
    if(data->children[i].pid > 0){
      mb.mtype = data->children[i].pid;
      if(msgsnd(qid, (void*)&mb, MSG_LEN, 0) == -1){
        perror("msgsnd");
      }
    }
  }
}

static void output_stat(){

	printf("Runtime: %li:%li\n", data->clock.sec, data->clock.nsec);

  printf("Child count: %d/%d\n", child_count, child_done);

	printf("RECEIVE count: %u\n", child_requests);
  printf("RETURN count: %u\n", child_releases);

  printf("ALLOW count: %u\n", allow_count);
  printf("DELAY count: %u\n", delay_count);
  printf("DENY count: %u\n", deny_count);

  printf("Deadlocks: %u\n", deadlocks_count);
  printf("Kills: %u\n", kill_count);
  printf("Kills Average: %f\n", (float) kill_count / deadlocks_count);
}


static int user_fork(struct oss_time * fork_time){

  if(child_count >= child_max){  //if we can fork more
    return 0;
  }

  //if its fork time
  if(oss_time_compare(&data->clock, fork_time) == -1){
    return 0;
  }

  //next fork time
  fork_time->sec = data->clock.sec + 1;
  fork_time->nsec = 0;

  const int ci = child_fork(data->children, child_count);
  if(ci < 0){
    return 0;
  }else if(ci >= 0){
    printf("[%li:%li] OSS: Generating process with PID %u\n", data->clock.sec, data->clock.nsec, data->children[ci].cid);
  }
  child_count++;

  return 1;
}

static int oss_init(){

  srand(getpid());


  stdout = freopen(logfile, "w", stdout);
  if(stdout == NULL){
		perror("freopen");
		return -1;
	}
  bzero(data, sizeof(struct DATA));

  q_alloc(&rq, CHILDREN);  /* one request for each child */


  int i;
  //set resources as static.
  //Their max value is 1, so only one user can take them at a time
	for(i=0; i < RESOURCES; i++){
		data->res.max[i] = 1;
	}

  //send randomly 20 % of the resources as shared
  int r = (rand() % 100 < 40) ? 5 : 4;  //20 or 25 %
  for(i=0; i < RESOURCES / r; i++){
    int rid = rand_rid(data->res.max);
    data->res.max[rid] = 1 + (rand() % 10);
  }

  //set current available resoruces to max
  add_r(data->res.cur, data->res.max);

  printf("Initial resources:\n");
  print_rids();
  print_r("OSS ", data->res.max);

  return 0;
}

static void move_time(){
  struct oss_time t;		//clock increment
  t.sec = 1;
  t.nsec = rand() % 10000;

  lock(semid);
  oss_time_update(&data->clock, &t);
  unlock(semid);
}

static void signal_handler(const int signal){
  is_signaled = 1;
  printf("[%li:%li] OSS: Signaled with %d\n", data->clock.sec, data->clock.nsec, signal);
}

int main(const int argc, const char * argv[]){

  signal(SIGINT,  signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGALRM, signal_handler);
  signal(SIGCHLD, SIG_IGN);

  if(argc == 2){
    if(argv[1][1] == 'v'){
      is_verbose = 1;
    }else{
      fprintf(stderr, "Error: Invalid option\n");
      return -1;
    }
  }

  if((attach(1) == -1) || (oss_init() == -1)){
    return -1;
  }

  struct oss_time fork_time;	//when to fork another process
	bzero(&fork_time, sizeof(struct oss_time));

  while(!is_signaled && (child_done < child_max)){

    move_time();

		//if we are ready to fork, start a process
    user_fork(&fork_time);

    //resource logic
		if(collect_requests() < 0){
      break;
    }

    if(dispatch_q() == 0){  //if we didn't dispatch
      if(data->clock.sec % 2){  //run deadlock every other second
    	   escape_deadlock(); //test for deadlock
      }
    }

  	check_lines();
  }

	output_stat();

  alarm_children();
  detach();

  return 0;
}
