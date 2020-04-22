CC=gcc
CFLAGS=-Wall -g -pedantic -D_XOPEN_SOURCE

sim: oss user

oss: oss.c oss_time.o queue.o res.o child.o lock.o oss.h
	$(CC) $(CFLAGS) oss.c oss_time.o queue.o res.o child.o lock.o -o oss -lrt

user: user.o oss_time.o res.o child.o lock.o oss.h
	$(CC) $(CFLAGS) user.o oss_time.o queue.o res.o child.o lock.o -o user

user.o: user.c child.h
	$(CC) $(CFLAGS) -c user.c

oss_time.o: oss_time.h oss_time.c
	$(CC) $(CFLAGS) -c oss_time.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

res.o: res.c res.h
	$(CC) $(CFLAGS) -c res.c

clean:
	rm -f ./oss ./user *.log *.o output.txt
