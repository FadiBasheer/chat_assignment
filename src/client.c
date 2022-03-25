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
#include <ctype.h>
#include <stdarg.h>

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

void packi16(unsigned char *buf, unsigned int i);
unsigned int pack(unsigned char *buf, char *format, ...);
unsigned int unpacku16(unsigned char *buf);
void unpack(unsigned char *buf, char *format, ...);

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

int cpt_login(void * cpt);
int cpt_get_users(void * cpt, void * query_string, void * response_buffer);
int cpt_send_msg(void * cpt, char * msg, int msg_flag);
int cpt_logout(void * cpt);
int cpt_join_channel(void * cpt, int channel_id);
int cpt_create_channel(void * cpt, void * members, int access_flag);
int cpt_leave_channel(void * cpt, int channel_id);

int main(void)
{

}

//##############################################################################################################
//CPT Builder Functions
//##############################################################################################################

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
    struct cpt *cpt = malloc(sizeof(struct cpt));

    return cpt;
}

/**
* Free all memory and set fields to null.
*
* @param cpt   Pointer to a cpt structure.
*/
void cpt_builder_destroy(struct cpt *cpt)
{
    cpt->version = 0;
    cpt->command = 0;
    cpt->channel_id = 0;
    cpt->msg_len = 0;
    free(cpt->msg);
}

/**
* Set the command value for the cpt header block.
*
* @param cpt   Pointer to a cpt structure.
* @param cmd   From enum commands.
*/
void cpt_builder_cmd(struct cpt *cpt, enum commands_client cmd)
{
    cpt->command = (uint8_t)cmd;
}

/**
* Set major and minor version for the cpt header block.
*
* @param cpt           Pointer to a cpt structure.
* @param version_major From enum version.
* @param version_minor From enum version.
*/
void cpt_builder_version(struct cpt *cpt, enum version version_major, enum version version_minor)
{
    cpt->version = (uint8_t) version_major;
}

/**
* Set the message length for the cpt header block.
*
* @param cpt       Pointer to a cpt structure.
* @param msg_len   An 8-bit integer.
*/
void cpt_builder_len(struct cpt *cpt, uint8_t msg_len)
{
    cpt->msg_len = msg_len;
}

/**
* Set the channel id for the cpt header block.
*
* @param cpt           Pointer to a cpt structure.
* @param channel_id    A 16-bit integer.
*/
void cpt_builder_chan(struct cpt *cpt, uint16_t channel_id)
{
    cpt->channel_id = channel_id;
}

/**
* Set the MSG field for the cpt packet and
* appropriately update the MSG_LEN
*
* @param cpt  Pointer to a cpt structure.
* @param msg  Pointer to an array of characters.
*/
void cpt_builder_msg(struct cpt *cpt, char *msg)
{
    cpt->msg = malloc(cpt->msg_len * sizeof(char));
    cpt->msg = msg;
}

