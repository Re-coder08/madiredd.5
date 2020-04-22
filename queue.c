#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int q_alloc(queue_t * q, const int qsize){


  q->items = (struct qitem*) malloc(sizeof(struct qitem)*qsize);
  if(q->items == NULL){
    perror("malloc");
    return -1;
  }

  memset(q->items, -1, sizeof(struct qitem)*q->len);
  q->size = qsize;
  q->front = q->back = q->len = 0;

  return 0;
}

void q_dealloc(queue_t * q){
  free(q->items);
}

int q_push(queue_t * q, const struct qitem *item){
  if(q->len >= q->size){
    return -1;
  }

  q->items[q->back++] = *item;
  q->back %= q->size;
  q->len++;

  return 0;
}

struct qitem* q_pop(queue_t * q){
  struct qitem* item = &q->items[q->front++];
  q->len--;
  q->front %= q->size;

  return item;
}

void q_drop(queue_t * q, int nth){
  int i;
  for(i=0; i < q->len; i++){
    struct qitem * msg = q_pop(q);
    if(i != nth){
      q_push(q, msg);
    }
  }
}

struct qitem* q_front(queue_t * q){
  return &q->items[q->front];
}

struct qitem * q_at(queue_t * q, const int i){
  if(i > q->len){
    return NULL;
  }
  const int at = (q->front + i) % q->size;
  return &q->items[at];
}

int q_len(const queue_t * q){
  return q->len;
}
