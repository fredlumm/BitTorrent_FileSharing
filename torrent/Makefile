CFLAGS=-O2 -pthread -g -ggdb3 -std=gnu99 -lssl -lcrypto
OBJS=sha1.o csapp.o torrent.o 
PROGRAM=torrent

$(PROGRAM): $(OBJS)
	gcc $(OBJS) $(CFLAGS) -o $@

%.o: %.c csapp.h sha1.h
	gcc -c $(CFLAGS) $<

clean:
	rm -f $(OBJS) $(PROGRAM) *~
