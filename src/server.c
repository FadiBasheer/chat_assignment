#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#define SERVER_PORT 8000
#define BUFSIZE 1024
#define SA struct sockaddr
#define MAXCHANSIZE 32
#define MAXCHAN 32

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

enum commands_client{
    SEND = 0,
    GET_USERS = 3,
    CREATE_CHANNEL = 4,
    JOIN_CHANNEL = 6,
    LEAVE_CHANNEL = 7
};

enum version{
    MAJOR = 1,
    MINOR = 2
};

struct cpt * cpt_builder_init(void);
void cpt_builder_destroy(struct cpt * cpt);
void cpt_builder_cmd(struct cpt * cpt, enum commands_client cmd);
void cpt_builder_version(struct cpt * cpt, enum version version_major, enum version version_minor);
void cpt_builder_len(struct cpt * cpt, uint8_t msg_len);
void cpt_builder_chan(struct cpt * cpt, uint16_t channel_id);
void cpt_builder_msg(struct cpt * cpt, char * msg);
struct cpt * cpt_builder_parse(void * packet);
void * cpt_builder_serialize(struct cpt * cpt);
int cpt_validate(void * packet);

void push(struct Client** head_ref, int chan_id, int fd);
void deleteClient(struct Client** head_ref, int chan_id, int fd_key);
void printList(struct Client* node);

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error 1: ");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("Error 2: ");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if ((bind(server_fd, (SA *)&server_addr, sizeof(server_addr))) < 0)
    {
        perror("Error 3: ");
        if(close(server_fd) < 0)
        {
            perror("Error 8: ");
        }
        exit(EXIT_FAILURE);
    }

    if ((listen(server_fd, 10)) < 0)
    {
        perror("Error 4: ");
        if(close(server_fd) < 0)
        {
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

    while (1)
    {
        FD_ZERO(&fd_read_set);
        FD_SET(server_fd, &fd_read_set);

        struct timeval timer;
        timer.tv_sec = 5;
        timer.tv_usec = 0;

        if (max_fd < server_fd)
        {
            max_fd = server_fd;
        }

        fd_accepted_set = fd_read_set;

        printf("------ Awaiting Connections! ------\n");

        int fds_selected;
        fds_selected = select(max_fd + 1, &fd_accepted_set, NULL, NULL, &timer);
        if(fds_selected > 0)
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            client_fd = accept(server_fd, (SA *)&client_addr, &client_addr_len);
            if (client_fd < 0)
            {
                perror("Error 6: ");
//                exit_code = 0;
                if(close(server_fd) < 0)
                {
                    perror("Error 8: ");
                }
                exit(EXIT_FAILURE);
            }
            printf("Client Connected, ip: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            if (FD_ISSET(server_fd, &fd_read_set) != 0)
            {
                char client_connected_msg[] = "Connected to Server!";

                if (send(client_fd, client_connected_msg, strlen(client_connected_msg), 0) != (ssize_t)strlen(client_connected_msg))
                {
                    perror("Error 7: ");
                }

                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                //add client FD to main channel linked-list
                push(&client, 0, client_fd);

                printf("------ Clients Currently Connected! ------\n");
                printList(client);

                FD_SET(client_fd, &fd_read_set);
            }
        }

        struct Client *head_client = client;
        while (head_client != NULL)
        {
            FD_SET(head_client->fd, &fd_read_set);
            if(head_client->fd > 0 && max_fd < head_client->fd)
            {
                max_fd = head_client->fd;
            }
            head_client = head_client->next;
        }

        char client_buffer[BUFSIZE];
        timer.tv_sec = 5;
        timer.tv_usec = 0;

        fds_selected = select(max_fd + 1, &fd_read_set, NULL, NULL, &timer);

        if (fds_selected > 0)
        {
            head_client = client;
            while (head_client != NULL)
            {
                if(FD_ISSET(head_client->fd, &fd_read_set) != 0)
                {
                    memset(client_buffer, 0, BUFSIZE);
                    ssize_t nread = read(head_client->fd, &client_buffer, BUFSIZE);

                    if(nread != 0)
                    {
                        struct Client *head_client_write = client;
                        while (head_client_write != NULL)
                        {
                            write(head_client_write->fd, client_buffer, BUFSIZE);
                            head_client_write = head_client_write->next;
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
struct cpt * cpt_builder_init(void)
{
}

/**
* Free all memory and set fields to null.
*
* @param cpt   Pointer to a cpt structure.
*/
void cpt_builder_destroy(struct cpt * cpt)
{

}

/**
* Set the command value for the cpt header block.
*
* @param cpt   Pointer to a cpt structure.
* @param cmd   From enum commands.
*/
void cpt_builder_cmd(struct cpt * cpt, enum commands_client cmd)
{

}

/**
* Set major and minor version for the cpt header block.
*
* @param cpt           Pointer to a cpt structure.
* @param version_major From enum version.
* @param version_minor From enum version.
*/
void cpt_builder_version(struct cpt * cpt, enum version version_major, enum version version_minor)
{

}

/**
* Set the message length for the cpt header block.
*
* @param cpt       Pointer to a cpt structure.
* @param msg_len   An 8-bit integer.
*/
void cpt_builder_len(struct cpt * cpt, uint8_t msg_len)
{

}

/**
* Set the channel id for the cpt header block.
*
* @param cpt           Pointer to a cpt structure.
* @param channel_id    A 16-bit integer.
*/
void cpt_builder_chan(struct cpt * cpt, uint16_t channel_id)
{

}

/**
* Set the MSG field for the cpt packet and
* appropriately update the MSG_LEN
*
* @param cpt  Pointer to a cpt structure.
* @param msg  Pointer to an array of characters.
*/
void cpt_builder_msg(struct cpt * cpt, char * msg)
{

}

/**
* Create a cpt struct from a cpt packet.
*
* @param packet    A serialized cpt protocol message.
* @return          A pointer to a cpt struct.
*/
struct cpt * cpt_builder_parse(void * packet)
{

}

/**
* Create a cpt struct from a cpt packet.
*
* @param packet    A serialized cpt protocol message.
* @return          A pointer to a cpt struct.
*/
void * cpt_builder_serialize(struct cpt * cpt)
{

}

/**
* Check serialized cpt to see if it is a valid cpt block.
*
* @param packet    A serialized cpt protocol message.
* @return          0 if no issues, otherwise CPT error code.
*/
int cpt_validate(void * packet)
{

}

void push(struct Client** head_ref, int chan_id, int fd)
{
    struct Client* new_node = (struct Client*)malloc(sizeof(struct Client));
    new_node->fd = fd;
    new_node->chan_id = chan_id;
    new_node->next = (*head_ref);
    (*head_ref) = new_node;
}

void deleteClient(struct Client** head_ref, int chan_id, int fd_key)
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

void printList(struct Client* node)
{
    while (node != NULL) {
        printf("chan_id:%d, fd:%d\n", node->chan_id, node->fd);
        node = node->next;
    }
}
