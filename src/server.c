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
#define BUFSIZE 1024
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
    char *msg;
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
 * Adds a new client node to the client linked-list.
 * @param head_ref reference to client linked-list.
 * @param chan_id channel id of the new client.
 * @param fd file descriptor of the new client.
 */
void push(struct Client **head_ref, int chan_id, int fd);

/**
 * Deletes a specific client from the client linked-list if the channel id
 * and file descriptor matches.
 * @param head_ref reference to client linked-list.
 * @param chan_id channel id of the new client.
 * @param fd_key file descriptor of the new client.
 */
void delete_client(struct Client **head_ref, int chan_id, int fd_key);

/**
 * Prints a list of all currently unique client file descriptor and channel id combination.
 * @param node reference to client linked-list.
 */
void print_client_list(struct Client *node);

/**
 * Takes the client channel_id and fd to creates a new node in the client linked-list.
 * @param node reference to client linked-list.
 * @param ref_node double pointer to client linked-list.
 * @param channel_id channel id of new client.
 * @param fd file descriptor of new client.
 * @return 9 if successful 7 if failure.
 */
int cpt_join_channel_response(void *server_info, struct Client *node, struct Client **ref_node, uint16_t channel_id, int fd);

/**
 * Takes the client channel_id and fd to creates a new node in the client linked-list.
 * @param node reference to client linked-list.
 * @param ref_node double pointer to client linked-list.
 * @param channel_id channel id of new client.
 * @param fd file descriptor of new client.
 * @return 10 if successful 5 if failure.
 */
int cpt_leave_channel_response(void *server_info, struct Client *node, struct Client **ref_node, uint16_t channel_id, int fd) ;

/**
 *
 * @param fd file descriptor of client.
 * @param code status code of response.
 * @param msg_length message length of response.
 * @param msg message from server to client.
 * @param chan_id channel id of client.
 * @return 0 if successful.
 */
int cpt_send_response(int fd, int code, int msg_length, char *msg, int chan_id);

/**
 * Main entry point to the program.
 */
int main(void) {
    int server_fd, client_fd, function_response;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error 1: ");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("Error 2: ");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if ((bind(server_fd, (SA *) &server_addr, sizeof(server_addr))) < 0) {
        perror("Error 3: ");
        if (close(server_fd) < 0) {
            perror("Error 8: ");
        }
        exit(EXIT_FAILURE);
    }

    if ((listen(server_fd, 10)) < 0) {
        perror("Error 4: ");
        if (close(server_fd) < 0) {
            perror("Error 8: ");
        }
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    int max_fd;
    max_fd = server_fd;

    fd_set fd_read_set;
    fd_set fd_accepted_set;

    //Linked-list for channels
    struct Client *client = NULL;
    struct CptResponse response;

    struct cpt cpt;
    char msg_rcv[1024];

    printf("------ Server Online! ------\n");

    while (1) {
        FD_ZERO(&fd_read_set);
        FD_SET(server_fd, &fd_read_set);

        struct timeval timer;
        timer.tv_sec = 0;
        timer.tv_usec = 0;

        if (max_fd < server_fd) {
            max_fd = server_fd;
        }

        fd_accepted_set = fd_read_set;

        int fds_selected;
        fds_selected = select(max_fd + 1, &fd_accepted_set, NULL, NULL, &timer);

        if (fds_selected > 0) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            //accepting new connection
            client_fd = accept(server_fd, (SA *) &client_addr, &client_addr_len);
            if (client_fd < 0) {
                perror("Error 6: ");
                //exit_code = 0;
                if (close(server_fd) < 0) {
                    perror("Error 8: ");
                }
                exit(EXIT_FAILURE);
            }
            printf("Client Connected, ip: %s, port: %d\n", inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));

            if (FD_ISSET(server_fd, &fd_read_set) != 0) {
                char client_connected_msg[] = "Connected to Server!";

                if (send(client_fd, client_connected_msg, strlen(client_connected_msg), 0) !=
                    (ssize_t) strlen(client_connected_msg)) {
                    perror("Error 7: ");
                }

                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                //Add client FD to main channel (0) linked-list
                push(&client, 0, client_fd);

                print_client_list(client);

                FD_SET(client_fd, &fd_read_set);
            }
        }

        struct Client *head_client = client;

        // Find max file descriptor in main channel
        while (head_client != NULL) {
            FD_SET(head_client->fd, &fd_read_set);
            if (head_client->fd > 0 && max_fd < head_client->fd) {
                max_fd = head_client->fd;
            }
            head_client = head_client->next;
        }

        unsigned char client_buffer[BUFSIZE];
        timer.tv_sec = 1;
        timer.tv_usec = 0;

        fds_selected = select(max_fd + 1, &fd_read_set, NULL, NULL, &timer);

        if (fds_selected > 0) {
            head_client = client;
            while (head_client != NULL) {
                if (FD_ISSET(head_client->fd, &fd_read_set) != 0) {
                    memset(client_buffer, 0, BUFSIZE);
                    ssize_t nread = read(head_client->fd, &client_buffer, BUFSIZE);

                    if (nread != 0) {

                        unpack(client_buffer, "CCHCs", &cpt.version, &cpt.command, &cpt.channel_id, &cpt.msg_len,
                               &msg_rcv);
                        cpt.msg = malloc(cpt.msg_len * sizeof(char));
                        strncpy(cpt.msg, msg_rcv, cpt.msg_len);

                        // If command is send
                        if (cpt.command == 1) {
                            struct Client *head_client_write = client;

                            while (head_client_write != NULL) {
//                                printf("%d\n", head_client_write->chan_id);

                                if (head_client_write->chan_id == cpt.channel_id) {
                                    if(head_client_write->fd != head_client->fd)
                                    {
                                        cpt_send_response(head_client_write->fd, 0, cpt.msg_len, cpt.msg, cpt.channel_id);
                                    }
                                }
                                head_client_write = head_client_write->next;
                            }
                        }

                        // join channel
                        if (cpt.command == 5) {
                            function_response = cpt_join_channel_response(NULL, client, &client, cpt.channel_id, client_fd);
                            cpt_send_response(head_client->fd, function_response, 0, "", cpt.channel_id);
                            print_client_list(client);
                        }

                        //LEAVE_CHANNEL
                        if (cpt.command == 6) {
                            function_response = cpt_leave_channel_response(NULL, client, &client,cpt.channel_id, client_fd);
                            cpt_send_response(head_client->fd, function_response, 0, "", cpt.channel_id);
                            print_client_list(client);
                        }
                    }
                }
                head_client = head_client->next;
            }
        }
    }
}

