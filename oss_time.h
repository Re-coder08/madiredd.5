#ifndef OSS_TIME_H
#define OSS_TIME_H

struct oss_time {
  long int sec;
  long int nsec;
};

int  oss_time_compare(const struct oss_time *x, const struct oss_time *y);
void oss_time_update(struct oss_time * res, const struct oss_time * b);
void oss_time_substract(struct oss_time *res, const struct oss_time *a, const struct oss_time *b);
void oss_time_divide(struct oss_time *res, const unsigned int d);

#endif
