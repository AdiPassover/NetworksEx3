#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "RUDP_API.h"

#define FILE_SIZE 2000000

int main(int argc, char *argv[]) {
    puts("Starting Receiver...\n");
    if (argc != 3)
    {
        puts("invalid command");
        return 1;
    }
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));

    // Create socket
    sockfd = rudp_socket();
    if (sockfd < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = inet_addr("127.0.0.1");;
    receiver_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    puts("Waiting for RUDP connection...");

    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    memset(&sender_addr, 0, sender_addr_len);
    // Wait for handshake
    int seqnum = rudp_accept(sockfd, &sender_addr, sizeof(sender_addr));
    printf("seqnum: %d\n",seqnum);
    if (seqnum < 0) {
        perror("connection failed");
        close(sockfd);
        return -1;
    }
    RudpPacket *packet = (RudpPacket *)malloc(sizeof(RudpPacket));
    packet->seq_num = seqnum;
    packet->flags = 0;
    packet->checksum = 1024;
    packet->length = 1024;
    //u_int8_t acked = 0;
    socklen_t *len = NULL; // saving the length we don't cate about
    for(int i = 0; i < FILE_SIZE; i+=1024){
        if(i+1024 > FILE_SIZE){
            packet->length = FILE_SIZE - i;
        }
        rudp_rcv(sockfd, packet, &sender_addr, len);
        if (packet->seq_num != seqnum || calculate_checksum(packet->data, packet->length) != packet->checksum) //if the packet is not the expected one or the checksum is not correct, ask for retransmission
        { 
           rudp_send(sockfd, packet, &sender_addr, sizeof(sender_addr));
        }
        else //if the packet is the expected one and the checksum is correct, send an ack, and update the sequence number for the next packet
        {
            //acked = 1;
            packet->flags = FLAG_ACK;
            packet->seq_num = ++seqnum;
            rudp_send(sockfd, packet, &sender_addr, sizeof(sender_addr));
        }
    }
    //finish recieveing, and send the last ack, close the rudp connection
    //packet->flags = FLAG_FIN;
    if(rudp_close(sockfd, &receiver_addr, sizeof(receiver_addr))==-1){
        perror("close failed");
        free(packet);
        return -1;
    }
    free(packet);

    
    return 0;
}
