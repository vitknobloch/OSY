CC=gcc
EXTRA_CFLAGS=
CFLAGS+= -Wall
MAIN_OBJ=forkpipe.o gen.o
NSD_OBJ=nsd_main.o nd.o nsd.o

all: forkpipe nsd

forkpipe: $(MAIN_OBJ)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o forkpipe $(MAIN_OBJ)

nsd: $(NSD_OBJ)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o nsd $(NSD_OBJ)

forkpipe.o: forkpipe.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

nsd_main.o: nsd_main.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

%.o: %.c %.h
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

zip:
	tar czf osy04.tgz Makefile *.[ch]

clean:
	rm -f *.o forkpipe nsd osy04.tgz
