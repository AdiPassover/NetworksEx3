#include <stdio.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "stdlib.h"
#include <sys/socket.h>
#include <unistd.h>
#include "RUDP_API.h"

#define FILE_SIZE 2000000

/*
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
    // Argument check.
    if (size == 0)
        return NULL;
    buffer = (char *) calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL)
        return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int) rand() % 256);
    return buffer;
}
int main(int argc, char *argv[]) {
    puts("Starting Sender...\n");

    if (argc != 5)
    {
        puts("invalid command");
        return 1;
    }
    int port = 0;
    char SERVER_IP[20] = {0};
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-ip") == 0)
        {
            strcpy(SERVER_IP, argv[i + 1]);
        }
    }

    // Create socket
    int sockfd = rudp_socket();
    if (sockfd < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set up receiver address
    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, SERVER_IP, &receiver_addr.sin_addr) <= 0) { // setting the SERVER_IP address
        perror("inet_pton(3)");
        close(sockfd);
        return 1;
    }

    char* file = util_generate_random_data(FILE_SIZE);

//    struct timeval tv = {5,0};
//    if (setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(tv)) == -1) { // checking the server is online
//        puts("server is offline");
//        perror("setsockopt(2)");
//        close(sockfd);
//        return 1;
//    }
//    puts("server is online");

    // Perform handshake
    int seqnum = rudp_connect(sockfd, &receiver_addr, sizeof(receiver_addr));
    if (seqnum < 0) {
        perror("connection failed");
        return -1;
    }
    puts("Connected successfully");
    // Send data 
    RudpPacket *packet = (RudpPacket *)malloc(sizeof(RudpPacket));
    packet->seq_num = seqnum;
    packet->flags = 0;
    packet->checksum = 1024;
    packet->length = 1024;
    u_int8_t acked = 0;
    socklen_t len = sizeof(receiver_addr); // saving the length we don't cate about
    for(int i = 0; i < FILE_SIZE; i+=1024){
        memcpy(packet->data, file+i, 1024);
        if(rudp_send(sockfd, (const RudpPacket *) &packet, &receiver_addr, sizeof(receiver_addr)) == -1){
            perror("sendto failed");
            free(packet);
            return -1;
        }
        printf("Sent packet %d\n", i/1024);
        while (!acked) {
            int bytes_received = rudp_rcv(sockfd, packet, &receiver_addr, &len);
            if (bytes_received < 0) {
                perror("recvfrom failed");
                free(packet);
                return -1;
            }
            if ((packet->flags & FLAG_ACK) && (packet->seq_num == ++seqnum)) {
                acked = 1;
                printf("Received ACK for packet %d\n", i/1024);
            }
        }
        acked = 0;
    }
    //finish sending, and recieve the last ack, close the rudp connection
    //packet->flags = FLAG_FIN;
    if(rudp_close(sockfd, &receiver_addr, sizeof(receiver_addr))==-1){
        perror("close failed");
        free(packet);
        return -1;
    }

    free(packet);
    return 0;
}
