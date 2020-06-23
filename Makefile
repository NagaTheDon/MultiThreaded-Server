.DEFAULT_GOAL := all

CC=g++
CFLAGS=-Wall -pthread -std=c++11

hello:
	$(CC) $(CFLAGS) -o hello ./learn/hello-world.cpp

thread-waiting: 
	$(CC) $(CFLAGS) -o thread-wait ./learn/thread-waiting.cpp

run-background: 
	$(CC) $(CFLAGS) -o run-background ./learn/run-background.cpp

all: hello thread-waiting run-background

clean:
	rm -f build/bin