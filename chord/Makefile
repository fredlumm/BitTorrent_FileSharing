CC = gcc
CFLAGS = -O2 -std=gnu99 
LDFLAGS = -pthread 
#LDFLAGS = -mthreads

OBJS = chord.o csapp.o sha1.o
OBJS_QUERY = query.o csapp.o sha1.o
#display.o

TEAM = NOBODY
VERSION = 1

all: chord query

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c -ggdb3 

sha1.o: sha1.c 
	$(CC) $(CFLAGS) -c sha1.c -ggdb3

chord.o: chord.c chord.h
	$(CC) $(CFLAGS) -c chord.c -ggdb3 -lm

query.o: query.c
	$(CC) $(CFLAGS) -c query.c -ggdb3
	
#display.o: display.c
#	$(CC) $(CFLAGS) -I/usr/X11R6/include  -c display.c

chord: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -lm -o chord -ggdb3
#handin:
#	cp chord.c $(INDIR)/$(TEAM)-$(VERSION)-chord.c
#	cp csapp.c $(INDIR)/$(TEAM)-$(VERSION)-csapp.c
#	cp csapp.h $(INDIR)/$(TEAM)-$(VERSION)-csapp.h

query: $(OBJS_QUERY)
	$(CC) $(LDFLAGS) $(OBJS_QUERY) -o query -ggdb3

clean:
	rm -f *~ csapp.o chord chord.o display.o sha1.o core query.o query
