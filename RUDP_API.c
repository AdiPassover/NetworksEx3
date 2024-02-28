#include "RUDP_API.h"


int rudp_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

ssize_t rudp_send(int sockfd, const RudpPacket *rudp_packet, struct sockaddr_in *serv_addr, socklen_t addrlen) {
    return sendto(sockfd, (const char *)rudp_packet, sizeof(RudpPacket) + rudp_packet->length, 0, (struct sockaddr *) serv_addr, addrlen);
}

ssize_t rudp_rcv(int socketfd, RudpPacket *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen) {
    return recvfrom(socketfd, rudp_packet, sizeof(RudpPacket) + rudp_packet->length, 0, (struct sockaddr *) src_addr, addrlen);
}

int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    RudpPacket syn_packet, synack_packet, ack_packet;
    socklen_t* len = &addrlen;

    // Prepare SYN packet
    syn_packet.length = htons(0); // No data in SYN packet
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_num = 0;

    // Send SYN packet TODO: add a timer
    if (rudp_send(sockfd, &syn_packet, dest_addr, addrlen) < 0) {
        perror("sendto failed");
        return -1;
    }

    printf("Sent SYN packet\n");

    // Receive SYNACK packet
    if (rudp_rcv(sockfd, &synack_packet, dest_addr, len) < 0) {
        perror("recvfrom failed");
        return -1;
    }

    printf("Received SYNACK packet\n");

    // Check if it's a valid SYNACK packet
    if ((synack_packet.flags & FLAG_SYN) && (synack_packet.flags & FLAG_ACK)) {
        // Prepare ACK packet
        ack_packet.length = htons(0); // No data in ACK packet
        ack_packet.flags = FLAG_ACK;
        ack_packet.seq_num = synack_packet.seq_num; // the squence number of the expected 

        // Send ACK packet
        if (rudp_send(sockfd, &ack_packet, dest_addr, addrlen) < 0) {
            perror("sendto failed");
            return -1;
        }

        printf("Sent ACK packet\n");

        return 0; // Handshake successful
    } else {
        printf("Invalid SYNACK packet\n");
        return -1; // Handshake failed
    }
}

int rudp_accept(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    RudpPacket syn_packet, synack_packet, ack_packet;
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
        synack_packet.seq_num = syn_packet.seq_num+1;//incrementing the sequence number, the new sequence number is the sequence number of the expected packet
        // Send SYNACK packet
        if (rudp_send(sockfd, &synack_packet, dest_addr, addrlen) < 0) {
            perror("sendto failed");
            return -1;
        }

    } else {
        printf("Invalid SYN packet\n");
        return -1; // Handshake failed
    }

    printf("Sent SYNACK packet\n");

    // Receive ACK packet
    if (rudp_rcv(sockfd, &ack_packet, dest_addr, len) < 0) {
        perror("recvfrom failed");
        return -1;
    }

    printf("Received ACK packet\n");

    // Check if it's a valid ACK packet
    if (ack_packet.flags & FLAG_ACK) {
        //printf("Connection established successfully\n");
        return 0; // Handshake successful
    }
    //printf("Connection failed\n");
    return -1; // Handshake failed
}

