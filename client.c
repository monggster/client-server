#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


#include <fcntl.h>
//#include <pthread.h>
#include <string.h>


#include "message.h"

#define DEFAULT_PORT 8080
#define DEFAULT_HOST "127.0.0.1"



int check(int expr, const char *msg) 
{
    if (expr < 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return expr;
}

int setup_client(const char *host, int port)
{
    int client_fd;
    struct sockaddr_in server_address;

    client_fd = check(socket(AF_INET, SOCK_STREAM, 0), "socket failed");

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(host);

    check(connect(client_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)),
            "connect failed");

    return client_fd;
}

void func(int sockfd)
{
    unsigned char buff[MESSAGE_SIZE];
    message_t message;
    int n;
    for (;;) {
        bzero(buff, MESSAGE_SIZE);
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n') ;

        bzero(&message, MESSAGE_SIZE);
        memcpy(message.sender, "Client", 6);
        memcpy(message.room, "paradnaya", 9);
        memcpy(message.text, buff, strlen((char *)buff));
        message_serialize(&message, buff);


        write(sockfd, buff, MESSAGE_SIZE);
        bzero(buff, MESSAGE_SIZE);
        printf("--start read--\n");
        read(sockfd, buff, MESSAGE_SIZE); 
        printf("---end read---\n");

        bzero(&message, MESSAGE_SIZE);
        message_deserialize(&message, buff);
        message_print(&message);

        if ((strncmp((char *)message.text, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}

int main(int argc, char **argv) {
    int client_fd, port;
    char *host;

    host = argc < 2 ? DEFAULT_HOST : *(argv + 1);
    port = argc < 3 ? DEFAULT_PORT : atoi(*(argv + 2));

    client_fd = setup_client(host, port);

    func(client_fd);

    close(client_fd);

    return 0;
}
