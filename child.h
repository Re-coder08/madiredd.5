#ifndef CHILD_H
#define CHILD_H

#include "oss_time.h"
#include <sys/types.h>
#include "res.h"

#define CHILDREN 20

enum state { CHILD_READY=1, CHILD_WAIT, CHILD_DONE };

struct child {
	pid_t	pid;	/* process ID */
	int cid;		/* child   ID */

	enum state state;	/* process state */
	struct resources res;
};

void child_reset(struct child * children, const unsigned int i);
int child_fork(struct child * children, const int id);

int clear_terminated(struct child * children);

#endif
