#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include "serialization.c"

#define SERVER_PORT 8000
#define SA struct sockaddr

#define COMMAND_SYMBOL "&"
#define LOGIN_COMMAND "&login"
#define LOGOUT_COMMAND "&logout"
#define USERS_COMMAND "&users"
#define CREATE_CHAN_COMMAND "&create"
#define JOIN_CHAN_COMMAND "&join"
#define LEAVE_CHAN_COMMAND "&leave"
#define MENU_COMMAND "&menu"
#define EXIT_COMMAND "&exit"

#define READ_BUFFER_SIZE 1024
#define COMMAND_INPUT_SIZE 20

/**
 * Chat protocol client request packet specified by COMP4981 chat protocol.
 */
struct CPT {
    uint8_t cpt_version;
    uint8_t command;
    uint16_t channel_id;
    uint8_t msg_len;
    char *msg;
};

/**
 * Chat protocol server response specified by COMP4981 chat protocol.
 */
struct CPTResponse {
    uint8_t code;
    uint16_t data_size;
    char *data;
};

struct ClientState {
    int *previous_channel;
    int *current_channel;
    int *is_logged_in;
};


enum command {
    SEND = 1, LOGOUT, GET_USERS, CREATE_CHANNEL, JOIN_CHANNEL, LEAVE_CHANNEL, LOGIN, JOIN_CHANNEL_FAILURE = 22
};

/**
 * Prints a menu for the user to interact with.
 */
static void print_menu(void);

size_t process_client_input(int *exit_code, char *user_input, unsigned char *cpt_serialized_buf,
                            struct ClientState *clientState);

size_t cpt_login(struct CPT *cpt, uint8_t *serial_buf, char *name);

size_t cpt_logout(struct CPT *cpt, uint8_t *serial_buf);

size_t cpt_get_users(struct CPT *cpt, uint8_t *serial_buf, uint16_t channel_id);

size_t cpt_create_channel(struct CPT *cpt, uint8_t *serial_buf, char *user_list);

size_t cpt_join_channel(struct CPT *cpt, uint8_t *serial_buf, uint16_t channel_id);

size_t cpt_leave_channel(struct CPT *cpt, uint8_t *serial_buf, uint16_t channel_id);

size_t cpt_send(struct CPT *cpt, uint8_t *serial_buf, char *msg, int current_channel);

void cpt_process_response(struct CPTResponse cpt_response, struct ClientState *clientState);

void cpt_packet_destroy(struct CPTResponse cpt_response);

// DELETE
/**
 * Struct for specifying the version of the chat protocol.
 */