/**
* Create a cpt struct from a cpt packet.
*
* @param packet    A serialized cpt protocol message.
* @return          A pointer to a cpt struct.
*/
struct cpt * cpt_builder_parse(void *packet)
{
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
void * cpt_builder_serialize(struct cpt * cpt)
{
    unsigned char *buf;
    buf = malloc(1024 * sizeof(char));
    pack(buf, "CCHCs", (uint8_t) cpt->version, (uint8_t) cpt->command, (uint16_t) cpt->channel_id, (uint8_t) cpt->msg_len, cpt->msg);

    return buf;
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

//##############################################################################################################
//CPT Functions
//##############################################################################################################

/**
* Send client information to the server.
*
* Sends client information and adds them to the chat server.
* Successful execution of this function call should enable
* the client program's information to persist within the
* server until cpt_logout() is called.
*
* @param cpt   CPT packet and any additional information.
* @return      A status code.  Either from the server, or user defined.
*/
int cpt_login(void * cpt)
{

}

/**
* Get users from the server.
*
* Makes a request to the CPT server for errors.
*      > <query_string> may be passed in optionally,
*        as additional filter parameters for the server's
*        data organization structure(s).
*      > If <query_string> is not NULL, it will overwrite
*        the existing BODY of the CPT packet before sending it.
*
* @param cpt           		CPT packet and any additional information.
* @param query_string  		A pointer intended to provide additional
*                      		query params.
* @param response_buffer  	This is the response that comes back from the
					server.
* @return A status code.  	Either from the server, or user defined.
*/
int cpt_get_users(void * cpt, void * query_string, void * response_buffer)
{

}

/**
* Send a message to the current channel.
*
* Sends a message to the channel specified in the packet CHAN_ID.
*      > <msg> may be passed in optionally in the event that the
*        MSG field of the packet has not been assigned.
*      > If <msg> is not NULL, it will overwrite the
*        existing MSG of the CPT packet and update
*        the MSG_LEN field accordingly.
*      > <msg_flag> may be passed in optionally to specify
*        the type of data being sent to the channel.
*      > If <msg_type> is not NULL, the function will overwrite
*        the existing value of the MSG_TYPE bit fields in the CPT
*        packets to the value passed to <msg_type>.
*      > If <msg_type> is NULL, the function will send a standard
*        text-based message, overwriting the existing flags set
*        in MSG_TYPE with the predefined value of CPT_TEXT.
*
* @param cpt  	CPT packet and any additional information.
* @param msg  	A string to be sent as a message.
* @return A status code.  Either from the server, or user defined.
*/
int cpt_send_msg(void * cpt, char * msg, int msg_flag)
{

}

/**
* Remove client information from the server.
*
* Sends a server request to remove the client from the server.
* Successful execution of this function call should remove the
* existing client's information from any and all data structures
* used to represent a CHANNEL.
*
* @param cpt   CPT packet and any additional information.
* @return      A status code. Either from the server, or user defined.
*/
int cpt_logout(void * cpt)
{

}

/**
* Add a user to the channel on the server.
*
* Adds a user to an existing channel on the server.
*      > Successful execution will appropriately move
*        the user to the specified CHAN_ID.
*      > Execution of this function will NOT remove
*        the user from any CHANNEL they exist in already.
*      > <channel_id> may be passed in optionally in
*        to join a channel different than the one
*        existing in the cpt packet.
*      > If <channel_id> is not NULL, it will overwrite the
*        the existing CHAN_ID field in the cpt packet.
*
* @param cpt         CPT packet and any additional information.
* @param channel_id  The ID of the intended channel to be removed from.
* @return 		   A status code. Either from the server, or user defined.
*/
int cpt_join_channel(void * cpt, int channel_id)
{

}

/**
* Create a new CHANNEL on the server.
*
* Sends a request to the server to create a new channel.
* If successful, the server will create a new CHANNEL data
* structure and add the client to the CHANNEL.
*      > <members> may be optionally passed as selection
*        parameters for the server.
*        If the parameters in <members> are sufficient to
*        identify the target channel members, all members
*        will be added to the channel by default.
*      > If <members> is not NULL, it will overwrite the
*        existing BODY of the cpt packet.
*      > If <is_private> is set to true, the packet ACCESS
*        bit field will be overwritten to PRIVATE making
*        the created CHANNEL visible only to the user, and
*        any members added via the <members> parameter.
*      > If <is_private> is set to false, the packet ACCESS
*        bit field will be overwritten to PUBLIC making the
*        created CHANNEL visible to all active users on the server.
*      > If <is_private> is NULL, the channel will be created using
*        the current FLAG value set in the ACCESS bit field.
*
* @param  cpt         CPT packet and any additional information.
* @param  members     Additional member selection parameters for the server.
* @param  is_private  Set the created channel to private or public.
* @return A status code. Either from the server, or user defined.
*/
int cpt_create_channel(void * cpt, void * members, int access_flag)
{

}

/**
* Leave the current channel.
*
* Sends a server request to remove the client from the current
* channel stored in the CHAN_ID field of the cpt packet.
*      > Successful execution will remove any instance
*        of the user in the CHANNEL within the server
*        and move them to CHANNEL 0 (zero) by default.
*      > <channel_id> may be passed in optionally in
*        to leave a channel different than the one
*        existing in the cpt packet.
*      > If <channel_id> is not NULL, it will overwrite the
*        the existing CHAN_ID field in the cpt packet.
*      > An attempt to leave CHANNEL 0 (zero), either by
*        setting the <channel_id> or using the existing
*        CHAN_ID field in the packet is forbidden, and will
*        return an error code.
*
* @param cpt         CPT packet and any additional information.
* @param channel_id  The ID of the intended channel to be removed from.
* @return 		   A status code. Either from the server, or user defined.
*/
int cpt_leave_channel(void * cpt, int channel_id)
{

}

/*
** packi16() -- store a 16-bit int into a char buffer (like htons())
*/
void packi16(unsigned char *buf, unsigned int i)
{
    *buf++ = i>>8; *buf++ = i;
}

/*
** pack() -- store data dictated by the format string in the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (16-bit unsigned length is automatically prepended to strings)
*/

unsigned int pack(unsigned char *buf, char *format, ...)
{
    va_list ap;

    signed char c;              // 8-bit
    unsigned char C;

    int h;                      // 16-bit
    unsigned int H;

    char *s;                    // strings
    unsigned int len;

    unsigned int size = 0;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {

            case 'C': // 8-bit unsigned
                size += 1;
                C = (unsigned char)va_arg(ap, unsigned int); // promoted
                *buf++ = C;
                break;

            case 'H': // 16-bit unsigned
                size += 2;
                H = va_arg(ap, unsigned int);
                packi16(buf, H);
                buf += 2;
                break;

            case 's': // string
                s = va_arg(ap, char*);
                len = strlen(s);
                size += len + 2;
                packi16(buf, len);
                buf += 2;
                memcpy(buf, s, len);
                buf += len;
                break;
        }
    }

    va_end(ap);

    return size;
}

unsigned int unpacku16(unsigned char *buf)
{
    return ((unsigned int)buf[0]<<8) | buf[1];
}

/*
** unpack() -- unpack data dictated by the format string into the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (string is extracted based on its stored length, but 's' can be
**  prepended with a max length)
*/
void unpack(unsigned char *buf, char *format, ...)
{
    va_list ap;

    signed char *c;              // 8-bit
    unsigned char *C;

    int *h;                      // 16-bit
    unsigned int *H;

    long int *l;                 // 32-bit
    unsigned long int *L;

    long long int *q;            // 64-bit
    unsigned long long int *Q;

    float *f;                    // floats
    double *d;
    long double *g;
    unsigned long long int fhold;

    char *s;
    unsigned int len, maxstrlen=0, count;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {
            case 'c': // 8-bit
                c = va_arg(ap, signed char*);
                if (*buf <= 0x7f) { *c = *buf;} // re-sign
                else { *c = -1 - (unsigned char)(0xffu - *buf); }
                buf++;
                break;

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
                    maxstrlen = maxstrlen * 10 + (*format-'0');
                }
        }

        if (!isdigit(*format)) maxstrlen = 0;
    }

    va_end(ap);
}
