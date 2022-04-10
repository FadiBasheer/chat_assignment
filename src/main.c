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

enum command {
    SEND = 1, LOGOUT = 2, GET_USERS = 3, CREATE_CHANNEL = 4, JOIN_CHANNEL = 5, LEAVE_CHANNEL = 6, LOGIN = 7, FAILED = 22
};
enum res_code {
    SUCCESS = 1, MESSAGE, USER_CONNECTED, USER_DISCONNECTED, MESSAGE_FAILED, CHANNEL_CREATED,
    CHANNEL_CREATION_ERROR, CHANNEL_DESTROYED, USER_JOINED_CHANNEL, USER_LEFT_CHANNEL, USER_LIST,
    UNKNOWN_CMD, LOGIN_FAILED, UNKNOWN_CHANNEL, BAD_VERSION, SEND_FAILED, CHAN_ID_OVERFLOW,
    MSG_OVERFLOW, MSG_LEN_OVERFLOW, CHAN_EMPTY, INVALID_ID, FAILURE, UNAUTH_ACCESS, SERVERFULL
};

uint16_t channelId = 0;
/**
 * Client linked-list, data structure to manage clients connecting to the server
 * in different channels.
 */
struct Client {
    int fd;
    char *name;
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
void push(struct Client **head_ref, int chan_id, int fd, char *name);

int cpt_login_response(struct Client *node, struct Client **head, int fd, char *name);

int cpt_create_channel_response(struct Client *node, struct Client **ref_node, uint16_t channel_id, int msg_len,
                                int fd, const char *message, char *name);

int get_uesrs_list(struct Client *node, uint16_t channel_id, uint8_t *msg_len, char **msg);

int cpt_logout_response(struct Client **head, int fd);

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
int cpt_join_channel_response(struct Client *node, struct Client **ref_node, uint16_t channel_id, int fd,
                              char *name);

/**
 * Takes the client channel_id and fd to creates a new node in the client linked-list.
 * @param node reference to client linked-list.
 * @param ref_node double pointer to client linked-list.
 * @param channel_id channel id of new client.
 * @param fd file descriptor of new client.
 * @return 10 if successful 5 if failure.
 */
int cpt_leave_channel_response(struct Client *node, struct Client **ref_node,
                               uint16_t channel_id, int fd);

/**
 *
 * @param fd file descriptor of client.
 * @param code status code of response.
 * @param msg_length message length of response.
 * @param msg message from server to client.
 * @param chan_id channel id of client.
 * @return 0 if successful.
 */
int cpt_send_response(int fd, int code, int msg_length, char *msg);

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
               // push(&client, 0, client_fd, "");

                //add the fd to fd_set for select.
                if (client_fd > max_fd) {
                    max_fd = client_fd;
                    printf("max_fd: %d\n", max_fd);
                }

                FD_SET(client_fd, &fd_read_set);
                // printf("client_fd: %d\n", client_fd);

            }
        }

        struct Client *head_client = client;

        // Find max file descriptor in main channel