/**
 * Adds a new client node to the client linked-list.
 * @param head_ref reference to client linked-list.
 * @param chan_id channel id of the new client.
 * @param fd file descriptor of the new client.
 */
void push(struct Client **head_ref, int chan_id, int fd) {
//    printf("chan_id: %d, fd:%d\n", chan_id, fd);
    struct Client *new_node = (struct Client *) malloc(sizeof(struct Client));
    new_node->fd = fd;
    new_node->chan_id = chan_id;
    new_node->next = (*head_ref);
    (*head_ref) = new_node;

}

/**
 * Prints a list of all currently unique client file descriptor and channel id combination.
 * @param node reference to client linked-list.
 */
void print_client_list(struct Client *node) {
    printf("------ Clients Currently Connected! ------\n");
    printf("Client list:\n");
    while (node != NULL) {
        printf("chan_id: %d, fd: %d\n", node->chan_id, node->fd);
        node = node->next;
    }
}

/**
 * Takes the client channel_id and fd to creates a new node in the client linked-list.
 * @param node reference to client linked-list.
 * @param ref_node double pointer to client linked-list.
 * @param channel_id channel id of new client.
 * @param fd file descriptor of new client.
 * @return 9 if successful 7 if failure.
 */
int cpt_join_channel_response(void *server_info, struct Client *node, struct Client **ref_node, uint16_t channel_id, int fd) {
    int flag = 0;
//    printf("chan_id: %d, fd: %d\n", channel_id, fd);
    while (node != NULL) {
        if (node->fd == fd && node->chan_id == channel_id) {
            flag = 1;
            printf("Channel already exist\n");
        }
        node = node->next;
    }

    if (flag == 0) {
        push(ref_node, channel_id, fd);
    }
    return 9;
}

/**
 * Deletes a specific client from the client linked-list if the channel id
 * and file descriptor matches.
 * @param head_ref reference to client linked-list.
 * @param chan_id channel id of the new client.
 * @param fd_key file descriptor of the new client.
 */
void delete_client(struct Client** head_ref, int chan_id, int fd_key)
{
    while (*head_ref)
    {
        if ((*head_ref)->fd == fd_key && (*head_ref)->chan_id == chan_id)
        {
            struct Client *tmp = *head_ref;
            *head_ref = (*head_ref)->next;
            free( tmp );
        }
        else
        {
            head_ref = &(*head_ref)->next;
        }
    }
}

int cpt_leave_channel_response(void *server_info, struct Client *node, struct Client **ref_node, uint16_t channel_id, int fd) {
    int flag = 0;
    while (node != NULL) {
        if (node->fd == fd && node->chan_id == channel_id) {
            flag = 1;
            break;
        }
        node = node->next;
    }

    if (flag == 1) {
        delete_client(ref_node, channel_id, fd);
    }

    return 10;
}

int cpt_send_response(int fd, int code, int msg_length, char *msg, int chan_id) {
    unsigned char buf[1024];
    unsigned char data[1024];
    uint8_t res_code;
    uint16_t data_size;
    size_t packet_size;

    struct CptResponse response;
    response.code = code;
    response.data_size = msg_length;
    response.data = msg;

    packet_size = pack(buf, "CHs", (uint8_t) response.code, (uint16_t) chan_id, response.data);

    send(fd, buf, packet_size, 0);

    return 0;
}
