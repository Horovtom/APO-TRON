#CC=gcc
CC=arm-linux-gnueabihf-gcc
CFLAGS=-g -std=c99 -pedantic -Wall
LDFLAGS=-lrt -lpthread

%.o:%.c
	$(CC) $(CFLAGS) -c $<

all: mzapo_lcdtest

mzapo_lcdtest: mzapo_lcdtest.o
	$(CC) $(CFLAGS) $(LDFLAGS) -L. $^ -o $@


.PHONY : depend dep all

dep:
	$(CC) $(CFLAGS) $(CPPFLAGS) -w -E -M *.c \
	  > depend

depend:
	@touch depend

clean:
	rm -f *.o *.a mzapo_lcdtest depend

-include depend
