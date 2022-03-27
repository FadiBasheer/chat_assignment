//
// Created by fadi on 2022-03-26.
//

#include <stdio.h>
#include <malloc.h>
#include <stdint-gcc.h>
#include "serialization.c"

struct Client {
    int fd;
    int chan_id;
    struct Client *next;
};


struct CptResponse {
    uint8_t code;
    uint16_t data_size;
    char *data;
};


void push(struct Client **head_ref, int chan_id, int fd) {
    struct Client *new_node = (struct Client *) malloc(sizeof(struct Client));
    new_node->fd = fd;
    new_node->chan_id = chan_id;
    new_node->next = (*head_ref);
    (*head_ref) = new_node;
}


void printList(struct Client *n) {
    while (n != NULL) {
        printf("chan_id:%d, fd:%d\n", n->chan_id, n->fd);
        n = n->next;
    }
}

void *cpt_builder_serialize(struct CptResponse *cpt) {
    unsigned char *buf;
    buf = malloc(1024 * sizeof(char));
    pack(buf, "CHs", (uint8_t) cpt->code, (uint16_t) cpt->data_size, cpt->data);
    return buf;
}

int cpt_send_response(void *server_info, char *name) {

}

int main() {
    struct Client *client = NULL;
    struct CptResponse *response = NULL;

    push(&client, 0, 4);
    push(&client, 0, 7);
    printList(client);
    printf("heelo");

    response->code = (uint8_t) 8;
    response->data_size = 0;
    response->data = " ";
    cpt_builder_serialize(response);
}