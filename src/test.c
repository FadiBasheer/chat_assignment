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

//typedef struct Client Client;
struct Client {
    int fd;
    int chan_id;
    struct Client *next;
};

struct cpt {
    uint8_t version;
    uint8_t command;
    uint16_t channel_id;
    uint8_t msg_len;
    char *msg;
};

struct CptResponse {
    uint8_t code;
    uint16_t data_size;
    char *data;
};

enum version {
    MAJOR = 1,
    MINOR = 2
};

struct cpt *cpt_builder_init(void);

void cpt_builder_destroy(struct cpt *cpt);

struct cpt *cpt_builder_parse(void *packet);

void *cpt_builder_serialize(struct cpt *cpt);

void push(struct Client **head_ref, int chan_id, int fd);

void deleteClient(struct Client **head_ref, int chan_id, int fd_key);

void printList(struct Client *node);


int cpt_get_users_response(void *server_info, struct Client *node, uint16_t channel_id);

int cpt_join_channel_response(void *server_info, struct Client *node, uint16_t channel_id, int fd);

int cpt_create_channel_response(void *server_info, struct Client *node, char *id_list);

int cpt_leave_channel_response(void *server_info, struct Client *node, uint16_t channel_id, int fd);

int cpt_send_response(int fd, int code, int msg_length, char *msg);

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

    while (1) {
        FD_ZERO(&fd_read_set);
        FD_SET(server_fd, &fd_read_set);

        struct timeval timer;
        timer.tv_sec = 5;
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

                printf("------ Clients Currently Connected! ------\n");
                //  printList(client);

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

                        printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                               cpt.version, cpt.command, cpt.channel_id, cpt.msg_len, cpt.msg);

                        // If command is send
                        if (cpt.command == 1) {
                            struct Client *head_client_write = client;

                            while (head_client_write != NULL) {
                                printf("%d\n", head_client_write->chan_id);

                                if (head_client_write->chan_id == cpt.channel_id) {
                                    cpt_send_response(head_client_write->fd, 0, cpt.msg_len, cpt.msg);
                                }
                                head_client_write = head_client_write->next;
                            }
                        }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                        // If command is get users
//                        if (cpt.command == 3) {
//                            function_response =  cpt_get_users_response(NULL, client, cpt.channel_id);
//                            cpt_send_response(head_client->fd, function_response, 0, "");
//                        }

                        // join channel
                        if (cpt.command == 5) {
                            function_response = cpt_join_channel_response(NULL, client, cpt.channel_id, client_fd);
                            cpt_send_response(head_client->fd, function_response, 0, "");
                            printList(client);
                        }

                        //LEAVE_CHANNEL
                        if (cpt.command == 6) {
                            function_response = cpt_leave_channel_response(NULL, client, cpt.channel_id, client_fd);
                            cpt_send_response(head_client->fd, function_response, 0, "");
                            printList(client);
                        }
                    }
                }
                head_client = head_client->next;
            }
        }
    }
}

/**
* Initialize cpt struct.
*
* Dynamically allocates a cpt struct and
* initializes all fields.
*
* @return Pointer to cpt struct.
*/
struct cpt *cpt_builder_init(void) {
    struct cpt *cpt = malloc(sizeof(struct cpt));

    return cpt;
}

/**
* Free all memory and set fields to null.
*
* @param cpt   Pointer to a cpt structure.
*/
void cpt_builder_destroy(struct cpt *cpt) {
    cpt->version = 0;
    cpt->command = 0;
    cpt->channel_id = 0;
    cpt->msg_len = 0;
    free(cpt->msg);
}

/**
* Create a cpt struct from a cpt packet.
*
}

* @param packet    A serialized cpt protocol message.
* @return          A pointer to a cpt struct.
*/
struct cpt *cpt_builder_parse(void *packet) {
    struct cpt *cpt;
    char msg_rcv[1024];

    unpack(packet, "CCHCs", &cpt->version, &cpt->command, &cpt->channel_id, &cpt->msg_len, &msg_rcv);

    cpt->msg = malloc(cpt->msg_len * sizeof(char));
    strncpy(cpt->msg, msg_rcv, cpt->msg_len);