//        while (head_client != NULL) {
//            FD_SET(head_client->fd, &fd_read_set);
//
//            printf("\n\nclient_fd: %d\n\n\n", head_client->fd);
//
//            if (head_client->fd > 0 && max_fd < head_client->fd) {
//                max_fd = head_client->fd;
//            }
//            head_client = head_client->next;
//        }

        unsigned char client_buffer[BUFSIZE];
        timer.tv_sec = 1;
        timer.tv_usec = 0;

        // max_fd = 4;
        // printf("client_fd: %d\n", max_fd);

        FD_SET(max_fd, &fd_read_set);

        fds_selected = select(max_fd + 1, &fd_read_set, NULL, NULL, &timer);

        //printf("fds_selected: %d\n", fds_selected);


        if (fds_selected > 0) {
            head_client = client;
            int temp = 0;
            //while (head_client != NULL) {
            //head_client->fd=temp
            while (temp != (max_fd + 1)) {

                if (FD_ISSET(temp, &fd_read_set) != 0) {
                    memset(client_buffer, 0, BUFSIZE);
                    ssize_t nread = read(temp, &client_buffer, BUFSIZE);

                    if (nread != 0) {

                        unpack(client_buffer, "CCHCs", &cpt.version, &cpt.command, &cpt.channel_id, &cpt.msg_len,
                               &msg_rcv);
                        printf("unpack: %u %u %u %u %s\n", cpt.version, cpt.command, cpt.channel_id, cpt.msg_len,
                               msg_rcv);

                        // If command is send
                        if (cpt.command == 1) {

                            //Adding "From channel <channel_id>" to the message.
                            char buf[255];
                            snprintf(buf, sizeof(buf), "%s %d: ", "From channel", cpt.channel_id);
                            cpt.msg_len += strlen(buf);
                            cpt.msg = malloc(cpt.msg_len * sizeof(char));
                            strncpy(cpt.msg, buf, strlen(buf));
                            strcat(cpt.msg, msg_rcv);

                            struct Client *head_client_write = client;

                            while (head_client_write != NULL) {
                                if (head_client_write->chan_id == cpt.channel_id &&
                                    head_client_write->fd != temp) {
                                    cpt_send_response(head_client_write->fd, SEND, cpt.msg_len, cpt.msg);
                                }
                                head_client_write = head_client_write->next;
                            }
                        } else {
                            cpt.msg = malloc(cpt.msg_len * sizeof(char));
                            strncpy(cpt.msg, msg_rcv, cpt.msg_len);
                        }

                        // logout
                        if (cpt.command == 2) {
                            function_response = cpt_logout_response(&client, client_fd);
                            cpt_send_response(temp, function_response, 0, "");
                            print_client_list(client);
                        }

                        //get users
                        if (cpt.command == 3) {
                            function_response = get_uesrs_list(client, cpt.channel_id, &cpt.msg_len, &cpt.msg);
                            cpt_send_response(temp, function_response, cpt.msg_len, cpt.msg);
                            print_client_list(client);
                        }

                        // create channel
                        if (cpt.command == 4) {
                            channelId++;
                            function_response = cpt_create_channel_response(client, &client, channelId,
                                                                            cpt.msg_len,
                                                                            client_fd, cpt.msg, client->name);
                            cpt_send_response(temp, function_response, 0, "");
                            print_client_list(client);
                        }

                        // join channel
                        if (cpt.command == 5) {
                            function_response = cpt_join_channel_response(client, &client, cpt.channel_id,
                                                                          client_fd, client->name);
                            cpt_send_response(temp, function_response, 0, "");
                            print_client_list(client);
                        }

                        //LEAVE_CHANNEL
                        if (cpt.command == 6) {
                            function_response = cpt_leave_channel_response(client, &client, cpt.channel_id,
                                                                           client_fd);
                            cpt_send_response(temp, function_response, 0, "");
                            print_client_list(client);
                        }

                        // login
                        if (cpt.command == 7) {
                            function_response = cpt_login_response(client, &client, client_fd, cpt.msg);
                            cpt_send_response(temp, function_response, 0, "");
                            print_client_list(client);
                        }
                    }
                }
                // head_client = head_client->next;
                temp++;
            }
        }
    }
}


