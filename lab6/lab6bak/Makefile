# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall -Wextra -Wpedantic
# I don't run all the warnings on the given code since it doesn't pass
CSAPPFLAGS = -g 
LDFLAGS = -lpthread

all: proxy

clienthandler.o: clienthandler.c clienthandler.h 
	$(CC) $(CFLAGS) -c clienthandler.c

debug.o: debug.c debug.h
	$(CC) $(CFLAGS) -c debug.c

csapp.o: csapp.c csapp.h
	$(CC) $(CSAPPFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h debug.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o debug.o clienthandler.o 
	$(CC) $(CFLAGS) proxy.o csapp.o debug.o clienthandler.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

