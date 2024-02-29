#include "RUDP_API.h"
ssize_t rudp_send(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen);

/*
 * Sending data to the peer. The function should wait for an acknowledgment packet,
 * and if it didn’t receive any in timeout microseconds, retransmits the data.
 * Return the bits sent.
 */
ssize_t rudp_send_with_timer(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen, int timeout);

/*
 * Receive data from a peer.
 */
ssize_t rudp_rcv(int socketfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen);

/*
 * Receive data from a peer.
 * If no data received in timeout microseconds returns 0.
 */
int rudp_rcv_with_timer(int sockfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen, int timeout);

/*
 * Accept a connection from a peer
 * Returns -1 if failure.
 */
int rudp_accept(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen);

/*
 * Offer a handshake in order to establish a connection with a peer.
 * Returns -1 if failure.
 */
int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen, int side);

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


int rudp_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}
int rudp_send_file(char* file, int sockfd, struct sockaddr_in receiver_addr, int seqNum) {
    RudpPacket packet;
    packet.seq_num = seqNum;
    struct timeval tv;
    for (int i = 0; i < FILE_SIZE; i+=MAXLINE){
        memcpy(packet.data, file+i, MAXLINE);
        packet.length = htons(MAXLINE);
        packet.flags = 0;
        packet.checksum = calculate_checksum(packet.data, packet.length);
        printf("Sending packet %d\n",packet.seq_num);
        gettimeofday(&tv,NULL);
        packet.usec = tv.tv_usec;
        packet.sec = tv.tv_sec;
        if (rudp_send_with_timer(sockfd, &packet, &receiver_addr, sizeof(receiver_addr),TIMEOUT) < 0){
            perror("sendto failed");
            //free(packet);
            return -1;
        }
        //printf("Sent packet %d\n", packet.seq_num);
        packet.seq_num++;
    }
    return 0;
}


ssize_t rudp_send(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen) {
    return sendto(sockfd, (const char *) rudp_packet, sizeof(RudpPacket) + rudp_packet->length, 0,
                  (struct sockaddr *) serv_addr, addrlen);
}

ssize_t rudp_send_with_timer(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen,
                     int timeout) {
    ssize_t bytes_sent;
    struct timeval tv;
    fd_set readfds;

    // Set timeout for socket
    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv));

    // Send the packet
    bytes_sent = rudp_send(sockfd, rudp_packet, serv_addr, addrlen);
    if (bytes_sent < 0) {
        perror("sendto failed");
        return -1;
    }

    // Wait for ACK with the same seqnum
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (ready < 0) {
            perror("select failed");
            return -1;
        } else if (ready == 0) { // Timeout reached, resend the packet and reset the timer
            bytes_sent = rudp_send(sockfd, rudp_packet, serv_addr, addrlen);
            if (bytes_sent < 0) {
                perror("sendto failed");
                return -1;
            }
            // Reset the timer
            tv.tv_sec = timeout / 1000000;
            tv.tv_usec = timeout % 1000000;
            printf("Timeout for packet %d\n", rudp_packet->seq_num);
            continue; // Continue waiting for ACK
        } else {
            // Socket is ready to read, check for ACK
            RudpPacket ack_packet;
            ssize_t bytes_received = rudp_rcv(sockfd, &ack_packet, serv_addr, &addrlen);
            if (bytes_received < 0) {
                perror("recvfrom failed");
                return -1;
            }
            // Check if received packet is an ACK with the same seqnum
            if ((ack_packet.flags & FLAG_ACK) && ack_packet.seq_num == rudp_packet->seq_num) {
                // ACK received, return
                printf("Received ACK %d\n", ack_packet.seq_num);
                return bytes_received;
            }
            // If not an ACK with the same seqnum, continue waiting
        }
    }
}

ssize_t rudp_rcv(int socketfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen) {
    return recvfrom(socketfd, rudp_packet, sizeof(RudpPacket) + rudp_packet->length, 0, (struct sockaddr *) src_addr,
                    addrlen);
}

int rudp_rcv_with_timer(int sockfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen,
                        int timeout) {
    struct timeval tv;
    fd_set readfds;

    // Set timeout for socket
    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;

    // Initialize read file descriptor set
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    // Wait for data to be available on the socket or timeout
    int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if (ready < 0) {
        perror("select failed");
        return -1;
    } else if (ready == 0) {
        // Timeout reached, no data available
        return 0;
    } else {
        // Data available, receive the packet
        ssize_t bytes_received = rudp_rcv(sockfd, rudp_packet, src_addr, addrlen);
        if (bytes_received < 0) {
            perror("recvfrom failed");
            return -1;
        }
        return bytes_received;
    }
}

