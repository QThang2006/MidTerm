CC      ?= cc
CFLAGS  += -Wall -Wextra -Wpedantic -g -O2
OBJS     = ls.o cmp.o print.o util.o
PROG     = ls

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

ls.o:    ls.c ls.h cmp.h print.h util.h
cmp.o:   cmp.c cmp.h ls.h
print.o: print.c print.h cmp.h util.h ls.h
util.o:  util.c util.h ls.h

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(PROG)
	sh ./test.sh

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: all clean test
