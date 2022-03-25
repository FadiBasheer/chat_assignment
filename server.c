
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <pthread.h>

#define TRUE   1
#define FALSE  0
#define PORT 8080

void *private_chat(void *arg);

int client_socket[30], max_clients = 30;
pthread_t thread_1;

int main(int argc, char *argv[]) {
    int opt = TRUE;
    int master_socket, addrlen, new_socket, private_sockets[2], activity, i, valread, sd, p = 0;
    int max_sd;
    struct sockaddr_in address;

    struct Client_Packet {
        uint8_t VER;
        uint8_t CMD_CODE;
        uint16_t CHAN_ID;
        uint16_t MSG_LEN;
        uint8_t *MSG;
    } Client_Packet;

    private_sockets[0] = 0;
    private_sockets[1] = 0;

    char buffer[1025];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //a message
    char *message = "ECHO Daemon v1.0 \r\n";

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

    //create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("10.65.98.65");
    // address.sin_addr.s_addr = inet_addr("localhost");

    address.sin_port = htons(PORT);

    //bind the socket to localhost port 8080
    if (bind(master_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }



    /////////////////////////////////////////////////////////////////////////////////////////

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (TRUE) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
        printf("add master socket to set\n");


        //add child sockets to set
        for (i = 0; i < max_clients; i++) {
            //socket descriptor
            sd = client_socket[i];
            // printf("Add child sockets to set: %d\n", i);

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        //If something happened on the master socket ,then it's an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket,
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //send new connection greeting message
            if (send(new_socket, message, strlen(message), 0) != strlen(message)) {
                perror("send");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                //if position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        //else it's some IO operation on some other socket :)
        printf("else it's some IO operation on some other socket :)\n");
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr),
                           ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                }

                    //Echo back the message that came in
                else {
                    //set the string terminating NULL byte on the end of the data read
                    printf("public chat: %s", buffer);

                    if (strcmp(buffer, "join\n") == 0) {
                        printf("thread\n");
                        private_sockets[p] = client_socket[i];
                        p++;
                        client_socket[i] = 0;
                        send(sd, buffer, strlen(buffer), 0);

                        if (pthread_create(&thread_1, NULL, &private_chat, private_sockets) != 0) {
                            perror("Failed to create thread");
                        }
                        //pthread_join(thread_1, NULL);
                    } else {
                        for (i = 0; i < max_clients; i++) {
                            sd = client_socket[i];
                            send(sd, buffer, strlen(buffer), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

void *private_chat(void *arg) {
    int *private_sockets;
    private_sockets = arg;


    fd_set readfds_private;
    int sd, activity, valread, max_sd = 0;
    char buffer[1025];
    FD_ZERO(&readfds_private);


    for (int i = 0; i < 2; i++) {
        //socket descriptor
        printf("i: %d\n", private_sockets[i]);
        sd = private_sockets[i];
        // printf("Add child sockets to set: %d\n", i);

        //if valid socket descriptor then add to read list
        if (sd > 0) {
            FD_SET(sd, &readfds_private);
            printf("sd %d\n", sd);
        }
        if (sd > max_sd)
            max_sd = sd;
    }

    while (TRUE) {
        activity = select(max_sd + 1, &readfds_private, NULL, NULL, NULL);
        printf("after select\n");

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        for (int i = 0; i < 2; i++) {
            sd = private_sockets[i];

            if (FD_ISSET(sd, &readfds_private)) {
                valread = read(sd, buffer, 1024);
                buffer[valread] = '\0';
                if (strcmp(buffer, "leave\n") == 0) {
                    printf("leave\n");
                    for (int p = 0; p < max_clients; p++) {
                        //if position is empty
                        if (client_socket[p] == 0) {
                            client_socket[p] = private_sockets[i];
                            printf("Adding to list of sockets as %d\n", i);
                            break;
                        }
                    }
                    send(sd, buffer, strlen(buffer), 0);
                    private_sockets[i] = 0;

                    int flag = 0;
                    for (int e = 0; e < 2; e++) {
                        printf("private_sockets[e]: %d\n", private_sockets[e]);
                        if (private_sockets[e] > 0) {
                            flag = 1;
                        }
                    }
                    printf("flag: %d\n", flag);

                    if (flag == 0) {
                        printf("cancel\n");
                        pthread_cancel(thread_1);
                        //pthread_join(thread_1, NULL);
                        return 0;
                    }
                    printf("flag: %d\n", flag);

                } else {
                    for (i = 0; i < 2; i++) {
                        sd = private_sockets[i];
                        send(sd, buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }
    return 0;
}