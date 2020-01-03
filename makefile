C=gcc
CFLAGS= -g -std=c11 -Wall -pedantic
OBJECT1= cserver.o
OBJECT2= cclient.o

all: cserver cclient
cserver: $(OBJECT1)
	$(CC) $(CFLAGS) -o cserver $(OBJECT1)
cclient: $(OBJECT2)
	$(CC) $(CFLAGS) -o cclient $(OBJECT2)
cserver.o: cserver.c
	$(CC) -c $(CFLAGS) cserver.c
cclient.o: cclient.c
	$(CC) -c $(CFLAGS) cclient.c

.PHONY: clean
clean:
	rm *.o cclient cserver
