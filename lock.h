union semun {
	int val;
	struct semid_ds *buf;
	unsigned short  *array;
};

int lock(const int);
int unlock(const int);
