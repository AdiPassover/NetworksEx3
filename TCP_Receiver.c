#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("TCP_Receiver");

    for (int i = 1; i < argc; i++) {
        printf("%s\n",argv[i]);
    }
}