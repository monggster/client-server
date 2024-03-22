#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


#include <fcntl.h>
#include <pthread.h>
#include <string.h>


#include "message.h"

#define DEFAULT_PORT 8080
#define DEFAULT_HOST "127.0.0.1"
#define NAME_LENGTH_MIN 4
#define NAME_LENGTH_MAX 20

#if NAME_LENGTH_MIN > NAME_LENGTH_MAX
#error NAME_LENGTH_MAX must be bigger than NAME_LENGTH_MIN
#elif NAME_LENGTH_MAX > NAME_SIZE
#error NAME_LENGTH_MAX can not be bigger than NAME_SIZE
#endif


char username[NAME_LENGTH_MAX + 1];
char roomname[NAME_LENGTH_MAX + 1];
pthread_t thread_id;


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


static int name_is_valid(const char *name)
{
    int l;
    l = strlen(name);
    if(l < NAME_LENGTH_MIN || l > NAME_LENGTH_MAX) return 0;
    return 1;
}

static void get_username() 
{
    int c, n;
    do {
        memset(username, '\0', NAME_LENGTH_MAX + 1);
        printf("Enter your name: ");
        n = 0;
        while((c = getchar()) != '\n' && n < NAME_LENGTH_MAX) {
            username[n++] = (char)c;
        }
    } while (!name_is_valid(username));
}



/* Читает сообщения от сервера */
void *func_read(void *data)
{
    unsigned char buff[MESSAGE_SIZE];
    message_t message;
    int client_fd;

    client_fd = *((int *)data);

    for (;;) {
        bzero(buff, MESSAGE_SIZE);
        read(client_fd, buff, MESSAGE_SIZE); 

        bzero(&message, MESSAGE_SIZE);
        message_deserialize(&message, buff);

        memcpy(roomname, message.room, NAME_LENGTH_MAX);

        printf("(%s) @%s: %s", roomname, message.sender, message.text);
        if ((strncmp((char *)message.text, "#exit", 5)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
    pthread_exit(NULL);
}

/* Отправляет сообщения на сервер */
void *func_write(void *data)
{
    unsigned char buff[MESSAGE_SIZE];
    message_t message;
    int n, client_fd;

    client_fd = *((int *)data);

    for (;;) {

        ///////////////////
        for (n = 0; n < 1000000; n++) __asm("nop");

        bzero(buff, MESSAGE_SIZE);
        printf("(%s): ", roomname);

        n = 0;
        while ((buff[n++] = getchar()) != '\n') ;

        bzero(&message, MESSAGE_SIZE);
        memcpy(message.sender, username, NAME_LENGTH_MAX);
        memcpy(message.room, roomname, NAME_LENGTH_MAX);
        memcpy(message.text, buff, strlen((char *)buff));
        message_serialize(&message, buff);

        write(client_fd, buff, MESSAGE_SIZE);

        if ((strncmp((char *)message.text, "#exit", 5)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
    pthread_exit(NULL);
}

void join_server(int client_fd) 
{
    unsigned char buff[MESSAGE_SIZE];
    message_t message;
    char command[] = "#enter-room"; 
    
    /* Очистка */
    bzero(&message, MESSAGE_SIZE);
    bzero(buff, MESSAGE_SIZE);

    memcpy(message.sender, username, NAME_LENGTH_MAX);
    memcpy(message.room, roomname, NAME_LENGTH_MAX);
    memcpy(message.text, command, strlen(command));
    
    message_serialize(&message, buff);
    write(client_fd, buff, MESSAGE_SIZE);

    /* Очистка */
    bzero(&message, MESSAGE_SIZE);
    bzero(buff, MESSAGE_SIZE);

    read(client_fd, buff, MESSAGE_SIZE); 
    message_deserialize(&message, buff);

    printf("Available rooms:\n%s\n", message.text);

    memcpy(roomname, message.room, NAME_LENGTH_MAX);
}

int main(int argc, char **argv) {
    int client_fd, port;
    char *host;

    host = argc < 2 ? DEFAULT_HOST : *(argv + 1);
    port = argc < 3 ? DEFAULT_PORT : atoi(*(argv + 2));
    get_username();

    client_fd = setup_client(host, port);

    join_server(client_fd);

    pthread_create(&thread_id, NULL, func_write, (void *)&client_fd);
    pthread_detach(thread_id);

    pthread_create(&thread_id, NULL, func_read, (void *)&client_fd);
    pthread_join(thread_id, NULL);

    close(client_fd);

    return 0;
}
