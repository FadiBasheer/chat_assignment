#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

void func(int sockfd) {
    char buff[MAX];
    char buff_read[MAX];
    int read_check = 0;
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        bzero(buff_read, sizeof(buff_read));
        n = 0;

        while (1) {
            fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
            int numRead = read(0, buff, sizeof(buff));

            int flags = fcntl(sockfd, F_GETFL, 0);
            fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
            if (read(sockfd, buff_read, sizeof(buff_read)) != -1) {
                printf("From Server: %s\n", buff_read);
            };
            bzero(buff_read, sizeof(buff_read));

            if (numRead > 0) {
                write(sockfd, buff, sizeof(buff));
                bzero(buff, sizeof(buff));
                if ((strncmp(buff, "exit", 4)) == 0) {
                    printf("Client Exit...\n");
                    break;
                }
            }
        }
    }
}

int main() {
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("10.65.98.65");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    } else
        printf("connected to the server..\n");

    // function for chat
    func(sockfd);

    // close the socket
    close(sockfd);
}
