CFLAGS+= -pedantic -Wall -std=c99 -O3 -g
HW=hw04
ZIP=zip
OBJ=$(patsubst %.c,%.o, $(wildcard *.c))

$(HW): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(HW)

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $< -o $@

main.o: main.c
	$(CC) -c $(CFLAGS) $< -o $@

zip:
	$(ZIP) $(HW).zip *.c *.h

clean:
	$(RM) -f *.o
	$(RM) -f $(HW)
	$(RM) -f $(HW).zip
	$(RM) -f datapub/*.myout
	$(RM) -f datapub/*.myerr

.PHONY: clean zip