    return cpt;
}

/**
* Create a cpt struct from a cpt packet.
*
* @param packet    A serialized cpt protocol message.
* @return          A pointer to a cpt struct.
*/
void *cpt_builder_serialize(struct cpt *cpt) {
    unsigned char *buf;
    buf = malloc(1024 * sizeof(char));
    pack(buf, "CHs", (uint8_t) cpt->version, (uint8_t) cpt->command, (uint16_t) cpt->channel_id,
         (uint8_t) cpt->msg_len, cpt->msg);

    return buf;
}


void push(struct Client **head_ref, int chan_id, int fd) {
    struct Client *new_node = (struct Client *) malloc(sizeof(struct Client));
    new_node->fd = fd;
    new_node->chan_id = chan_id;
    new_node->next = (*head_ref);
    (*head_ref) = new_node;

}


void printList(struct Client *node) {
    while (node != NULL) {
        printf("chan_id:%d, fd:%d\n", node->chan_id, node->fd);
        node = node->next;
    }
}

/**
 * Handle a received 'LOGOUT' protocol message.
 *
 * Uses information in a received CptRequest to handle
 * a GET_USERS protocol message from a connected client.
 *
 * If successful, the function should collect user information
 * from the channel in the CHAN_ID field of the request packet
 * in the following format:
 *
 *  <user_id><whitespace><username><newline>
 *
 * Example given:
 *      1 'Clark Kent'
 *      2 'Bruce Wayne'
 *      3 'Fakey McFakerson'
 *
 * @param server_info   Server data structures and information.
 * @param channel_id    Target channel ID.
 * @return Status Code (SUCCESS if successful, other if failure).
 */
int cpt_get_users_response(void *server_info, struct Client *node, uint16_t channel_id) {
    while (node != NULL) {
        if (node->chan_id == channel_id) {
            printf("chan_id:%d, fd:%d\n", node->chan_id, node->fd);
        }
        node = node->next;
    }
    return 89;
}

/**
 * Handle a received 'JOIN_CHANNEL' protocol message.
 *
 * Uses information in a received CptRequest to handle
 * a JOIN_CHANNEL protocol message from a connected client.
 * If successful, function should add the requesting client
 * user into the channel specified by the CHANNEL_ID field
 * in the CptPacket <channel_id>.
 *
 * @param server_info   Server data structures and information.
 * @param channel_id    Target channel ID.
 * @return Status Code (SUCCESS if successful, other if failure).
 */
int cpt_join_channel_response(void *server_info, struct Client *node, uint16_t channel_id, int fd) {
    int flag = 0;
    while (node != NULL) {
        if (node->fd == fd && node->chan_id == channel_id) {
            flag = 1;
            printf("channel already exist\n");
        }
        node = node->next;
    }

    if (flag == 0) {
        push(&node, channel_id, fd);
    }
    return 9;
}

int cpt_create_channel_response(void *server_info, struct Client *node, char *id_list) {
//    while (node != NULL) {
//        if (node->fd == fd) {
//            node->chan_id = 0;
//        }
//        node = node->next;
//    }
    return 0;
}

void del(struct Client *before_del) {
    struct Client *temp;
    temp = before_del->next;
    before_del->next = temp->next;
    free(temp);
}

int cpt_leave_channel_response(void *server_info, struct Client *node, uint16_t channel_id, int fd) {
    while (node != NULL) {
        if (node->next->fd == fd && node->next->chan_id == channel_id) {
            del(node);
        }
        node = node->next;
    }
    return 10;
}

int cpt_send_response(int fd, int code, int msg_length, char *msg) {
    unsigned char buf[1024];
    unsigned char data[1024];
    uint8_t res_code;
    uint16_t data_size;
    size_t packet_size;

    struct CptResponse response;
    response.code = code;
    response.data_size = msg_length;
    response.data = msg;

    packet_size = pack(buf, "CHs", (uint8_t) response.code, (uint16_t) response.data_size, response.data);

    unpack(buf, "CHs", &res_code, &data_size, &data);

    printf("response.code: %d response.data_size: %d  response.data: %s\n", res_code, data_size, data);

    write(fd, buf, packet_size);
    return 0;
}