struct version {
    int ver_num;
};

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
    server_addr.sin_addr.s_addr = inet_addr("10.65.104.252");
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(socket_fd, (SA *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error 3: ");
    }

    ssize_t nread;
    ssize_t nread_all_clients;
    char server_msg_buf[READ_BUFFER_SIZE];
    char all_clients_buffer[READ_BUFFER_SIZE];

    memset(server_msg_buf, 0, READ_BUFFER_SIZE);
    recv(socket_fd, server_msg_buf, 21, 0);
    printf("%s\n", server_msg_buf);

    int max_fd;
    max_fd = socket_fd;
    fd_set fd_read_set;
    fd_set fd_read_accepted_set;

    struct timeval timer;
    timer.tv_sec = 1;
    timer.tv_usec = 0;

    char user_input[READ_BUFFER_SIZE];

    struct CPT cpt, cptTemp;
    cpt.channel_id = 0;

    unsigned char cpt_serialized_buf[READ_BUFFER_SIZE];
    size_t packet_size;

    struct version version;
    version.ver_num = 1;

    struct CPTResponse cpt_response_packet;
    uint8_t res_code;
    uint16_t data_size;
    unsigned char all_client_data[READ_BUFFER_SIZE];

    struct ClientState client_state;
    client_state.current_channel = malloc(sizeof(client_state.current_channel));
    *client_state.current_channel = 0;
    client_state.previous_channel = malloc(sizeof(client_state.previous_channel));
    *client_state.previous_channel = 0;
    client_state.is_logged_in = malloc(sizeof(client_state.is_logged_in));
    *client_state.is_logged_in = 0;


    int flags = fcntl(socket_fd, F_GETFL);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    fcntl(1, F_SETFL, flags | O_NONBLOCK);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);

    print_menu();
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
                nread_all_clients = read(socket_fd, all_clients_buffer, READ_BUFFER_SIZE);

                if (nread_all_clients > 0) {
                    unpack(all_clients_buffer, "CHs", &res_code, &data_size, &all_client_data);

                    cpt_response_packet.data = malloc(data_size * sizeof(char));
                    cpt_response_packet.code = res_code;
                    cpt_response_packet.data_size = data_size;
                    strcpy(cpt_response_packet.data, all_client_data);

                    printf("res_code: %d, data_size: %d, msg: %s\n", cpt_response_packet.code,
                           cpt_response_packet.data_size, cpt_response_packet.data);

                    cpt_process_response(cpt_response_packet, &client_state);
                }
            }

            //read from keyboard
            if (FD_ISSET(1, &fd_read_set) != 0) {
                memset(user_input, 0, READ_BUFFER_SIZE);
                memset(cpt_serialized_buf, 0, READ_BUFFER_SIZE);
                nread = read(STDOUT_FILENO, user_input, READ_BUFFER_SIZE);

                if (nread > 0) {
                    packet_size = process_client_input(&exit_code, user_input, cpt_serialized_buf, &client_state);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    char msg_r[1024];

                    unpack(cpt_serialized_buf, "CCHCs", &cptTemp.cpt_version, &cptTemp.command, &cptTemp.channel_id,
                           &cptTemp.msg_len,
                           &msg_r);

                    printf("unpack: %u %u %u %u %s\n", cptTemp.cpt_version, cptTemp.command, cptTemp.channel_id,
                           cptTemp.msg_len,
                           msg_r);
//////////////////////////////////////////////////////////////////////////////////////////////////////

                    if (packet_size) {
                        send(socket_fd, cpt_serialized_buf, packet_size, 0);
                    }
                }
            }
        }
    }
    return EXIT_FAILURE;
}

/**
 * Prints a menu for the user to interact with.
 */
static void print_menu() {
    printf("Welcome to Global Channel 0\n");

    printf("########## Chat Commands ##########\n");
    printf("1) Send Message to Current Channel (eg. Hello, World!)\n");
    printf("2) Login (eg. &login Charlie)\n");
    printf("3) Logout (eg. &logout)\n");
    printf("4) Get Users List For A Channel(eg. &users 5)\n");
    printf("5) Create Channel And Add Users(eg. &create 4 5 6)\n");
    printf("6) Join Channel (eg. &join 3)\n");
    printf("7) Leave Channel (eg. &leave 3)\n");
    printf("8) Print Menu Options (eg. &menu)\n");
    printf("9) Exit Chat (eg. &exit)\n\n");
}

