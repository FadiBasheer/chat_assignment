//
// Created by fadi on 2022-03-26.
//

#include <stdio.h>
#include <malloc.h>

struct Client {
    int fd;
    int chan_id;
    struct Client *next;
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

int main() {
    struct Client *client = NULL;

    push(&client, 0, 4);
    push(&client, 0, 7);
    printList(client);
    printf("heelo");
}