int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen, int side) {
    if(side == 1) {
        return rudp_accept(sockfd, dest_addr, addrlen);
    }
    RudpPacket syn_packet, synack_packet, ack_packet;
    //socklen_t* len = &addrlen;
    ssize_t bytes_sent;
    struct timeval tv;
    fd_set readfds;

    // Set timeout for socket
    tv.tv_sec = TIMEOUT / 1000000;
    tv.tv_usec = TIMEOUT % 1000000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv));

    // Prepare SYN packet
    syn_packet.length = htons(0); // No data in SYN packet
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_num = 0;

    // Send the packet
    bytes_sent = rudp_send(sockfd, &syn_packet, dest_addr, addrlen);
    if (bytes_sent < 0) {
        perror("sendto failed");
        return -1;
    }
    printf("Sent SYN %d\n", syn_packet.seq_num);

    // Wait for ACK with the same seqnum
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (ready < 0) {
            perror("select failed");
            return -1;
        } else if (ready == 0) { // Timeout reached, resend the packet and reset the timer
            bytes_sent = rudp_send(sockfd, &syn_packet, dest_addr, addrlen);
            if (bytes_sent < 0) {
                perror("sendto failed");
                return -1;
            }
            // Reset the timer
            tv.tv_sec = TIMEOUT / 1000000;
            tv.tv_usec = TIMEOUT % 1000000;
            printf("Timeout for SYN %d\n", syn_packet.seq_num);
            continue; // Continue waiting for ACK
        } else {
            // Socket is ready to read, check for ACK
            ssize_t bytes_received = rudp_rcv(sockfd, &synack_packet, dest_addr, &addrlen);
            if (bytes_received < 0) {
                perror("recvfrom failed");
                return -1;
            }
            // Check if received packet is an ACK with the same seqnum
            if ((synack_packet.flags & FLAG_SYN) && (synack_packet.flags & FLAG_ACK) &&
                synack_packet.seq_num == syn_packet.seq_num) {
                // ACK received, return
                printf("Received SYNACK %d\n", synack_packet.seq_num);
                break;
            }
            // If not an ACK with the same seqnum, continue waiting
        }
    }

    ack_packet.length = htons(0); // No data in ACK packet
    ack_packet.flags = FLAG_ACK;
    ack_packet.seq_num = synack_packet.seq_num; // the squence number of the expected

    // Send ACK packet
    if (rudp_send(sockfd, &ack_packet, dest_addr, addrlen) < 0) {
        perror("sendto failed");
        return -1;
    }

    printf("Sent ACK packet %d\n", ack_packet.seq_num);

    while (rudp_rcv_with_timer(sockfd,&synack_packet,dest_addr, &addrlen,TIMEOUT*2)) {
        RudpPacket reack_packet;
        reack_packet.length = htons(0); // No data in ACK packet
        reack_packet.flags = FLAG_ACK;
        reack_packet.seq_num = synack_packet.seq_num; // the squence number of the expected

        // Send ACK packet
        if (rudp_send(sockfd, &reack_packet, dest_addr, sizeof(addrlen)) < 0) {
            perror("sendto failed");
            return -1;
        }
    }

    return 0; // Handshake successful
}

int rudp_accept(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    RudpPacket syn_packet, synack_packet;
    socklen_t *len = &addrlen;

    // Receive SYN packet
    int status = rudp_rcv_with_timer(sockfd, &syn_packet, dest_addr, len,5000000);
    if (status < 0) {
        perror("rcv failed");
        return -1;
    } else if (status == 0) {
        return -2;
    }

    // Check if it's a valid SYN packet
    if (syn_packet.flags & FLAG_SYN) {
        // Prepare SYNACK packet
        synack_packet.length = htons(0); // No data in ACK packet
        synack_packet.flags = FLAG_ACK + FLAG_SYN;
        synack_packet.seq_num = syn_packet.seq_num;
        // Send SYNACK packet
        printf("Sending SYNACK packet\n");
        if (rudp_send_with_timer(sockfd, &synack_packet, dest_addr, addrlen, TIMEOUT) < 0) {
            perror("sendto failed");
            return -1;
        }
        return 0;
    } else {
        printf("Invalid SYN packet\n");
        return -1; // Handshake failed
    }
}

