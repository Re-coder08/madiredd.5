#include "oss_time.h"

//maximum nanoseconds in a second
#define NS_MAX 1000000000

int oss_time_compare(const struct oss_time *x, const struct oss_time *y){
  int cmp = -1; // >
  if(	(y->sec  < x->sec) ||
     ((y->sec == x->sec) && (y->nsec <= x->nsec)) ){
    cmp = 1; // <
  }else{
    cmp = 0; // ==
  }
  return cmp;
}

void oss_time_update(struct oss_time * t, const struct oss_time * x){

  t->sec  += x->sec;
  t->nsec += x->nsec;
  if(t->nsec > NS_MAX){
    t->sec++;
    t->nsec %= NS_MAX;
  }
}

void oss_time_substract(struct oss_time *t, const struct oss_time *x, const struct oss_time *y){

  t->sec = x->sec  - y->sec;
  t->nsec = x->nsec - y->nsec;
  if (t->nsec < 0) {
    t->sec -= 1;
    t->nsec = y->nsec - x->nsec;
  }
}

void oss_time_divide(struct oss_time *t, const unsigned int x){
  t->sec  = t->sec / x;
  t->nsec = t->nsec / x;
}
