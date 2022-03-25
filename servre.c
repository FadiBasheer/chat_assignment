#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

#define MAX 80
#define PORT 8001
#define SA struct sockaddr

/*
** unpacku16() -- unpack a 16-bit unsigned from a char buffer (like ntohs())
*/
unsigned int unpacku16(unsigned char *buf) {
    return ((unsigned int) buf[0] << 8) | buf[1];
}

void unpack(unsigned char *buf, char *format, ...) {
    va_list ap;

    signed char *c; // 8-bit
    unsigned char *C;

    int *h; // 16-bit
    unsigned int *H;

    long int *l; // 32-bit
    unsigned long int *L;

    long long int *q; // 64-bit
    unsigned long long int *Q;

    float *f; // floats
    double *d;
    long double *g;
    unsigned long long int fhold;

    char *s;
    unsigned int len, maxstrlen = 0, count;

    va_start(ap, format);

    for (; *format != '\0'; format++) {
        switch (*format) {

            case 'C': // 8-bit unsigned
                C = va_arg(ap, unsigned char*);
                *C = *buf++;
                break;

            case 'H': // 16-bit unsigned
                H = va_arg(ap, unsigned int*);
                *H = unpacku16(buf);
                buf += 2;
                break;

            case 's': // string
                s = va_arg(ap, char*);
                len = unpacku16(buf);
                buf += 2;
                if (maxstrlen > 0 && len >= maxstrlen) count = maxstrlen - 1;
                else count = len;
                memcpy(s, buf, count);
                s[count] = '\0';
                buf += len;
                break;

            default:
                if (isdigit(*format)) { // track max str len
                    maxstrlen = maxstrlen * 10 + (*format - '0');
                }
        }
        if (!isdigit(*format)) maxstrlen = 0;
    }
    va_end(ap);
}


// Function designed for chat between client and server.
void func(int connfd) {
    unsigned char buf[1024];
    int8_t magic;
    int16_t monkeycount;
    char s2[96];
    int16_t packetsize, ps2;


    struct Client_Packet {
        uint8_t VER;
        uint8_t CMD_CODE;
        uint16_t CHAN_ID;
        uint16_t MSG_LEN;
        char *MSG;
    } Client_Packet;

    struct Client_Packet cp;

    char buff[MAX];
    int n;
    // infinite loop for chat
    for (;;) {
        bzero(buf, MAX);

        // read the message from client and copy it in buffer
        recv(connfd, buf, 1024, 0);

        unpack(buf, "CCHH1024s", Client_Packet.VER, Client_Packet.CMD_CODE, Client_Packet.CHAN_ID,
               Client_Packet.MSG_LEN,
               Client_Packet.MSG);

        printf("VER %d\n", Client_Packet.VER);
        printf("CMD_CODE %d\n", Client_Packet.CMD_CODE);
        printf("CHAN_ID %d\n", Client_Packet.CHAN_ID);
        printf("MSG_LEN %d\n", Client_Packet.MSG_LEN);
        printf("MSG %s\n", Client_Packet.MSG);

        // print buffer which contains the client contents
        printf("From client: %s\t To client : ", buf);
        bzero(buff, MAX);
        n = 0;
        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n');

        // and send that buffer to client
        write(connfd, buff, 1024);

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}

// Driver function
int main() {
    int sockfd, connfd, len;
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    } else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    } else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *) &cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    } else
        printf("server accept the client...\n");

    // Function for chatting between client and server
    func(connfd);

    // After chatting close the socket
    close(sockfd);
}
