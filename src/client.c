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
#define MAXLINE 4096
#define SA struct sockaddr

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
    char msg[1024];
};

struct commands_client {
    int send;
    int join_chan;
    int leave_chan;
//    SEND = 1,
//    GET_USERS = 3,
//    CREATE_CHANNEL = 4,
//    JOIN_CHANNEL = 5,
//    LEAVE_CHANNEL = 6
};

struct version {
    int ver_num;
};

static void print_menu(void);

//void packi16(unsigned char *buf, unsigned int i);
//unsigned int pack(unsigned char *buf, char *format, ...);
//unsigned int unpacku16(unsigned char *buf);
//void unpack(unsigned char *buf, char *format, ...);

struct cpt *cpt_builder_init(void);

void cpt_builder_destroy(struct cpt *cpt);

void cpt_builder_cmd(struct cpt *cpt, struct commands_client cmd);

void cpt_builder_version(struct cpt *cpt, struct version ver);

void cpt_builder_len(struct cpt *cpt, uint8_t msg_len);

void cpt_builder_chan(struct cpt *cpt, uint16_t channel_id);

void cpt_builder_msg(struct cpt *cpt, char *msg);

struct cpt *cpt_builder_parse(void *packet);

void *cpt_builder_serialize(struct cpt *cpt);

int cpt_validate(void *packet);

int cpt_login(void *cpt);

int cpt_get_users(void *cpt, void *query_string, void *response_buffer);

int cpt_send_msg(void *cpt, char *msg, int msg_flag);

int cpt_logout(void *cpt);

int cpt_join_channel(void *cpt, int channel_id);

int cpt_create_channel(void *cpt, void *members, int access_flag);

int cpt_leave_channel(void *cpt, int channel_id);


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
                    printf("From other clients: %s\n", all_clients_buffer);
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

                        fflush(stdout);
                        printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n", cpt.version,
                               cpt.command, cpt.channel_id, cpt.msg_len, cpt.msg);
                        fflush(stdout);
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
                        fflush(stdout);
                        printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n", cpt.version,
                               cpt.command, cpt.channel_id, cpt.msg_len, cpt.msg);
                        fflush(stdout);
                        packet_size = pack(buf, "CCHCs", (uint8_t) cpt.version, (uint8_t) cpt.command,
                                           (uint16_t) cpt.channel_id, (uint8_t) cpt.msg_len, cpt.msg);
                        send(socket_fd, buf, packet_size, 0);
                        cpt.channel_id = 0;
                        printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n", cpt.version,
                               cpt.command, cpt.channel_id, cpt.msg_len, cpt.msg);
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

                        fflush(stdout);
                        printf("Version: %ul, Command: %ul, channel_id: %ul, msg_len: %ul, msg: %s\n",
                               cpt.version, cpt.command, cpt.channel_id, cpt.msg_len, cpt.msg);
                        fflush(stdout);
                        packet_size = pack(buf, "CCHCs", (uint8_t) cpt.version, (uint8_t) cpt.command,
                                           (uint16_t) cpt.channel_id, (uint8_t) cpt.msg_len, cpt.msg);

                        send(socket_fd, buf, packet_size, 0);
                    }
                }
            }
        }
    }
}

//##############################################################################################################
//CPT Builder Functions
//##############################################################################################################

