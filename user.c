#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include "lock.h"
#include "oss.h"

static struct DATA * data = NULL;
static int semid = -1;

int attach(){

 key_t k = ftok("oss.c", 1);

 const int shmid = shmget(k, sizeof(struct DATA), 0);
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
 const int qid = msgget(k, 0);
 if(qid == -1){
	 perror("msgget");
	 return -1;
 }

 k = ftok("oss.c", 3);
 semid = semget(k, 1, 0);
 if(semid == -1){
   perror("semget");
   return -1;
 }

 return qid;
}

int main(const int argc, const char * argv[]){

  const int id = atoi(argv[1]);

	struct msgbuf buf;

	const int qid = attach();
	if(qid < 0){
    fprintf(stderr, "User can't attach\n");
		return -1;
	}

	srand(getpid());

  //decide what resources we need
  int * max = data->children[id].res.max;
  rand_max(max, data->res.max);

  int max_decision = 60;  //we should start with a receive, since we have nothing to return

  lock(semid);
  struct oss_time max_time = data->clock;

  max_time.sec += 15; //make user run at least 5 virtual seconds


  while(oss_time_compare(&max_time, &data->clock) == 1){
    unlock(semid);

		bzero(&buf, sizeof(struct msgbuf));


		//what decision the user will take - take or release
    buf.msg.id = id;
		buf.msg.decision = ((rand() % max_decision) < 60) ? RECEIVE : RETURN;
    buf.msg.rid = rand_rid(max);


    if(buf.msg.rid == -1){
      if(buf.msg.decision == RECEIVE){  //if no more resource needed
        break;  //quit
      }else{  //nothing more to return
        max_decision = 60;  //disable returns
        lock(semid);
        continue;
      }
    }

    buf.msg.val = 1 + (rand() % max[buf.msg.rid]);
    buf.msg.result = DENY;

    //send our decision and receive oss decision for resource
		buf.mtype = getppid();
		if( (msgsnd(qid, (void*)&buf, MSG_LEN, 0) == -1) ||
        (msgrcv(qid, (void*)&buf, MSG_LEN, getpid(), 0) == -1) ){
	    //perror("msgsnd");
	    break;
	  }

    if(buf.msg.result == ALLOW){	// receive accepted

      max_decision = 100; //allow returns
      max[buf.msg.rid] -= buf.msg.val;  //reduce our need

		}else if(buf.msg.result == DENY){	// release
      lock(semid); //lock before comparing time
      break;  //quit
		}

    lock(semid); //lock before comparing time
  }
  unlock(semid);

  //return all resources
  buf.mtype = getppid();
  buf.msg.id = id;
  buf.msg.decision = RETURN;
  buf.msg.rid = -1; //all
  buf.msg.val = 0;
  msgsnd(qid, (void*)&buf, MSG_LEN, 0);

	shmdt(data);

  return 0;
}
