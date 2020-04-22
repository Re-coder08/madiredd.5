#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "lock.h"

int lock(const int semid){
  struct sembuf semops;

  semops.sem_num = 0;
  semops.sem_flg = 0;
	semops.sem_op  = -1;

  return semop(semid, &semops, 1);
}

int unlock(const int semid){
  struct sembuf semops;

  semops.sem_num = 0;
  semops.sem_flg = 0;
  semops.sem_op  = 1;

  return semop(semid, &semops, 1);
}