//int main() {
//    struct cpt cpt;
//    char msg_rcv[] = "hello";
//
//    struct Client *client2 = NULL;
//
//    push(&client2, 0, 1, "fadi");
//    push(&client2, 0, 2, "hey");
//    //print_client_list(client2);
//    cpt_login_response(client2, &client2, 1, "fadi");
//    cpt_login_response(client2, &client2, 3, "mina");
//
//    printf("\n---------- Normal print -------------\n");
//    print_client_list(client2);
//
//    printf("\n---------- After Join -------------\n");
//    cpt_join_channel_response(client2, &client2, 5, 1, "fadi");
//    cpt_join_channel_response(client2, &client2, 0, 5, "Jon");
//    print_client_list(client2);
////
////    get_uesrs_list(client2, cpt.channel_id, &cpt.msg_len, &cpt.msg);
////    printf("\ncpt.msg: %s\n", cpt.msg);
//
//
//    char buf[255];
//    snprintf(buf, sizeof(buf), "%s %d: ", "From channel", cpt.channel_id);
//
//    cpt.msg_len += strlen(buf);
//    cpt.msg = malloc(cpt.msg_len * sizeof(char));
//    strncpy(cpt.msg, buf, strlen(buf));
//
//    strcat(cpt.msg, msg_rcv);
//   printf("\ncpt.msg final: %s\n", cpt.msg);
//
//
//
////    printf("\n---------- After leave channel -------------\n");
////    cpt_leave_channel_response(client2, &client2, 5, 1);
////    print_client_list(client2);
////
////    printf("\n---------- After logout-------------\n");
////    cpt_logout_response(&client2, 1);
////    print_client_list(client2);
////
////
////    printf("\n---------- After create channel-------------\n");
////    cpt_create_channel_response(client2, &client2, 5, 0, 1, "", "fadi");
////    print_client_list(client2);
////
////    printf("\n---------- After create channel with existing channel-------------\n");
////    cpt_create_channel_response(client2, &client2, 0, 0, 2, "", "hey");
////    print_client_list(client2);
////
////    printf("\n---------- After create channel with existing channel-------------\n");
////    cpt_login_response(client2, &client2, 1, "fadi");
////    cpt_create_channel_response(client2, &client2, 5, 2, 1, "3", "fadi");
////    print_client_list(client2);
//}


/**
 * Adds a new client node to the client linked-list.
 * @param head_ref reference to client linked-list.
 * @param chan_id channel id of the new client.
 * @param fd file descriptor of the new client.
 */
///////////////////////////////////////// co ///////////////////////////////////////////////
void push(struct Client **head_ref, int chan_id, int fd, char *name) {
    struct Client *new_node = (struct Client *) malloc(sizeof(struct Client));
    new_node->fd = fd;
    new_node->chan_id = chan_id;
    new_node->name = name;
    new_node->next = (*head_ref);
    (*head_ref) = new_node;
}

/**
 * Adds a new client node to the client linked-list.
 * @param head_ref reference to client linked-list.
 * @param chan_id channel id of the new client.
 * @param fd file descriptor of the new client.
 */
///////////////////////////////////////// co ///////////////////////////////////////////////
int cpt_login_response(struct Client *node, struct Client **head, int fd, char *name) {
    while (node != NULL) {
        if (node->fd == fd) {
            printf("user already exist\n");
            return FAILED;
        }
        node = node->next;
    }

    //Add client FD to main channel (0) linked-list
    push(head, 0, fd, name);

    return LOGIN;
}

///////////////////////////////////////////////////////////////co //////////////////////////////////
int cpt_logout_response(struct Client **head, int fd) {
    delete_client(head, -1, fd);
    return LOGOUT;
}

/**
 * Prints a list of all currently unique client file descriptor and channel id combination.
 * @param node reference to client linked-list.
 */
///////////////////////////////////////// co /////////////////////////////////////////////////////
void print_client_list(struct Client *node) {
    printf("------ Clients Currently Connected! ------\n");
    printf("Client list:\n");
    while (node != NULL) {
        printf("chan_id: %d, fd: %d, name: %s\n", node->chan_id, node->fd, node->name);
        node = node->next;
    }
}


int get_uesrs_list(struct Client *node, uint16_t channel_id, uint8_t *msg_len, char **msg) {

    char buf[256];
    while (node != NULL) {
        if (node->chan_id == channel_id) {
            snprintf(buf, sizeof(buf), "%d %s\n", node->fd, node->name);
            *msg_len += strlen(buf);
            *msg = (char *) realloc(*msg, (strlen(buf) * sizeof(char)));
            strcat(*msg, buf);
        }
        node = node->next;
    }
    return 3;
}


