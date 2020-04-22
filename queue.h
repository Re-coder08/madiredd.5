struct qitem {
	int id;									//who is sending messag
	int decision;	//receive or return
	int result;
  int rid;	//resource id
  int val;	//value
};

typedef struct circular_queue {
	struct qitem *items;
	int front, back;
	int size;
	int len;
} queue_t;

int q_alloc(queue_t * q, const int size);
void q_dealloc(queue_t * q);

int q_push(queue_t * q, const struct qitem *);
struct qitem * q_pop(queue_t * q);
void q_drop(queue_t * q, int nth);

struct qitem * q_front(queue_t * q);
struct qitem * q_at(queue_t * q, const int i);
int q_len(const queue_t * q);
