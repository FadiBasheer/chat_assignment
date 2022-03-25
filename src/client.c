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
#define MAXLINE 4096
#define SA struct sockaddr

int main(void)
{
    int socket_fd;
    struct sockaddr_in server_addr;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error 1: ");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(socket_fd, (SA *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error 3: ");
    }

    ssize_t nread;
    ssize_t nread_all_clients;
    char data[1024];
    char server_msg_buf[1024];
    char all_clients_buffer[1024];

    recv(socket_fd, server_msg_buf, 21, 0);
    printf("%s\n", server_msg_buf);

    int max_fd;
    max_fd = socket_fd;
    fd_set fd_read_set;
    fd_set fd_read_accepted_set;
    fd_set fd_write_set;
    fd_set fd_write_accepted_set;

    struct timeval timer;
    timer.tv_sec = 1;
    timer.tv_usec = 0;

    int flags = fcntl(socket_fd, F_GETFL);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    fcntl(1, F_SETFL, flags | O_NONBLOCK);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);

    while(1)
    {
        FD_ZERO(&fd_read_set);
        FD_SET(socket_fd, &fd_read_set);
        FD_SET(1, &fd_read_set);
        FD_ZERO(&fd_write_set);
        FD_SET(socket_fd, &fd_write_set);

        if(max_fd < socket_fd)
        {
            max_fd = socket_fd;
        }

        fd_read_accepted_set = fd_read_set;
        fd_write_accepted_set = fd_write_set;

        int fds_selected;
        fds_selected = select(max_fd + 1, &fd_read_accepted_set, NULL, NULL, &timer);

        if(fds_selected > 0)
        {
            if (FD_ISSET(socket_fd, &fd_read_set) != 0)
            {
                nread_all_clients = read(socket_fd, all_clients_buffer, 1024);
                if(nread_all_clients > 0)
                {
                    printf("From other clients: %s\n", all_clients_buffer);
                }
            }

            if (FD_ISSET(1, &fd_read_set) != 0)
            {
                nread = read(STDOUT_FILENO, data, 1024);
                if(nread > 0)
                {
                    write(socket_fd, data, (size_t) nread);
                }
            }
        }
    }

}
