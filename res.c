#include <stdio.h>
#include <stdlib.h>
#include "res.h"

int rand_rid(const int r[RESOURCES]){

  int i, no_zero=0;

  //count how much of the resource descriptors are non-zero
	for(i=0; i < RESOURCES; i++){
		if(r[i] > 0)
			no_zero++;
	}

	if(no_zero == 0)
    return -1;

	int nth = rand() % no_zero;

  //get id of the nth non-zero resource
  for(i=0; i < RESOURCES; i++){
		if(r[i] > 0){
			if(--nth <= 0){
        return i;
      }
    }
	}
	return -1;
}

/*void rand_r(int r[RESOURCES], const int max){
  int i;
	for(i=0; i < RESOURCES; i++){
		r[i] = 1 + (rand() % max);
	}
}*/

void rand_max(int r[RESOURCES], const int max[RESOURCES]){
  int i;
	for(i=0; i < RESOURCES; i++){
		r[i] = 1 + (rand() % max[i]);
	}
}

void add_r(int a[RESOURCES], const int b[RESOURCES]){
  int i;
  for(i=0; i < RESOURCES; i++){
    a[i] += b[i];
  }
}

int count_r(const int r[RESOURCES]){
  int i, count = 0;
  for(i=0; i < RESOURCES; i++){
    if(r[i] > 0){
      count++;
    }
  }
  return count;
}

void print_r(const char * label, const int r[RESOURCES]){
  int i;
  printf("%s", label);
  for(i=0; i < RESOURCES; i++){
    printf("%3d ", r[i]);
  }
  printf("\n");
}

void print_rids(){
  int i;

  printf("    ");
	for(i=0; i < RESOURCES; i++){
		printf("R%02d ", i);
  }
	printf("\n");
}
