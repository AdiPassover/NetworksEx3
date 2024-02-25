#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        puts("invalid command");
        return 1;
    }
    int port = atoi(argv[2]);


}








#include "api.h"

int main() {
    int sockfd;
    struct sockaddr_in receiver_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = INADDR_ANY;
    receiver_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Wait for handshake
    struct sockaddr_in sender_addr;
    perform_handshake(sockfd, &sender_addr, &receiver_addr);

    // Implement Go-Back-N protocol
    go_back_n(sockfd, &sender_addr);

    // Perform teardown
    perform_teardown(sockfd, &sender_addr, &receiver_addr);

    close(sockfd);
    return 0;
}
