#pragma once

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

/*
 * Creating a RUDP socket and creating a handshake between two peers.
 */
int rudp_socket();

/*
 * Sending data to the peer. The function should wait for an
 * acknowledgment packet, and if it didn’t receive any, retransmits the data.
 */
ssize_t rudp_send(int socketfd, const void *buffer, size_t msgSize, int flags);

/*
 * Receive data from a peer.
 */
ssize_t rudp_rcv(int socketfd, void *buffer, size_t msgSize, int flags);

/*
 * Closes a connection between peers.
 */
void rudp_close(int socketfd);

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