///**
//* Initialize cpt struct.
//*
//* Dynamically allocates a cpt struct and
//* initializes all fields.
//*
//* @return Pointer to cpt struct.
//*/
//struct cpt *cpt_builder_init(void) {
//    struct cpt *cpt = malloc(sizeof(struct cpt));
//
//    return cpt;
//}
//
///**
//* Free all memory and set fields to null.
//*
//* @param cpt   Pointer to a cpt structure.
//*/
//void cpt_builder_destroy(struct cpt *cpt) {
//    cpt->version = 0;
//    cpt->command = 0;
//    cpt->channel_id = 0;
//    cpt->msg_len = 0;
//    free(cpt->msg);
//}
//
///**
//* Set the command value for the cpt header block.
//*
//* @param cpt   Pointer to a cpt structure.
//* @param cmd   From enum commands.
//*/
//void cpt_builder_cmd(struct cpt *cpt, struct commands_client cmd) {
//    cpt->command = (uint8_t) cmd.send;
//}
//
///**
//* Set major and minor version for the cpt header block.
//*
//* @param cpt           Pointer to a cpt structure.
//* @param version_major From enum version.
//* @param version_minor From enum version.
//*/
//void cpt_builder_version(struct cpt *cpt, struct version ver) {
//    cpt->version = (uint8_t) ver.ver_num;
//}
///**
//* Set the message length for the cpt header block.
//*
//* @param cpt       Pointer to a cpt structure.
//* @param msg_len   An 8-bit integer.
//*/
//void cpt_builder_len(struct cpt *cpt, uint8_t msg_len) {
//    cpt->msg_len = msg_len;
//}
//
///**
//* Set the channel id for the cpt header block.
//*
//* @param cpt           Pointer to a cpt structure.
//* @param channel_id    A 16-bit integer.
//*/
//void cpt_builder_chan(struct cpt *cpt, uint16_t channel_id) {
//    cpt->channel_id = channel_id;
//}
//
///**
//* Set the MSG field for the cpt packet and
//* appropriately update the MSG_LEN
//*
//* @param cpt  Pointer to a cpt structure.
//* @param msg  Pointer to an array of characters.
//*/
//void cpt_builder_msg(struct cpt *cpt, char *msg) {
////    cpt->msg = malloc(cpt->msg_len * sizeof(char));
////    cpt->msg = msg;
//}
//
///**
//* Create a cpt struct from a cpt packet.
//*
//* @param packet    A serialized cpt protocol message.
//* @return          A pointer to a cpt struct.
//*/
//struct cpt *cpt_builder_parse(void *packet) {
//    struct cpt *cpt;
//    char msg_rcv[1024];
//
//    unpack(packet, "CCHCs", &cpt->version, &cpt->command, &cpt->channel_id, &cpt->msg_len, &msg_rcv);
//
////    cpt->msg = malloc(cpt->msg_len * sizeof(char));
//    strncpy(cpt->msg, msg_rcv, cpt->msg_len);
//
//    return cpt;
//}
//
///**
//* Create a cpt struct from a cpt packet.
//*
//* @param packet    A serialized cpt protocol message.
//* @return          A pointer to a cpt struct.
//*/
//void *cpt_builder_serialize(struct cpt *cpt) {
//    unsigned char *buf;
//    buf = malloc(1024 * sizeof(char));
//    pack(buf, "CCHCs", (uint8_t) cpt->version, (uint8_t) cpt->command, (uint16_t) cpt->channel_id,
//         (uint8_t) cpt->msg_len, cpt->msg);
//
//    return buf;
//}
//
///**
//* Check serialized cpt to see if it is a valid cpt block.
//*
//* @param packet    A serialized cpt protocol message.
//* @return          0 if no issues, otherwise CPT error code.
//*/
//int cpt_validate(void *packet) {
//
//}
//
////##############################################################################################################
////CPT Functions
////##############################################################################################################
//
///**
//* Send client information to the server.
//*
//* Sends client information and adds them to the chat server.
//* Successful execution of this function call should enable
//* the client program's information to persist within the
//* server until cpt_logout() is called.
//*
//* @param cpt   CPT packet and any additional information.
//* @return      A status code.  Either from the server, or user defined.
//*/
//int cpt_login(void *cpt) {
//
//}
//
///**
//* Get users from the server.
//*
//* Makes a request to the CPT server for errors.
//*      > <query_string> may be passed in optionally,
//*        as additional filter parameters for the server's
//*        data organization structure(s).
//*      > If <query_string> is not NULL, it will overwrite
//*        the existing BODY of the CPT packet before sending it.
//*
//* @param cpt           		CPT packet and any additional information.
//* @param query_string  		A pointer intended to provide additional
//*                      		query params.
//* @param response_buffer  	This is the response that comes back from the
//					server.
//* @return A status code.  	Either from the server, or user defined.
//*/
//int cpt_get_users(void *cpt, void *query_string, void *response_buffer) {
//
//}
//
///**
//* Send a message to the current channel.
//*
//* Sends a message to the channel specified in the packet CHAN_ID.
//*      > <msg> may be passed in optionally in the event that the
//*        MSG field of the packet has not been assigned.
//*      > If <msg> is not NULL, it will overwrite the
//*        existing MSG of the CPT packet and update
//*        the MSG_LEN field accordingly.
//*      > <msg_flag> may be passed in optionally to specify
//*        the type of data being sent to the channel.
//*      > If <msg_type> is not NULL, the function will overwrite
//*        the existing value of the MSG_TYPE bit fields in the CPT
//*        packets to the value passed to <msg_type>.
//*      > If <msg_type> is NULL, the function will send a standard
//*        text-based message, overwriting the existing flags set
//*        in MSG_TYPE with the predefined value of CPT_TEXT.
//*
//* @param cpt  	CPT packet and any additional information.
//* @param msg  	A string to be sent as a message.
//* @return A status code.  Either from the server, or user defined.
//*/
//int cpt_send_msg(void *cpt, char *msg, int msg_flag) {
//
//}
//
///**
//* Remove client information from the server.
//*
//* Sends a server request to remove the client from the server.
//* Successful execution of this function call should remove the
//* existing client's information from any and all data structures
//* used to represent a CHANNEL.
//*
//* @param cpt   CPT packet and any additional information.
//* @return      A status code. Either from the server, or user defined.
//*/
//int cpt_logout(void *cpt) {
//
//}
//
///**
//* Add a user to the channel on the server.
//*
//* Adds a user to an existing channel on the server.
//*      > Successful execution will appropriately move
//*        the user to the specified CHAN_ID.
//*      > Execution of this function will NOT remove
//*        the user from any CHANNEL they exist in already.
//*      > <channel_id> may be passed in optionally in
//*        to join a channel different than the one
//*        existing in the cpt packet.
//*      > If <channel_id> is not NULL, it will overwrite the
//*        the existing CHAN_ID field in the cpt packet.
//*
//* @param cpt         CPT packet and any additional information.
//* @param channel_id  The ID of the intended channel to be removed from.
//* @return 		   A status code. Either from the server, or user defined.
//*/
//int cpt_join_channel(void *cpt, int channel_id) {
//
//}
//
///**
//* Create a new CHANNEL on the server.
//*
//* Sends a request to the server to create a new channel.
//* If successful, the server will create a new CHANNEL data
//* structure and add the client to the CHANNEL.
//*      > <members> may be optionally passed as selection
//*        parameters for the server.
//*        If the parameters in <members> are sufficient to
//*        identify the target channel members, all members
//*        will be added to the channel by default.
//*      > If <members> is not NULL, it will overwrite the
//*        existing BODY of the cpt packet.
//*      > If <is_private> is set to true, the packet ACCESS
//*        bit field will be overwritten to PRIVATE making
//*        the created CHANNEL visible only to the user, and
//*        any members added via the <members> parameter.
//*      > If <is_private> is set to false, the packet ACCESS
//*        bit field will be overwritten to PUBLIC making the
//*        created CHANNEL visible to all active users on the server.
//*      > If <is_private> is NULL, the channel will be created using
//*        the current FLAG value set in the ACCESS bit field.
//*
//* @param  cpt         CPT packet and any additional information.
//* @param  members     Additional member selection parameters for the server.
//* @param  is_private  Set the created channel to private or public.
//* @return A status code. Either from the server, or user defined.
//*/
//int cpt_create_channel(void *cpt, void *members, int access_flag) {
//
//}
//
///**
//* Leave the current channel.
//*
//* Sends a server request to remove the client from the current
//* channel stored in the CHAN_ID field of the cpt packet.
//*      > Successful execution will remove any instance
//*        of the user in the CHANNEL within the server
//*        and move them to CHANNEL 0 (zero) by default.
//*      > <channel_id> may be passed in optionally in
//*        to leave a channel different than the one
//*        existing in the cpt packet.
//*      > If <channel_id> is not NULL, it will overwrite the
//*        the existing CHAN_ID field in the cpt packet.
//*      > An attempt to leave CHANNEL 0 (zero), either by
//*        setting the <channel_id> or using the existing
//*        CHAN_ID field in the packet is forbidden, and will
//*        return an error code.
//*
//* @param cpt         CPT packet and any additional information.
//* @param channel_id  The ID of the intended channel to be removed from.
//* @return 		   A status code. Either from the server, or user defined.
//*/
//int cpt_leave_channel(void *cpt, int channel_id) {
//
//}

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
