#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include "serialization.c"

#define SERVER_PORT 8000
#define SA struct sockaddr

/**
 * Client linked-list, data structure to manage clients connecting to the server
 * in different channels.
 */
struct Client {
    int fd;
    int chan_id;
    struct Client *next;
};

/**
 * Chat protocol client request packet specified by COMP4981 chat protocol.
 */
struct cpt {
    uint8_t version;
    uint8_t command;
    uint16_t channel_id;
    uint8_t msg_len;
    char msg[1024];
};

/**
 * Chat protocol server response specified by COMP4981 chat protocol.
 */
struct CptResponse {
    uint8_t code;
    uint16_t data_size;
    char *data;
};

/**
 * Struct for values to client request command number.
 */
struct commands_client {
    int send;
    int join_chan;
    int leave_chan;
};

/**
 * Struct for specifying the version of the chat protocol.
 */
struct version {
    int ver_num;
};

/**
 * Prints a menu for the user to interact with.
 */
static void print_menu(void);

/**
 * Main entry point to the program.
 */
int main(void) {
    int socket_fd;
    struct sockaddr_in server_addr;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error 1: ");
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("Error 2: ");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(socket_fd, (SA *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error 3: ");
    }

    ssize_t nread;
    ssize_t nread_all_clients;
    char data[1024];
    char server_msg_buf[1024];
    char all_clients_buffer[1024];

    memset(server_msg_buf, 0, 1024);
    recv(socket_fd, server_msg_buf, 21, 0);
    printf("%s\n", server_msg_buf);
    print_menu();

    int max_fd;
    max_fd = socket_fd;
    fd_set fd_read_set;
    fd_set fd_read_accepted_set;

    struct timeval timer;
    timer.tv_sec = 1;
    timer.tv_usec = 0;

    char msg[1024];
    size_t len;

    char chan[5];
    long chan_id;

    struct cpt cpt;
    cpt.channel_id = 0;

    unsigned char buf[1024];
    size_t packet_size;

    struct commands_client cmd;
    cmd.send = 1;
    cmd.join_chan = 5;
    cmd.leave_chan = 6;

    struct version version;
    version.ver_num = 1;

    unsigned char all_client_data[1024];
    uint8_t res_code;
    uint16_t data_size;
    uint16_t channel_id;

    long current_channel = 0;

    int flags = fcntl(socket_fd, F_GETFL);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    fcntl(1, F_SETFL, flags | O_NONBLOCK);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);

    int exit_code = 1;
    while (exit_code) {
        FD_ZERO(&fd_read_set);
        FD_SET(socket_fd, &fd_read_set);
        FD_SET(1, &fd_read_set);

        if (max_fd < socket_fd) {
            max_fd = socket_fd;
        }

        fd_read_accepted_set = fd_read_set;

        int fds_selected;
        fds_selected = select(max_fd + 1, &fd_read_accepted_set, NULL, NULL, &timer);

        //read from server
        if (fds_selected > 0) {
            if (FD_ISSET(socket_fd, &fd_read_set) != 0) {
                nread_all_clients = read(socket_fd, all_clients_buffer, 1024);

                if (nread_all_clients > 0) {
                    unpack(all_clients_buffer, "CHs", &res_code, &channel_id, &all_client_data);
                    if (res_code == 0 && current_channel == channel_id)
                    {
                        printf("From other clients: %s\n", all_client_data);
                    }
                }
            }

            //read from keyboard
            if (FD_ISSET(1, &fd_read_set) != 0) {
                memset(msg, 0, 1024);
                memset(cpt.msg, 0, 1024);
                memset(buf, 0, 1024);
                nread = read(STDOUT_FILENO, data, 1024);
                if (nread > 0) {
                    ssize_t i;
                    for (i = 0; i < nread; i++) {
                        msg[i] = data[i];
                    }
                    msg[nread] = '\0';

                    if (msg[0] == 'j' && msg[1] == 'o' && msg[2] == 'i'
                        && msg[3] == 'n' && msg[4] == ' ' && msg[5] != ' ') {
                        cpt.version = (uint8_t) version.ver_num;
                        cpt.command = (uint8_t) cmd.join_chan;
                        cpt.msg_len = 0;
                        cpt.msg[0] = ' ';

                        chan[0] = msg[5];
                        chan[1] = msg[6];
                        chan[2] = msg[7];
                        chan[3] = msg[8];
                        chan[4] = msg[9];

                        chan_id = strtol(chan, NULL, 10);
                        cpt.channel_id = (uint16_t) chan_id;
                        current_channel = chan_id;
                        packet_size = pack(buf, "CCHCs", (uint8_t) cpt.version, (uint8_t) cpt.command,
                                           (uint16_t) cpt.channel_id, (uint8_t) cpt.msg_len, cpt.msg);
                        send(socket_fd, buf, packet_size, 0);
                    } else if (msg[0] == 'l' && msg[1] == 'e' && msg[2] == 'a'
                               && msg[3] == 'v' && msg[4] == 'e' && msg[5] == ' ' && msg[6] != ' ') {
                        cpt.version = (uint8_t) version.ver_num;
                        cpt.command = (uint8_t) cmd.leave_chan;
                        cpt.msg_len = 0;
                        cpt.msg[0] = ' ';

                        chan[0] = msg[6];
                        chan[1] = msg[7];
                        chan[2] = msg[8];
                        chan[3] = msg[9];
                        chan[4] = msg[10];

                        chan_id = strtol(chan, NULL, 10);
                        cpt.channel_id = (uint16_t) chan_id;
                        packet_size = pack(buf, "CCHCs", (uint8_t) cpt.version, (uint8_t) cpt.command,
                                           (uint16_t) cpt.channel_id, (uint8_t) cpt.msg_len, cpt.msg);
                        send(socket_fd, buf, packet_size, 0);
                        cpt.channel_id = 0;
                        current_channel = 0;
                    } else if (msg[0] == 'm' && msg[1] == 'e' && msg[2] == 'n' && msg[3] == 'u') {
                        print_menu();
                        printf("\n");
                    } else if (msg[0] == 'e' && msg[1] == 'x' && msg[2] == 'i' && msg[3] == 't') {
                        exit_code = 0;
                        printf("GoodBye!\n");
                    } else {
                        cpt.version = (uint8_t) version.ver_num;
                        cpt.command = (uint8_t) cmd.send;
                        cpt.msg_len = (uint8_t) nread;

                        strncpy(cpt.msg, msg, (size_t) nread);

                        packet_size = pack(buf, "CCHCs", (uint8_t) cpt.version, (uint8_t) cpt.command,
                                           (uint16_t) cpt.channel_id, (uint8_t) cpt.msg_len, cpt.msg);

                        send(socket_fd, buf, packet_size, 0);
                    }
                }
            }
        }
    }
}

/**
 * Prints a menu for the user to interact with.
 */
static void print_menu() {
    printf("Welcome to Global Channel 0\n");
    printf("Current Channel: 0\n\n");

    printf("########## Chat Commands ##########\n");
    printf("1) Send Message to Current Channel (eg. Hello, World!)\n");
    printf("2) Join Channel (eg. join 3)\n");
    printf("3) Leave Channel (eg. leave 3)\n");
    printf("4) Print Menu Options (eg. menu)\n");
    printf("5) Exit Chat (eg. exit)\n\n");
}