size_t process_client_input(int *exit_code, char *user_input, unsigned char *cpt_serialized_buf,
                            struct ClientState *clientState) {
    struct CPT cpt_packet;
    size_t packet_size;

    if (*clientState->is_logged_in == 1) {
        if (!strncmp(user_input, COMMAND_SYMBOL, 1)) {
            if (!strncmp(user_input, LOGOUT_COMMAND, strlen(LOGOUT_COMMAND))) {
                packet_size = cpt_logout(&cpt_packet, cpt_serialized_buf);

                printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                       cpt_packet.cpt_version, cpt_packet.command, cpt_packet.channel_id, cpt_packet.msg_len,
                       cpt_packet.msg);

                return packet_size;
            }

            if (!strncmp(user_input, USERS_COMMAND, strlen(USERS_COMMAND))) {
                char chan_id[COMMAND_INPUT_SIZE];
                long int_chan_id;

                strncpy(chan_id, user_input + strlen(USERS_COMMAND) + 1, strlen(user_input));
                int_chan_id = strtol(chan_id, NULL, 10);
                packet_size = cpt_get_users(&cpt_packet, cpt_serialized_buf, (uint16_t) int_chan_id);

                printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                       cpt_packet.cpt_version, cpt_packet.command, cpt_packet.channel_id, cpt_packet.msg_len,
                       cpt_packet.msg);

                return packet_size;
            }

            if (!strncmp(user_input, CREATE_CHAN_COMMAND, strlen(CREATE_CHAN_COMMAND))) {
                char chan_ids[COMMAND_INPUT_SIZE];

                strncpy(chan_ids, user_input + strlen(CREATE_CHAN_COMMAND) + 1, strlen(user_input));
                packet_size = cpt_create_channel(&cpt_packet, cpt_serialized_buf, chan_ids);

                printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                       cpt_packet.cpt_version, cpt_packet.command, cpt_packet.channel_id, cpt_packet.msg_len,
                       cpt_packet.msg);

                return packet_size;
            }

            if (!strncmp(user_input, JOIN_CHAN_COMMAND, strlen(JOIN_CHAN_COMMAND))) {
                char chan_id[COMMAND_INPUT_SIZE];
                long int_chan_id;

                strncpy(chan_id, user_input + strlen(JOIN_CHAN_COMMAND) + 1, strlen(user_input));
                int_chan_id = strtol(chan_id, NULL, 10);
                packet_size = cpt_join_channel(&cpt_packet, cpt_serialized_buf, (uint16_t) int_chan_id);

                printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                       cpt_packet.cpt_version, cpt_packet.command, cpt_packet.channel_id, cpt_packet.msg_len,
                       cpt_packet.msg);

                *clientState->previous_channel = *clientState->current_channel;
                *clientState->current_channel = (int) int_chan_id;

                return packet_size;
            }

            if (!strncmp(user_input, LEAVE_CHAN_COMMAND, strlen(LEAVE_CHAN_COMMAND))) {
                char chan_id[COMMAND_INPUT_SIZE];
                long int_chan_id;

                strncpy(chan_id, user_input + strlen(LEAVE_CHAN_COMMAND) + 1, strlen(user_input));
                int_chan_id = strtol(chan_id, NULL, 10);
                packet_size = cpt_leave_channel(&cpt_packet, cpt_serialized_buf, (uint16_t) int_chan_id);

                printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                       cpt_packet.cpt_version, cpt_packet.command, cpt_packet.channel_id, cpt_packet.msg_len,
                       cpt_packet.msg);

                *clientState->current_channel = 0;

                return packet_size;
            }

            if (!strncmp(user_input, MENU_COMMAND, strlen(MENU_COMMAND))) {
                print_menu();

                return 0;
            }

            if (!strncmp(user_input, EXIT_COMMAND, strlen(EXIT_COMMAND))) {
                printf("GoodBye!\n");
                *exit_code = 0;

                return 0;
            }

            printf("Invalid Command.\n");
            return 0;
        }
        packet_size = cpt_send(&cpt_packet, cpt_serialized_buf, user_input, *clientState->current_channel);

        printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
               cpt_packet.cpt_version, cpt_packet.command, cpt_packet.channel_id, cpt_packet.msg_len, cpt_packet.msg);
    } else {
        if (!strncmp(user_input, LOGIN_COMMAND, strlen(LOGIN_COMMAND))) {
            char name[COMMAND_INPUT_SIZE];

            strncpy(name, user_input + strlen(LOGIN_COMMAND) + 1, strlen(user_input));
            packet_size = cpt_login(&cpt_packet, cpt_serialized_buf, name);

            printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                   cpt_packet.cpt_version, cpt_packet.command, cpt_packet.channel_id, cpt_packet.msg_len,
                   cpt_packet.msg);

            return packet_size;
        } else {
            printf("Please Login First.\n");
            packet_size = 0;

            return packet_size;
        }
    }

    return packet_size;
}

void cpt_process_response(struct CPTResponse cpt_response, struct ClientState *clientState) {
    switch (cpt_response.code) {
        case SEND:
        case GET_USERS:
            printf("%s\n", cpt_response.data);
            cpt_packet_destroy(cpt_response);
            break;
        case LOGOUT:
            printf("Logout Successful.\n");
            *clientState->is_logged_in = 0;
            cpt_packet_destroy(cpt_response);
            break;
        case CREATE_CHANNEL:
            printf("Channel Created Successfully.\n");
            cpt_packet_destroy(cpt_response);
            break;
        case JOIN_CHANNEL:
            printf("Channel Joined Successfully.\n");
            cpt_packet_destroy(cpt_response);
            break;
        case LEAVE_CHANNEL:
            printf("Channel Left Successfully.\n");
            cpt_packet_destroy(cpt_response);
            break;
        case LOGIN:
            printf("Login Successful.\n");
            *clientState->is_logged_in = 1;
            cpt_packet_destroy(cpt_response);
            break;
        case JOIN_CHANNEL_FAILURE:
            *clientState->current_channel = *clientState->previous_channel;
            printf("Server: Client Command Failed!\n");
            cpt_packet_destroy(cpt_response);
            break;
        default:
            printf("Invalid Server Response Code!\n");
            cpt_packet_destroy(cpt_response);
    }
}

