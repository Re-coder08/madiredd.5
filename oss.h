#include "oss_time.h"
#include "child.h"
#include "queue.h"

typedef struct DATA {
	struct oss_time clock;
	struct child children[CHILDREN];

	struct resources res;
} DATA;

struct msgbuf {
	long mtype;
	struct qitem msg;
};

#define MSG_LEN sizeof(struct qitem)
