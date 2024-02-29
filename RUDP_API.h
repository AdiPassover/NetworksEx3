#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#define MAXLINE 2048
#define FILE_SIZE 2000000
#define MAX_RUNS 100

#define FLAG_ACK 1
#define FLAG_SYN 2
#define FLAG_FIN 4

#define TIMEOUT 5000 // microseconds


typedef struct _RudpPacket {
    uint16_t length;
    uint16_t checksum;
    uint8_t flags;
    int seq_num;
    ulong sec;
    ulong usec;
    char data[MAXLINE];

} RudpPacket;

int rudp_send_file(char* file, int sockfd, struct sockaddr_in receiver_addr,int seqNum);


int rudp_rcv_file(char* file, int sockfd, struct sockaddr_in receiver_addr, int seqNum,double* time, int* run);


/*
 * Creating a RUDP socket and creating a handshake between two peers.
 */
int rudp_socket();


/*
 * Offer a handshake in order to establish a connection with a peer.
 * Returns -1 if failure.
 */
int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen, int side);

/*
 * Closes a connection from a peer
 * Returns -1 if failure.
 */
int rudp_close(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen, int side);

