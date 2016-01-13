CC=gcc
RELEASE=-O2
DEBUG=-g -O0
CFLAGS=-Wall -Werror $(DEBUG)
OBJS=main.o
PROG=checkers

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $?
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: clean
clean:
	rm -rf $(PROG) $(OBJS)

