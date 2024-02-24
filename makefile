CC = gcc
AR = ar
CFL = -Wall -g
LFL = rcs
PART_ONE = TCP_Receiver TCP_Sender
PART_TWO = RUDP_API RUDP_Receiver RUDP_Sender
.PHONY: all clean

all: part1 part2

part1: $(PART_ONE)

part2: $(PART_TWO)

TCP_Receiver: TCP_Receiver.o
	$(CC) $(CFL) $< -o $@

TCP_Receiver.o: TCP_Receiver.c
	$(CC) $(CFL) -c $<

TCP_Sender: TCP_Sender.o
	$(CC) $(CFL) $< -o $@

TCP_Sender.o: TCP_Sender.c
	$(CC) $(CFL) -c $<


RUDP_Receiver: RUDP_Receiver.o RUDP_API.a
	$(CC) $(CFL) $^ -o $@

RUDP_Receiver.o: RUDP_Receiver.c RUDP_API.h
	$(CC) $(CFL) -c $<

RUDP_Sender: RUDP_Sender.o RUDP_API.a
	$(CC) $(CFL) $^ -o $@

RUDP_Sender.o: RUDP_Sender.c RUDP_API.h
	$(CC) $(CFL) -c $<

RUDP_API.a: RUDP_API.o
	$(AR) $(LFL) $@ $^

RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(CFL) -c $<

clean:
	rm -f *.o *.a TCP_Receiver TCP_Sender RUDP_API RUDP_Receiver RUDP_Sender