int rudp_close(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen) {
    RudpPacket fin_packet, ack_packet;
    socklen_t* len = &addrlen;

    // Prepare FIN packet
    fin_packet.length = htons(0); // No data in SYN packet
    fin_packet.flags = FLAG_FIN;
    fin_packet.seq_num = ((unsigned int)rand()%256);

    // Send FIN packet
    if (rudp_send(sockfd, &fin_packet, dest_addr, addrlen) < 0) {
        perror("sendto failed");
        return -1;
    }

    printf("Sent FIN %d packet, waiting for ACK.\n",fin_packet.seq_num);

    // Receive ACK packet
    if (rudp_rcv(sockfd, &ack_packet, dest_addr, len) < 0) {
        perror("recvfrom failed");
        return -1;
    }

    printf("Received ACK packet for FIN\n");

    // Check if it's a valid ACK packet
    if (ack_packet.flags & FLAG_ACK) {
        //printf("ACK seqnum: %d\n",ack_packet.seq_num);
        //close(sockfd);
        return 0; // Closing successful
    }
    printf("ack_packet isn't flagged ACK. Seqnum: %d. Flag: %d.\n",ack_packet.seq_num,ack_packet.flags);
    return -1; // Closing failed
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

//#include "rudp.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//
//int rudp_socket() {
//    return socket(AF_INET, SOCK_DGRAM, 0);
//}
//
//int rudp_send(int sockfd, const struct RudpHeader *packet, const struct sockaddr *dest_addr, socklen_t addrlen) {
//    return sendto(sockfd, packet, sizeof(struct RudpPHeader) + ntohs(packet->length), 0, dest_addr, addrlen);
//}
//
//int rudp_recv(int sockfd, struct RUDPHeader *packet, struct sockaddr *src_addr, socklen_t *addrlen) {
//    return recvfrom(sockfd, packet, sizeof(struct RUDPHeader) + MAXLINE, 0, src_addr, addrlen);
//}




//
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <time.h>
//
//#define PORT 8080
//#define BUFFER_SIZE 1024
//#define WINDOW_SIZE 4
//#define TIMEOUT_SEC 1
//
//// Packet structure
//struct Packet {
//    int seq_num;
//    int ack_num;
//    int size;
//    char data[BUFFER_SIZE];
//};
//
//// Function to send a packet
//void send_packet(int sockfd, struct sockaddr_in *receiver_addr, struct Packet *packet) {
//    sendto(sockfd, packet, sizeof(struct Packet), 0, (struct sockaddr *)receiver_addr, sizeof(struct sockaddr_in));
//}
//
//// Function to receive a packet
//void receive_packet(int sockfd, struct sockaddr_in *sender_addr, struct Packet *packet) {
//    socklen_t addr_len = sizeof(struct sockaddr_in);
//    recvfrom(sockfd, packet->data, sizeof(struct Packet), 0, (struct sockaddr *)sender_addr, &addr_len);
//}
//
//// Function to perform handshake (SYN, SYN-ACK, ACK)
//void perform_handshake(int sockfd, struct sockaddr_in *sender_addr, struct sockaddr_in *receiver_addr) {
//    struct Packet syn_packet, syn_ack_packet, ack_packet;
//    syn_packet.seq_num = 0;
//    syn_packet.ack_num = 0;
//    syn_packet.size = 0;
//    memset(syn_packet.data, 0, BUFFER_SIZE);
//    send_packet(sockfd, receiver_addr, &syn_packet);
//    printf("SYN sent\n");
//
//    receive_packet(sockfd, sender_addr, &syn_ack_packet);
//    printf("SYN-ACK received\n");
//
//    ack_packet.seq_num = syn_ack_packet.ack_num;
//    ack_packet.ack_num = syn_ack_packet.seq_num + 1;
//    ack_packet.size = 0;
//    memset(ack_packet.data, 0, BUFFER_SIZE);
//    send_packet(sockfd, receiver_addr, &ack_packet);
//    printf("ACK sent\n");
//}
//
//// Function to perform connection teardown (FIN, FIN-ACK, ACK)
//void perform_teardown(int sockfd, struct sockaddr_in *sender_addr, struct sockaddr_in *receiver_addr) {
//    struct Packet fin_packet, fin_ack_packet, ack_packet;
//    fin_packet.seq_num = 0;
//    fin_packet.ack_num = 0;
//    fin_packet.size = 0;
//    memset(fin_packet.data, 0, BUFFER_SIZE);
//    send_packet(sockfd, receiver_addr, &fin_packet);
//    printf("FIN sent\n");
//
//    receive_packet(sockfd, sender_addr, &fin_ack_packet);
//    printf("FIN-ACK received\n");
//
//    ack_packet.seq_num = fin_ack_packet.ack_num;
//    ack_packet.ack_num = fin_ack_packet.seq_num + 1;
//    ack_packet.size = 0;
//    memset(ack_packet.data, 0, BUFFER_SIZE);
//    send_packet(sockfd, receiver_addr, &ack_packet);
//    printf("ACK sent\n");
//}
//
//// Function to implement Go-Back-N protocol
//void go_back_n(int sockfd, struct sockaddr_in *receiver_addr) {
//    struct Packet packets[WINDOW_SIZE];
//    int base = 0, nextseqnum = 0, n_packets = 0;
//    int congestion_window = 1; // Initial congestion window size
//    int ssthresh = WINDOW_SIZE; // Initial slow start threshold
//    srand(time(NULL));
//
//    while (1) {
//        // Send packets
//        while (n_packets < congestion_window && nextseqnum < 10) {
//            packets[nextseqnum % WINDOW_SIZE].seq_num = nextseqnum;
//            packets[nextseqnum % WINDOW_SIZE].ack_num = 0;
//            packets[nextseqnum % WINDOW_SIZE].size = BUFFER_SIZE;
//            for (int i = 0; i < BUFFER_SIZE; i++)
//                packets[nextseqnum % WINDOW_SIZE].data[i] = rand() % 256;
//
//            send_packet(sockfd, receiver_addr, &packets[nextseqnum % WINDOW_SIZE]);
//            printf("Packet sent: %d\n", nextseqnum);
//            nextseqnum++;
//            n_packets++;
//        }
//
//        // Receive acknowledgments
//        struct Packet ack_packet;
//        while (recvfrom(sockfd, &ack_packet, sizeof(struct Packet), MSG_DONTWAIT, NULL, NULL) > 0) {
//            printf("ACK received: %d\n", ack_packet.ack_num);
//            if (ack_packet.ack_num >= base) {
//                base = ack_packet.ack_num + 1;
//                n_packets--;
//                if (congestion_window < ssthresh) {
//                    congestion_window *= 2; // Slow start phase
//                } else {
//                    congestion_window++; // Congestion avoidance phase
//                }
//            }
//        }
//
//        // Timeout handling and congestion control
//        sleep(TIMEOUT_SEC);
//        ssthresh = congestion_window / 2; // Set slow start threshold
//        congestion_window = 1; // Reset congestion window to 1 after timeout
//    }
//}

