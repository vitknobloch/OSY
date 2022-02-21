CFLAGS+= -Wall -g -O2
EXTRA_CFLAGS?= -nostdlib -nostdinc -static -m32
LINKS=
HW=hw05
ZIP=zip
OBJ=$(patsubst %.c,%.o, $(wildcard *.c))
CC=gcc

$(HW): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(EXTRA_CFLAGS) $(LINKS) -o $(HW)

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $< $(EXTRA_CFLAGS) -o $@

main.o: main.c
	$(CC) -c $(CFLAGS) $< $(EXTRA_CFLAGS) -o $@

zip:
	$(ZIP) $(HW).zip Makefile *.c *.h

clean:
	$(RM) -f *.o
	$(RM) -f $(HW)
	$(RM) -f $(HW).zip
	$(RM) -f datapub/*.myout
	$(RM) -f datapub/*.myerr
	$(RM) -f vgcore.*

.PHONY: clean zip