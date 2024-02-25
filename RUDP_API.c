#include "RUDP_API.h"

int rudp_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

ssize_t rudp_send(int socketfd, const void *buffer, size_t msgSize, int flags) {
    return send(socketfd,buffer,msgSize,flags);
}

ssize_t rudp_rcv(int socketfd, void *buffer, size_t msgSize, int flags) {
    return recv(socketfd,buffer,msgSize,flags);
}

void rudp_close(int socket) {
    close(socket);
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






#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define WINDOW_SIZE 4
#define TIMEOUT_SEC 1

// Packet structure
struct Packet {
    int seq_num;
    int ack_num;
    int size;
    char data[BUFFER_SIZE];
};

// Function to send a packet
void send_packet(int sockfd, struct sockaddr_in *receiver_addr, struct Packet *packet) {
    sendto(sockfd, packet, sizeof(struct Packet), 0, (struct sockaddr *)receiver_addr, sizeof(struct sockaddr_in));
}

// Function to receive a packet
void receive_packet(int sockfd, struct sockaddr_in *sender_addr, struct Packet *packet) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    recvfrom(sockfd, packet->data, sizeof(struct Packet), 0, (struct sockaddr *)sender_addr, &addr_len);
}

// Function to perform handshake (SYN, SYN-ACK, ACK)
void perform_handshake(int sockfd, struct sockaddr_in *sender_addr, struct sockaddr_in *receiver_addr) {
    struct Packet syn_packet, syn_ack_packet, ack_packet;
    syn_packet.seq_num = 0;
    syn_packet.ack_num = 0;
    syn_packet.size = 0;
    memset(syn_packet.data, 0, BUFFER_SIZE);
    send_packet(sockfd, receiver_addr, &syn_packet);
    printf("SYN sent\n");

    receive_packet(sockfd, sender_addr, &syn_ack_packet);
    printf("SYN-ACK received\n");

    ack_packet.seq_num = syn_ack_packet.ack_num;
    ack_packet.ack_num = syn_ack_packet.seq_num + 1;
    ack_packet.size = 0;
    memset(ack_packet.data, 0, BUFFER_SIZE);
    send_packet(sockfd, receiver_addr, &ack_packet);
    printf("ACK sent\n");
}

// Function to perform connection teardown (FIN, FIN-ACK, ACK)
void perform_teardown(int sockfd, struct sockaddr_in *sender_addr, struct sockaddr_in *receiver_addr) {
    struct Packet fin_packet, fin_ack_packet, ack_packet;
    fin_packet.seq_num = 0;
    fin_packet.ack_num = 0;
    fin_packet.size = 0;
    memset(fin_packet.data, 0, BUFFER_SIZE);
    send_packet(sockfd, receiver_addr, &fin_packet);
    printf("FIN sent\n");

    receive_packet(sockfd, sender_addr, &fin_ack_packet);
    printf("FIN-ACK received\n");

    ack_packet.seq_num = fin_ack_packet.ack_num;
    ack_packet.ack_num = fin_ack_packet.seq_num + 1;
    ack_packet.size = 0;
    memset(ack_packet.data, 0, BUFFER_SIZE);
    send_packet(sockfd, receiver_addr, &ack_packet);
    printf("ACK sent\n");
}

// Function to implement Go-Back-N protocol
void go_back_n(int sockfd, struct sockaddr_in *receiver_addr) {
    struct Packet packets[WINDOW_SIZE];
    int base = 0, nextseqnum = 0, n_packets = 0;
    int congestion_window = 1; // Initial congestion window size
    int ssthresh = WINDOW_SIZE; // Initial slow start threshold
    srand(time(NULL));

    while (1) {
        // Send packets
        while (n_packets < congestion_window && nextseqnum < 10) {
            packets[nextseqnum % WINDOW_SIZE].seq_num = nextseqnum;
            packets[nextseqnum % WINDOW_SIZE].ack_num = 0;
            packets[nextseqnum % WINDOW_SIZE].size = BUFFER_SIZE;
            for (int i = 0; i < BUFFER_SIZE; i++)
                packets[nextseqnum % WINDOW_SIZE].data[i] = rand() % 256;

            send_packet(sockfd, receiver_addr, &packets[nextseqnum % WINDOW_SIZE]);
            printf("Packet sent: %d\n", nextseqnum);
            nextseqnum++;
            n_packets++;
        }

        // Receive acknowledgments
        struct Packet ack_packet;
        while (recvfrom(sockfd, &ack_packet, sizeof(struct Packet), MSG_DONTWAIT, NULL, NULL) > 0) {
            printf("ACK received: %d\n", ack_packet.ack_num);
            if (ack_packet.ack_num >= base) {
                base = ack_packet.ack_num + 1;
                n_packets--;
                if (congestion_window < ssthresh) {
                    congestion_window *= 2; // Slow start phase
                } else {
                    congestion_window++; // Congestion avoidance phase
                }
            }
        }

        // Timeout handling and congestion control
        sleep(TIMEOUT_SEC);
        ssthresh = congestion_window / 2; // Set slow start threshold
        congestion_window = 1; // Reset congestion window to 1 after timeout
    }
}

