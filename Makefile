CC=gcc
RELEASE=-O3 -fomit-frame-pointer
DEBUG=-g -O0
CFLAGS=-Wall -Werror $(RELEASE) #$(DEBUG)
OBJS=main.o
PROG=checkers

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: clean
clean:
	rm -rf $(PROG) $(OBJS)

