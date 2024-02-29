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

    if (argc != 5) {
        puts("invalid command");
        return 1;
    }
    int port = 0;
    char SERVER_IP[20] = {0};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-ip") == 0) {
            strcpy(SERVER_IP, argv[i + 1]);
        }
    }

    // Create socket
    int sockfd = rudp_socket();
    if (sockfd < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set up receiver address
    struct sockaddr_in receiver_addr;
    //socklen_t rec_len = sizeof(receiver_addr);
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, SERVER_IP, &receiver_addr.sin_addr) <= 0) { // setting the SERVER_IP address
        perror("inet_pton(3)");
        close(sockfd);
        return 1;
    }

    char *file = util_generate_random_data(FILE_SIZE);
    int status = 1;
    while (status == 1) {
        // Perform handshake
        int seqnum = rudp_connect(sockfd, &receiver_addr, sizeof(receiver_addr), 0);
        if (seqnum < 0) {
            perror("connection failed");
            return -1;
        }
        puts("Connected successfully");

        // Send data
        if (rudp_send_file(file, sockfd, receiver_addr, seqnum) < 0) {
            perror("send file failed");
            return -1;
        }

        puts("Finished sending data. Closing connection");
        //finish sending, and receive the last ack, close the rudp connection
        //packet->flags = FLAG_FIN;
        if (rudp_close(sockfd, &receiver_addr, sizeof(receiver_addr), 0) == -1) {
            perror("close failed");

            return -1;
        }

        puts("Sender finished successfully");


        puts("would you like to send the file again? (0 to exit, 1 to resend)");
        scanf("%d", &status);
    }
    close(sockfd);
    //to print stats
    return 0;
}