void cpt_packet_destroy(struct CPTResponse cpt_response) {
    cpt_response.code = 0;
    cpt_response.data_size = 0;
    free(cpt_response.data);
}


/**
 * Prepare a SEND request packet for the server.
 *
 * Prepares a SEND request to the server. If successful,
 * the resulting data in <serial_buf> will contain a CPT packet
 * with the necessary information to instruct the server to SEND
 * the message specified by <msg> to every user in the channel
 * specified within the packet CHAN_ID field.
 *
 * @param client_info    CPT packet information and any other necessary data.
 * @param serial_buf     A buffer intended for storing the result.
 * @param msg            Intended chat message.
 * @return Size of the resulting serialized packet in <serial_buf>.
*/
size_t cpt_send(struct CPT *cpt, uint8_t *serial_buf, char *msg, int current_channel) {
    printf("#################\n");
    printf("%d\n", current_channel);

    size_t packet_size;

    cpt->cpt_version = 1;
    cpt->command = SEND;
    cpt->channel_id = (uint16_t) current_channel;
    cpt->msg_len = strlen(msg) + 1;
    cpt->msg = malloc((strlen(msg) + 1) * sizeof(char));
    strcpy(cpt->msg, msg);

    packet_size = pack(serial_buf, "CCHCs", (uint8_t) cpt->cpt_version, (uint8_t) cpt->command,
                       (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return packet_size;
}


/**
 * Prepare a LEAVE_CHANNEL request packet for the server.
 *
 * Prepares a LEAVE_CHANNEL request to the server. If successful,
 * the resulting data in <serial_buf> will contain a CPT packet
 * with the necessary information to remove the client's information
 * from the channel specified by <channel_id>.
 *
 * @param client_info    CPT packet information and any other necessary data.
 * @param serial_buf     A buffer intended for storing the result.
 * @param channel_id     The target channel id.
 * @return Size of the resulting serialized packet in <serial_buf>
*/
size_t cpt_leave_channel(struct CPT *cpt, uint8_t *serial_buf, uint16_t channel_id) {
    size_t packet_size;

    cpt->cpt_version = 1;
    cpt->command = LEAVE_CHANNEL;
    cpt->channel_id = channel_id;
    cpt->msg_len = 0;
    cpt->msg = malloc(1);

    packet_size = pack(serial_buf, "CCHCs", (uint8_t) cpt->cpt_version, (uint8_t) cpt->command,
                       (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return packet_size;
}


/**
 * Prepare a JOIN_CHANNEL request packet for the server.
 *
 * Prepares a JOIN_CHANNEL request to the server. If successful,
 * the resulting data in <serial_buf> will contain a CPT packet
 * with the necessary information to instruct the server to add
 * the client's information to an existing channel.
 *
 * @param client_info    CPT packet information and any other necessary data.
 * @param serial_buf     A buffer intended for storing the result.
 * @param channel_id     The target channel id.
 * @return Size of the resulting serialized packet in <serial_buf>
*/
size_t cpt_join_channel(struct CPT *cpt, uint8_t *serial_buf, uint16_t channel_id) {
    size_t packet_size;

    cpt->cpt_version = 1;
    cpt->command = JOIN_CHANNEL;
    cpt->channel_id = channel_id;
    cpt->msg_len = 0;
    cpt->msg = malloc(1);

    packet_size = pack(serial_buf, "CCHCs", (uint8_t) cpt->cpt_version, (uint8_t) cpt->command,
                       (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return packet_size;
}


/**
 * Prepare a CREATE_CHANNEL request packet for the server.
 *
 * Prepares a CREATE_CHANNEL request to the server. If successful,
 * the resulting data in <serial_buf> will contain a CPT packet
 * with the necessary information to instruct the server to create
 * a new channel.
 *
 *      > <user_list> may be optionally passed as user selection
 *        parameters for the new Channel.
 *      > If <members> is not NULL, it will be assigned to the
 *        MSG field of the packet.
 *
 * @param cpt            CPT packet information and any other necessary data.
 * @param serial_buf     A buffer intended for storing the result.
 * @param user_list      Whitespace separated user IDs as a string.
 * @return Size of the resulting serialized packet in <serial_buf>
*/
size_t cpt_create_channel(struct CPT *cpt, uint8_t *serial_buf, char *user_list) {
    size_t packet_size;

    cpt->cpt_version = 1;
    cpt->command = CREATE_CHANNEL;
    cpt->channel_id = 0;
    cpt->msg_len = 0;
    cpt->msg = malloc((strlen(user_list) + 1) * sizeof(char));
    strcpy(cpt->msg, user_list);

    packet_size = pack(serial_buf, "CCHCs", (uint8_t) cpt->cpt_version, (uint8_t) cpt->command,
                       (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return packet_size;
}


/**
 * Prepare a GET_USERS request packet for the server.
 *
 * Prepares a GET_USERS request to the server. If successful,
 * the resulting data in <serial_buf> will contain a CPT packet
 * with the necessary information to instruct the server to
 * send back a list of users specified by the <channel_id>.
 *
 * @param client_info    CPT packet information and any other necessary data.
 * @param serial_buf     A buffer intended for storing the result.
 * @param channel_id     The ID of the CHANNEL to get users from.
 * @return Size of the resulting serialized packet in <serial_buf>
*/
size_t cpt_get_users(struct CPT *cpt, uint8_t *serial_buf, uint16_t channel_id) {
    size_t packet_size;

    cpt->cpt_version = 1;
    cpt->command = GET_USERS;
    cpt->channel_id = channel_id;
    cpt->msg_len = 0;
    cpt->msg = malloc(1);

    packet_size = pack(serial_buf, "CCHCs", (uint8_t) cpt->cpt_version, (uint8_t) cpt->command,
                       (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return packet_size;
}


/**
 * Prepare a LOGOUT request packet for the server.
 *
 * Prepares a LOGOUT request to the server. If successful,
 * the resulting data in <serial_buf> will contain a CPT packet
 * with the necessary information to instruct the server to remove
 * any instance of the requesting client's information.
 *
 * @param cpt            CPT packet information and any other necessary data.
 * @param serial_buf     A buffer intended for storing the result.
 * @return Size of the resulting serialized packet in <serial_buf>
*/
size_t cpt_logout(struct CPT *cpt, uint8_t *serial_buf) {
    size_t packet_size;

    cpt->cpt_version = 1;
    cpt->command = LOGOUT;
    cpt->channel_id = 0;
    cpt->msg_len = 0;
    cpt->msg = malloc(1);

    packet_size = pack(serial_buf, "CCHCs", (uint8_t) cpt->cpt_version, (uint8_t) cpt->command,
                       (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return packet_size;
}


/**
 * Prepare a LOGIN request packet for the server.
 *
 * Prepares a LOGIN request to the server. If successful,
 * the resulting data in <serial_buf> will contain a CPT packet
 * with the necessary information to instruct the server to
 * persist the client's information until cpt_logout() is called.
 *
 * @param cpt            CPT packet information and any other necessary data.
 * @param serial_buf     A buffer intended for storing the result.
 * @param name           Client login name.
 * @return The size of the serialized packet in <serial_buf>.
*/
size_t cpt_login(struct CPT *cpt, uint8_t *serial_buf, char *name) {
    size_t packet_size;

    cpt->cpt_version = 1;
    cpt->command = LOGIN;
    cpt->channel_id = 0;
    cpt->msg_len = 0;
    cpt->msg = malloc(strlen(name) * sizeof(char));
    strcpy(cpt->msg, name);

    packet_size = pack(serial_buf, "CCHCs", (uint8_t) cpt->cpt_version, (uint8_t) cpt->command,
                       (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return packet_size;
}