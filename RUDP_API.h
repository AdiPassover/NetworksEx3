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

#define MAXLINE 1024
#define FLAG_ACK 1
#define FLAG_SYN 2
#define FLAG_FIN 4


typedef struct _RudpPacket {
    uint16_t length;
    uint16_t checksum;
    uint8_t flags;
    int seq_num;
    char data[MAXLINE];
} RudpPacket;
/*
 * Creating a RUDP socket and creating a handshake between two peers.
 */
int rudp_socket();

/*
 * Sending data to the peer. The function should wait for an
 * acknowledgment packet, and if it didn’t receive any, retransmits the data.
 */
ssize_t rudp_send(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen);

/*
 * Receive data from a peer.
 */
ssize_t rudp_rcv(int socketfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen);

/*
 * Accept a connection from a peer
 * Returns -1 if failure.
 */
int rudp_accept(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen);

/*
 * Offer a handshake in order to establish a connection with a peer.
 * Returns -1 if failure.
 */
int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen);

/*
 * Closes a connection from a peer
 * Returns -1 if failure.
 */
int rudp_close(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen);

/*
* @brief A checksum function that returns 16 bit checksum for data.
* @param data The data to do the checksum for.
* @param bytes The length of the data in bytes.
* @return The checksum itself as 16 bit unsigned number.
* @note This function is taken from RFC1071, can be found here:
* @note https://tools.ietf.org/html/rfc1071
* @note It is the simplest way to calculate a checksum and is not very strong.
* However, it is good enough for this assignment.
* @note You are free to use any other checksum function as well.
* You can also use this function as such without any change.
*/
unsigned short int calculate_checksum(void *data, unsigned int bytes);