# Author:     Prasanna Paithankar
# Roll No.:   21CS30065
# Course:     Operating Systems Laboratory Spr 2023-24
# Date:       07/04/2024
# File:       Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g -pedantic
DFLAGS = -DVERBOSE

BINARIES = Master MMU sched process

.PHONY: all clean tar run
	
all: $(BINARIES)

$(BINARIES): %: %.c
	$(CC) $(CFLAGS) $< -o $@ -lrt $(DFLAGS)

tar:
	tar -czvf 21CS30065_LA8.tar.gz Makefile Master.c MMU.c sched.c process.c result.txt

run: all
	./Master -k 9 -m 5 -f 3
	
clean:
	rm -rf $(BINARIES) 21CS30065_LA8.tar.gz
	-rm /dev/mqueue/mq* /dev/shm/SM*
