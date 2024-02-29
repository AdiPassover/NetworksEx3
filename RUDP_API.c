#include "RUDP_API.h"


int rudp_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

ssize_t rudp_send(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen) {
    return sendto(sockfd, (const char *)rudp_packet, sizeof(RudpPacket) + rudp_packet->length, 0, (struct sockaddr *) serv_addr, addrlen);
}

ssize_t rudp_send_with_timer(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen, int timeout) {
    ssize_t bytes_sent;
    struct timeval tv;
    fd_set readfds;

    // Set timeout for socket
    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // Send the packet
    bytes_sent = rudp_send(sockfd,rudp_packet,serv_addr,addrlen);
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
            bytes_sent = rudp_send(sockfd,rudp_packet,serv_addr,addrlen);
            if (bytes_sent < 0) {
                perror("sendto failed");
                return -1;
            }
            // Reset the timer
            tv.tv_sec = timeout / 1000000;
            tv.tv_usec = timeout % 1000000;
            printf("Timeout for packet %d\n",rudp_packet->seq_num);
            continue; // Continue waiting for ACK
        } else {
            // Socket is ready to read, check for ACK
            RudpPacket ack_packet;
            ssize_t bytes_received = rudp_rcv(sockfd,&ack_packet,serv_addr,&addrlen);
            if (bytes_received < 0) {
                perror("recvfrom failed");
                return -1;
            }
            // Check if received packet is an ACK with the same seqnum
            if ((ack_packet.flags & FLAG_ACK) && ack_packet.seq_num == rudp_packet->seq_num) {
                // ACK received, return
                printf("Received ACK %d\n",ack_packet.seq_num);
                return bytes_received;
            }
            // If not an ACK with the same seqnum, continue waiting
        }
    }
}

ssize_t rudp_rcv(int socketfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen) {
    return recvfrom(socketfd, rudp_packet, sizeof(RudpPacket) + rudp_packet->length, 0, (struct sockaddr *) src_addr, addrlen);
}

int rudp_rcv_with_timer(int sockfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen, int timeout) {
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
        ssize_t bytes_received = rudp_rcv(sockfd,rudp_packet,src_addr,addrlen);
        if (bytes_received < 0) {
            perror("recvfrom failed");
            return -1;
        }
        return bytes_received;
    }
}

int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    RudpPacket syn_packet, synack_packet, ack_packet;
    //socklen_t* len = &addrlen;
    ssize_t bytes_sent;
    struct timeval tv;
    fd_set readfds;

    // Set timeout for socket
    tv.tv_sec = TIMEOUT / 1000000;
    tv.tv_usec = TIMEOUT % 1000000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // Prepare SYN packet
    syn_packet.length = htons(0); // No data in SYN packet
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_num = 0;

    // Send the packet
    bytes_sent = rudp_send(sockfd,&syn_packet,dest_addr,addrlen);
    if (bytes_sent < 0) {
        perror("sendto failed");
        return -1;
    }
    printf("Sent SYN %d\n",syn_packet.seq_num);

    // Wait for ACK with the same seqnum
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (ready < 0) {
            perror("select failed");
            return -1;
        } else if (ready == 0) { // Timeout reached, resend the packet and reset the timer
            bytes_sent = rudp_send(sockfd,&syn_packet,dest_addr,addrlen);
            if (bytes_sent < 0) {
                perror("sendto failed");
                return -1;
            }
            // Reset the timer
            tv.tv_sec = TIMEOUT / 1000000;
            tv.tv_usec = TIMEOUT % 1000000;
            printf("Timeout for SYN %d\n",syn_packet.seq_num);
            continue; // Continue waiting for ACK
        } else {
            // Socket is ready to read, check for ACK
            ssize_t bytes_received = rudp_rcv(sockfd,&synack_packet,dest_addr,&addrlen);
            if (bytes_received < 0) {
                perror("recvfrom failed");
                return -1;
            }
            // Check if received packet is an ACK with the same seqnum
            if ((synack_packet.flags & FLAG_SYN) && (synack_packet.flags & FLAG_ACK) && synack_packet.seq_num == syn_packet.seq_num) {
                // ACK received, return
                printf("Received SYNACK %d\n",synack_packet.seq_num);
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

    printf("Sent ACK packet %d\n",ack_packet.seq_num);

    return 0; // Handshake successful
}

int rudp_accept(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    RudpPacket syn_packet, synack_packet;
    socklen_t* len = &addrlen;

    // Receive SYN packet
    if (rudp_rcv(sockfd, &syn_packet, dest_addr, len) < 0) {
        perror("rcv failed");
        return -1;
    }

    // Check if it's a valid SYN packet
    if (syn_packet.flags & FLAG_SYN) {
        // Prepare SYNACK packet
        synack_packet.length = htons(0); // No data in ACK packet
        synack_packet.flags = FLAG_ACK + FLAG_SYN;
        synack_packet.seq_num = syn_packet.seq_num;
        // Send SYNACK packet
        printf("Sending SYNACK packet\n");
        if (rudp_send_with_timer(sockfd, &synack_packet, dest_addr, addrlen,TIMEOUT) < 0) {
            perror("sendto failed");
            return -1;
        }
        return 0;
    } else {
        printf("Invalid SYN packet\n");
        return -1; // Handshake failed
    }
}

int rudp_close(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    RudpPacket fin_packet;

    // Prepare FIN packet
    fin_packet.length = htons(0); // No data in SYN packet
    fin_packet.flags = FLAG_FIN;
    fin_packet.seq_num = ((unsigned int)rand()%256);

    // Send FIN packet
    if (rudp_send_with_timer(sockfd, &fin_packet, dest_addr, addrlen,TIMEOUT) < 0) {
        perror("sendto failed");
        return -1;
    }
    return 0;
}

unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0)
        total_sum += *((unsigned char *)data_pointer);
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    return (~((unsigned short int)total_sum));
}