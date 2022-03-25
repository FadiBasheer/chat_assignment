#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define MAX 80
#define PORT 8001
#define SA struct sockaddr

void packi16(unsigned char *buf, unsigned int i) {
    *buf++ = i >> 8;
    *buf++ = i;
}

unsigned int pack(unsigned char *buf, char *format, ...) {
    va_list ap;

    unsigned char C;

    unsigned int H;

    char *s; // strings
    unsigned int len;

    unsigned int size = 0;

    va_start(ap, format);

    for (; *format != '\0'; format++) {
        switch (*format) {

            case 'C': // 8-bit unsigned
                size += 1;
                C = (unsigned char) va_arg(ap, unsigned int); // promoted
                printf("C: %08X\n", C);
                *buf++ = C;
                break;

            case 'H': // 16-bit unsigned
                size += 2;
                H = va_arg(ap, unsigned int);
                packi16(buf, H);
                buf += 2;
                break;

            case 's': // string
                s = va_arg(ap, char*);
                len = strlen(s);
                size += len + 2;
                packi16(buf, len);
                buf += 2;
                memcpy(buf, s, len);
                buf += len;
                break;
        }
    }
    va_end(ap);
    return size;
}


void func(int sockfd) {

    struct Client_Packet {
        uint8_t VER;
        uint8_t CMD_CODE;
        uint16_t CHAN_ID;
        uint16_t MSG_LEN;
        char *MSG;
    } Client_Packet;

    unsigned char buf[1024] = {0};

    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n');

        Client_Packet.VER = 0;
        Client_Packet.CMD_CODE = 5;
        Client_Packet.CHAN_ID = 90;
        Client_Packet.MSG_LEN = n;
        Client_Packet.MSG = buff;

        size_t buff_size = pack(buf, "CCHHs", Client_Packet.VER, Client_Packet.CMD_CODE, Client_Packet.CHAN_ID,
                                Client_Packet.MSG_LEN,
                                Client_Packet.MSG);

        printf("VER %d\n", Client_Packet.VER);
        printf("CMD_CODE %d\n", Client_Packet.CMD_CODE);
        printf("CHAN_ID %d\n", Client_Packet.CHAN_ID);
        printf("MSG_LEN %d\n", Client_Packet.MSG_LEN);
        printf("MSG %s\n", Client_Packet.MSG);
        printf("buf %016X\n", buf);


        send(sockfd, buf, buff_size, 0);

        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
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
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
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
