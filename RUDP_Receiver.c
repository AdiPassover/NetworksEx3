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
    char file[FILE_SIZE] = {0};
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
    receiver_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
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
    int seqnum = rudp_connect(sockfd, &sender_addr, sizeof(sender_addr),1);
    if (seqnum < 0) {
        perror("connection failed");
        close(sockfd);
        return -1;
    }
    printf("Connected successfully. seqnum: %d\n",seqnum);

    if (rudp_rcv_file(file,sockfd,receiver_addr,seqnum) < 0) {
        perror("receiving failed");
        return -1;
    }

    printf("Finished receiving\n");
    for (int i = 0; i < 50; i++) {
        putc(file[i],stdout);
    }

    if (rudp_close(sockfd, &sender_addr, sizeof(sender_addr),1)==-1) {
        perror("close failed");
        return -1;
    }


    puts("Receiver finished successfully");
    close(sockfd);
    return 0;
}
