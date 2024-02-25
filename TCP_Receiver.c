

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


#define FILE_SIZE 2000000
#define MAX_RUNS 100


void printStats(double times[100], double speeds[100], int run);

int main(int argc, char* argv[]) {
    puts("Starting Receiver...\n");
    if (argc != 5) {
        puts("invalid command");
        return 1;
    }
    int port = 0;
    char algo[10] = {0};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-algo") == 0) {
            strcpy(algo, argv[i + 1]);
        }
    }

    struct sockaddr_in receiverAddress, senderAddress;
    struct timeval start, end;

    memset(&receiverAddress, 0, sizeof(receiverAddress));
    memset(&senderAddress, 0, sizeof(senderAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = INADDR_ANY;
    receiverAddress.sin_port = htons(port);

    int socketfd = -1;
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("socket(2)");
        //free(algo);
        exit(1);
    }
    if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) != 0) {
        perror("setsockopt(2)");
        //free(algo);
        exit(1);
    }
    //free(algo);

    if (bind(socketfd, (struct sockaddr *) &receiverAddress, sizeof(receiverAddress)) == -1) {
        perror("bind(2)");
        exit(1);
    }

    if (listen(socketfd, 1) < 0) {
        perror("listen(2)");
        close(socketfd);
        return 1;
    }
    socklen_t clientAddressLen = sizeof(senderAddress);

    double times[MAX_RUNS];
    double speeds[MAX_RUNS];
    int currentRun = 1;

    puts("Waiting for TCP connection...");


    while (1) {

        int client_sock = accept(socketfd, (struct sockaddr *) &senderAddress, &clientAddressLen);
        if (client_sock < 0) {
            perror("accept(2)");
            close(socketfd);
            return 1;
        }
        puts("Sender connected, beginning to receive file...\n");
        char buffer[FILE_SIZE] = {0};
        int totalRecv = 0;
        while (currentRun < MAX_RUNS) {
            gettimeofday(&start, NULL);
            int bytes_received = recv(client_sock, buffer, FILE_SIZE, 0);
            totalRecv += bytes_received;
            if (buffer[0]=='e'&&buffer[1]=='x'&&buffer[2]=='i'&&buffer[3]=='t' && bytes_received < 10) { // TODO: change
                puts("Sender sent exit message.\n");
                printStats(times, speeds, currentRun);
                close(client_sock);
                close(socketfd);
                exit(1);
            }
            if (totalRecv != FILE_SIZE) {
                continue;
            }
            totalRecv = 0;
            gettimeofday(&end, NULL);
            puts("File transfer completed.\n");

            if (bytes_received < 0) {
                perror("recv(2)");
                close(client_sock);
                close(socketfd);
                return 1;
            }

            times[currentRun] = (end.tv_sec - start.tv_sec) * 1000.0;
            times[currentRun] += (end.tv_usec - start.tv_usec) / 1000.0;
            speeds[currentRun] = (bytes_received / times[currentRun]) / 1000;
            currentRun++;
            puts("Waiting for Sender response...");
        }
    }

    return 0;
}

void printStats(double times[100], double speeds[100], int run) {
    double timeSum = 0;
    double sum1 = 0;
    double avgTime = 0;
    double avgSpeed = 0;
    puts("--------------------------------------------\n");
    puts("(*) Times summary:\n\n");
    for (int i = 1; i < run; i++) {
        timeSum += times[i];
        printf("(*) Run %d, Time: %0.3lf ms\n", i, times[i]);
    }
    printf("\n(*) Speeds summary:\n\n");
    for (int i = 1; i < run; i++) {
        sum1 += speeds[i];
        printf("(*) Run %d, Speed: %0.3lf MB/s\n", i, speeds[i]);
    }
    if (run > 0) {
        avgTime = (timeSum / (double) (run-1));
        avgSpeed = (sum1 / (double) (run-1));
    }
    printf("\n(*) Time avarages:\n");
    printf("(*) Avarage transfer time for whole file: %0.3lf ms\n", avgTime);
    printf("(*) Avarage bandwidth: %0.3lf MB/s\n", avgSpeed);
    printf("--------------------------------------------\n");

}


