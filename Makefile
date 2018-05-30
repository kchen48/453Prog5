C = gcc
CFLAGS = -Wall -ansi -pedantic -g
all: minget minls

minget: minget.c min.h
	 $(CC) $(CFLAGS) -o minget minget.c

minls: minls.c min.h
	 $(CC) $(CFLAGS) -o minls minls.c

clean:
	 rm -f $(OBJS) *~ TAGS
