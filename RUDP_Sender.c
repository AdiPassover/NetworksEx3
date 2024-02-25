#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
    if (argc != 5) {
        puts("invalid command");
        return 1;
    }
    int port = 0;
    char IP[20] = {0};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-ip") == 0) {
            strcpy(IP, argv[i + 1]);
        }
    }


}




#include "RUDP_API.h"

int main() {
    int sockfd;
    struct sockaddr_in receiver_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set up receiver address
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(PORT);
    receiver_addr.sin_addr.s_addr = INADDR_ANY;

    // Perform handshake
    perform_handshake(sockfd, NULL, &receiver_addr);

    // Implement Go-Back-N protocol
    go_back_n(sockfd, &receiver_addr);

    // Perform teardown
    perform_teardown(sockfd, NULL, &receiver_addr);

    close(sockfd);
    return 0;
}
