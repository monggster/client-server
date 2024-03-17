#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "message.h"

#define DEFAULT_PORT 8080
#define DEFAULT_BACKLOG 10


/* */
pthread_mutex_t mutex;
pthread_t thread_id;
/* */

void *func(void *data) 
{ 
    unsigned char buff[MESSAGE_SIZE]; 
    message_t message;
    int n, connfd; 

    connfd = *((int *)data);
    
    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MESSAGE_SIZE); 
   
        // read the message from client and copy it in buffer 
        printf("--start read--\n");
        read(connfd, buff, MESSAGE_SIZE); 
        printf("---end read---\n");

        bzero(&message, MESSAGE_SIZE);
        message_deserialize(&message, buff);
        // print message text
        message_print(&message);
        bzero(buff, MESSAGE_SIZE); 

        n = 0; 
        printf("Enter your message: ");
        while ((buff[n++] = getchar()) != '\n') ; 
   
        bzero(&message, MESSAGE_SIZE);
        memcpy(message.sender, "Server", 6);
        memcpy(message.room, "paradnaya", 9);
        memcpy(message.text, buff, strlen((char *)buff));
        // copy server message in the buffer 
        message_serialize(&message, buff);

        write(connfd, buff, MESSAGE_SIZE); 

        if ((strncmp((char *)message.text, "exit", 4)) == 0) {
            printf("Exit...\n");
            break;
        }
    } 
    pthread_exit(NULL);
} 


int check(int expr, const char *msg) 
{
    if (expr < 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return expr;
}

int setup_server(short port, int backlog)
{
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = check(socket(AF_INET, SOCK_STREAM, 0), "socket failed");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    check(bind(server_fd,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)),
            "bind failed");
    check(listen(server_fd, backlog),
            "listen failed");

    return server_fd;
}


int accept_new_connection(int server_fd)
{
    struct sockaddr_in client_addr;
    int client_fd;
    
    socklen_t client_addr_len = sizeof(client_addr);

    client_fd = accept(server_fd,
                    (struct sockaddr *)&client_addr,
                    &client_addr_len);
    return client_fd;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);

    printf("Got request for connection.\n");

    pthread_create(&thread_id, NULL, func, arg);
    pthread_join(thread_id, NULL);

    close(client_fd);
    free(arg);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int server_fd, port;

    /* initialize mutex */
    pthread_mutex_init(&mutex, NULL);


    port = argc < 2 ? DEFAULT_PORT : atoi(*(argv + 1));

    server_fd = setup_server(port, DEFAULT_BACKLOG);

    printf("Server listening on port %d\n", port);

    while (1) {
        int *client_fd = malloc(sizeof(int));

        *client_fd = accept_new_connection(server_fd);

        if (*client_fd < 0) { perror("accept failed"); continue; }

        pthread_create(&thread_id, NULL, handle_client, (void *)client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
