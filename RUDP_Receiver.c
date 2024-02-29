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

void printStats(double times[100], int run) {
    double timeSum = 0;
    double speedSum = 0;
    double avgTime = 0;
    double avgSpeed = 0;
    puts("--------------------------------------------");
    puts("(*) Times and speeds summary:\n");
    for (int i = 1; i < run; i++) {
        timeSum += times[i];
        double speed  = (FILE_SIZE / times[i]) / 1000;
        speedSum += speed;
        printf("(*) Run %d, Time: %0.3lf ms, Speed: %0.3lf MB/s\n", i, times[i], speed);
    }
    if (run > 0) {
        avgTime = (timeSum / (double) (run-1));
        avgSpeed = (speedSum / (double) (run-1));
    }
    printf("\n(*) Overall statistics:\n");
    printf("(*) Number of runs: %d\n", run-1);
    printf("(*) Average RTT: %0.3lf ms\n", avgTime);
    printf("(*) Average throughput: %0.3lf MB/s\n", avgSpeed);
    printf("(*) Total time: %0.3lf MB/s\n", timeSum);
    printf("--------------------------------------------\n");

}


int main(int argc, char *argv[]) {
    double times[MAX_RUNS] = {0};
    int currentRun = 1;

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

    while (1) {
        // Wait for handshake
        int seqnum = rudp_connect(sockfd, &sender_addr, sizeof(sender_addr), 1);
        if (seqnum == -1) {
            perror("connection failed");
            close(sockfd);
            return -1;
        } else if (seqnum == -2) {
            break;
        }
        printf("Connected successfully. seqnum: %d\n", seqnum);

        if (rudp_rcv_file(file, sockfd, receiver_addr, seqnum,times,&currentRun) < 0) {
            perror("receiving failed");
            return -1;
        }

        printf("Finished receiving\n");

        if (rudp_close(sockfd, &sender_addr, sizeof(sender_addr), 1) == -1) {
            perror("close failed");
            return -1;
        }

        puts("Receiver finished successfully");
    }
    printStats(times, currentRun);

    close(sockfd);
    return 0;
}
