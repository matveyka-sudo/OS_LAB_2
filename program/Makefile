CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic

PROGRAMS = parent child1 child2

all: $(PROGRAMS)

parent: parent.c
	$(CC) $(CFLAGS) -o $@ $<

child1: child1.c
	$(CC) $(CFLAGS) -o $@ $<

child2: child2.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMS)

.PHONY: all clean