int rudp_close_sender(int sockfd, struct sockaddr_in receiver_addr) {
    // Wait for the FIN from receiver
    RudpPacket last_packet;
    socklen_t len = sizeof(receiver_addr);
    ssize_t bytes_recv = rudp_rcv(sockfd, &last_packet, &receiver_addr, &len);
    while (bytes_recv < 0) {
        bytes_recv = rudp_rcv(sockfd, &last_packet, &receiver_addr, &len);
    }
    if (!(last_packet.flags & FLAG_FIN)) {
        perror("last packet received needs to be FIN");
        return -1;
    }

    printf("Received FIN %d\n",last_packet.seq_num);
    // ACK the FIN
    last_packet.flags = FLAG_ACK;
    if (rudp_send(sockfd, &last_packet, &receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("sendto failed");
        //free(packet);
        return -1;
    }

    // wait a little to ensure no FIN retransmission
    while (rudp_rcv_with_timer(sockfd,&last_packet,&receiver_addr,&len,TIMEOUT*2)) {
        if (!(last_packet.flags & FLAG_FIN)) {
            perror("last packet received needs to be FIN");
            return -1;
        }

        printf("Received FIN %d\n",last_packet.seq_num);
        // ACK the FIN
        last_packet.flags = FLAG_ACK;
        if (rudp_send(sockfd, &last_packet, &receiver_addr, sizeof(receiver_addr)) < 0) {
            perror("sendto failed");
            return -1;
        }
    }
    return 0;
}

int rudp_close(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen, int side) { //side 0 means sender, 1 means receiver
    RudpPacket packet;
    if (side == 1) {
        packet.flags = FLAG_ACK;
        rudp_send(sockfd, &packet, dest_addr, addrlen);
        puts("Sent ACK for FIN");

        // wait a little to ensure no FIN retransmission
        while (rudp_rcv_with_timer(sockfd,&packet,dest_addr,&addrlen,TIMEOUT*2)) {
            packet.flags = FLAG_ACK;
            rudp_send(sockfd, &packet, dest_addr, addrlen);
            puts("Sent ACK for FIN");
        }
    }
    RudpPacket fin_packet;

    // Prepare FIN packet
    fin_packet.length = htons(0); // No data in SYN packet
    fin_packet.flags = FLAG_FIN;
    fin_packet.seq_num = ((unsigned int) rand() % 256);

    // Send FIN packet
    if (rudp_send_with_timer(sockfd, &fin_packet, dest_addr, addrlen, TIMEOUT) < 0) {
        perror("sendto failed");
        return -1;
    }

    if (side == 0) { // if it's the sender wait for the FIN
        return rudp_close_sender(sockfd, *dest_addr);
    }
    return 0;
}

int rudp_rcv_file(char* file, int sockfd, struct sockaddr_in sender_addr, int seqNum, double times[], int* currentRun) {
    RudpPacket packet;
    char* current = &file[0];
    socklen_t sender_addr_len = sizeof(sender_addr);
    struct timeval start, end;
    int first = 1;
    while (1) {
        printf("Waiting for packet %d\n",seqNum);
        ssize_t bytes_recv = rudp_rcv(sockfd, &packet, &sender_addr, &sender_addr_len);
        while (bytes_recv < 0) {
            bytes_recv = rudp_rcv(sockfd, &packet, &sender_addr, &sender_addr_len);
        }
        if (first) {
            first = 0;
            start.tv_sec = packet.sec;
            start.tv_usec = packet.usec;
        }
        if ((packet.flags & FLAG_FIN)) {
            printf("Received FIN packet %d.\n",packet.seq_num);
            gettimeofday(&end,NULL);
            times[*currentRun] = (end.tv_sec - start.tv_sec) * 1000.0;
            times[*currentRun] += (end.tv_usec - start.tv_usec) / 1000.0;
            (*currentRun)++;
            return 0;
        }
        printf("Received packet %d\n",packet.seq_num);
        packet.flags = FLAG_ACK;
        //if the packet is not the expected one or the checksum is not correct, ask for retransmission
        if (packet.seq_num != seqNum || calculate_checksum(packet.data, packet.length) != packet.checksum) {
            packet.seq_num = seqNum-1;
            printf("Packet had error. Sending ACK With previous seqnum %d\n",packet.seq_num);
            rudp_send(sockfd, &packet, &sender_addr, sizeof(sender_addr));
        }
        else //if the packet is the expected one and the checksum is correct, send an ack, and update the sequence number for the next packet
        {
            //acked = 1;
            packet.seq_num = seqNum;
            printf("Packet is good. Sending ACK %d\n",packet.seq_num);
            rudp_send(sockfd, &packet, &sender_addr, sizeof(sender_addr));
            seqNum++;
            char* data = packet.data;
            for (int i = 0; i < packet.length; i++) {
                *current=data[i];
                current++;
            }
        }
    }
}


unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *) data;
    unsigned int total_sum = 0;
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0)
        total_sum += *((unsigned char *) data_pointer);
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    return (~((unsigned short int) total_sum));
}