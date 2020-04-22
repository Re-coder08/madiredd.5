
enum decision {RECEIVE=0, RETURN};
enum oss_decision {ALLOW=0, DELAY, DENY};

#define RESOURCES	20

struct resources {
	int max[RESOURCES];	//maximum value
	int cur[RESOURCES];	//current value
};

//return random resource id
int rand_rid(const int r[RESOURCES]);

//fill resource array with random values
//void rand_r(int r[RESOURCES], const int max);
void rand_max(int r[RESOURCES], const int max[RESOURCES]);

void add_r(int a[RESOURCES], const int b[RESOURCES]);
int count_r(const int r[RESOURCES]);
void print_r(const char * label, const int r[RESOURCES]);
void print_rids();
