C = gcc
CFLAGS = -Wall -ansi -pedantic -g
all: minget minls

min.o: min.c min.h
	$(CC) $(CFLAGS) -c -o min.o min.c

minls.o: minls.c
	$(CC) $(CFLAGS) -c -o minls.o minls.c

minget.o: minget.c
	$(CC) $(CFLAGS) -c -o minget.o minget.c

minget: minget.o min.o
	 $(CC) $(CFLAGS) minget.o min.o -o minget

minls: minls.o min.o
	 $(CC) $(CFLAGS) minls.o min.o -o minls

clean:
	 rm -f $(OBJS) *~ TAGS