/**
 * Takes the client channel_id and fd to creates a new node in the client linked-list.
 * @param node reference to client linked-list.
 * @param ref_node double pointer to client linked-list.
 * @param channel_id channel id of new client.
 * @param fd file descriptor of new client.
 * @return 9 if successful 7 if failure.
 */
/////////////////////////////////////////////co ////////////////////////////////////////////////////
int cpt_join_channel_response(struct Client *node, struct Client **ref_node, uint16_t channel_id,
                              int fd, char *name) {
    struct Client *node1;
    node1 = node;

    // Check if the client who is asking to join the channel has logged in or not.
    int flag = 0;
    while (node1 != NULL) {
        if (node1->fd == fd) {
            flag = 1;
        }
        node1 = node1->next;
    }
    if (flag == 0) {
        printf("You need to login first: %s\n", name);
        return FAILED;
    }

    //Check if the user is a member of this channel.
    while (node != NULL) {
        if (node->fd == fd && node->chan_id == channel_id) {
            printf("You are a member of this channel\n");
            return FAILED;
        }
        node = node->next;
    }
    //IF not, add it.
    push(ref_node, channel_id, fd, name);

    return JOIN_CHANNEL;
}

int cpt_create_channel_response(struct Client *node, struct Client **ref_node, uint16_t channel_id, int msg_len,
                                int fd, const char *message, char *name) {
    struct Client *node1, *node3;
    node1 = node;

    // Check if the client who is asking to create the channel has logged in or not.
    int flag = 0;
    while (node1 != NULL) {
        if (node1->fd == fd) {
            flag = 1;
        }
        node1 = node1->next;
    }
    if (flag == 0) {
        printf("You need to login first: %s\n", name);
        return FAILED;
    }

    //Creating a channel for yourself
    if (msg_len == 0) {
        push(ref_node, channel_id, fd, name);
    }

        //Creating a channel for yourself and with users in message (Find all fd's in the message and add them to the new channel).
    else {
        push(ref_node, channel_id, fd, name);
        for (int i = 0; i < msg_len; i += 2) {
            node3 = node;

            // to get the name of the fd
            while (node3 != NULL) {
                if (node3->fd == message[i] - '0') {
                    push(ref_node, channel_id, node3->fd, node3->name);
                    break;
                }
                node3 = node3->next;
            }
        }
    }
    return CREATE_CHANNEL;
}

/**
 * Deletes a specific client from the client linked-list if the channel id
 * and file descriptor matches.
 * @param head_ref reference to client linked-list.
 * @param chan_id channel id of the new client.
 * @param fd_key file descriptor of the new client.
 */
//////////////////////////////////////////////////////////////co//////////////////////////////////////////
void delete_client(struct Client **head_ref, int chan_id, int fd_key) {
    while (*head_ref) {
        if (chan_id == -1) {
            if ((*head_ref)->fd == fd_key) {
                struct Client *tmp = *head_ref;
                *head_ref = (*head_ref)->next;
                free(tmp);
            } else {
                head_ref = &(*head_ref)->next;
            }
        } else {
            if ((*head_ref)->fd == fd_key && (*head_ref)->chan_id == chan_id) {
                struct Client *tmp = *head_ref;
                *head_ref = (*head_ref)->next;
                free(tmp);
            } else {
                head_ref = &(*head_ref)->next;
            }
        }
    }
}

/////////////////////////////////////////////////////////co//////////////////////////////////////////////////////////
int cpt_leave_channel_response(struct Client *node, struct Client **ref_node, uint16_t channel_id,
                               int fd) {
    int flag = 0;
    //Check if the
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
    return LEAVE_CHANNEL;
}

int cpt_send_response(int fd, int code, int msg_length, char *msg) {
    unsigned char buf[1024];
    size_t packet_size;

    struct CptResponse response;
    response.code = code;
    response.data_size = msg_length;
    response.data = msg;

    packet_size = pack(buf, "CHs", (uint8_t) response.code, (uint16_t) msg_length, response.data);

    send(fd, buf, packet_size, 0);

    return SEND;
}
