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
