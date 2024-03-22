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
#include "associations.h"
#include "nodes.h"

#define DEFAULT_PORT 8080
#define DEFAULT_BACKLOG 10

#define SERVER_NAME "SERVER"
#define START_ROOM "paradnaya"
#define ERROR "ERROR"
#define SUCCESS "SUCCESS"

#define COMMAND_CREATE_ROOM "#create-room "
#define COMMAND_ENTER_ROOM "#enter-room "
#define COMMAND_SHOW_ROOMS "#show-rooms"
#define COMMAND_EXIT "#exit"


pthread_mutex_t mutex;
pthread_t thread_id;
struct associations *rooms;


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


char *get_rooms_names()
{
    int i, length; 
    char *rooms_names, *tmp1, *tmp2;
    struct nodes *rooms_names_list;
    
    pthread_mutex_lock(&mutex);
    rooms_names_list = associations_keys(rooms);

    length = 0;
    for (i = 0; i < nodes_length(rooms_names_list); i++) {
        tmp1 = (char *)nodes_get(rooms_names_list, i);
        length += strlen(tmp1) + 1;
    }
    
    rooms_names = malloc(length + 1);
    rooms_names[length] = '\0';

    tmp1 = rooms_names;
    for (i = 0; i < nodes_length(rooms_names_list); i++) {
        tmp2 = (char *)nodes_get(rooms_names_list, i);
        length = strlen(tmp2);
        memcpy(tmp1, tmp2, length); 
        tmp2[length] = '\n';
        tmp1 += length + 1;
    }
    pthread_mutex_unlock(&mutex);

    return rooms_names;
}

void *func(void *data) 
{ 
    unsigned char buff[MESSAGE_SIZE]; 
    message_t message;
    int client_fd, i; 
    struct nodes *room;
    char *rooms_names, *room_name, *tmp;
    char roomname[NAME_LENGTH_MAX];

    client_fd = *((int *)data);
    
    for (;;) { 

        bzero(buff, MESSAGE_SIZE); 
        bzero(&message, MESSAGE_SIZE);
   
        read(client_fd, buff, MESSAGE_SIZE); 
        message_deserialize(&message, buff);


        if ((strncmp((char *)message.text, COMMAND_CREATE_ROOM,
                        strlen(COMMAND_CREATE_ROOM))) == 0) {

            //TODO 
            memcpy(roomname, message.room, NAME_LENGTH_MAX);
            tmp = message.text + strlen(COMMAND_ENTER_ROOM);
            i = strlen(tmp);
            if (i > 20) {
                memcpy(message.sender, SERVER_NAME, strlen(SERVER_NAME));
                memcpy(message.text, ERROR, strlen(ERROR);
            }
            else {
                room_name = malloc(i + 1);
                memcpy(room_name, tmp, i + 1);
            }

        }
            //TODO переход в комнату
            //
            //
            //TODO definы перенести в message.h
        else if ((strncmp((char *)message.text, COMMAND_SHOW_ROOMS,
                        strlen(COMMAND_SHOW_ROOMS))) == 0) {

            memcpy(roomname, message.room, NAME_LENGTH_MAX);
            rooms_names = get_rooms_names();

            bzero(buff, MESSAGE_SIZE); 
            bzero(&message, MESSAGE_SIZE);

            memcpy(message.sender, SERVER_NAME, strlen(SERVER_NAME));
            memcpy(message.room, roomname, NAME_LENGTH_MAX);
            memcpy(message.text, rooms_names, strlen(rooms_names);

            message_serialize(&message, buff);

            write(client_fd, buff, MESSAGE_SIZE); 
            free(rooms_names);
        }
        else if ((strncmp((char *)message.text, COMMAND_EXIT,
                        strlen(COMMAND_EXIT))) == 0) {


            memcpy(roomname, message.room, NAME_LENGTH_MAX);
            pthread_mutex_lock(&mutex);

            /* Получаем список дескрипторов участников комнаты */
            room = (struct nodes *)associations_get(rooms, roomname);
            /* Ищем дескриптор данного пользователя */
            for (i = 0; i < nodes_length(room); i++) {
                if (*((int*)nodes_get(room, i)) == client_fd) {
                    break;
                }
            }
            /* Удаляем его из комнаты */
            nodes_remove(room, i, nodes_pass);
            pthread_mutex_unlock(&mutex);

            /* Отправляем сообщение о выходе */
            bzero(buff, MESSAGE_SIZE); 
            bzero(&message, MESSAGE_SIZE);

            memcpy(message.sender, SERVER_NAME, strlen(SERVER_NAME));
            memcpy(message.room, roomname, NAME_LENGTH_MAX);
            memcpy(message.text, COMMAND_EXIT, strlen(COMMAND_EXIT));

            message_serialize(&message, buff);
            write(client_fd, buff, MESSAGE_SIZE); 

            printf("Disconnected.\n");
            break;
        }
        //TODO else отправить сообщения всем участникам комнаты

    } 
    pthread_exit(NULL);
} 


void join_client(int *data)
{
    unsigned char buff[MESSAGE_SIZE]; 
    message_t message;
    int client_fd;
    struct nodes *start_room;
    char *rooms_names;

    client_fd = *data;
    rooms_names = get_rooms_names();

    read(client_fd, buff, MESSAGE_SIZE); 

    bzero(&message, MESSAGE_SIZE);
    bzero(buff, MESSAGE_SIZE); 


    memcpy(message.sender, SERVER_NAME, strlen(SERVER_NAME));
    memcpy(message.room, START_ROOM, strlen(START_ROOM));
    memcpy(message.text, rooms_names, strlen(rooms_names));


    message_serialize(&message, buff);
    write(client_fd, buff, MESSAGE_SIZE); 

    /* Добавление пользователя в комнату */
    pthread_mutex_lock(&mutex);
    start_room = (struct nodes *)associations_get(rooms, START_ROOM);
    nodes_push_back(start_room, data);

    free(rooms_names);
    pthread_mutex_unlock(&mutex);
}


void *handle_client(void *data) {
    int client_fd = *((int *)data);

    printf("Got request for connection.\n");

    join_client(data);

    pthread_create(&thread_id, NULL, func, data);
    pthread_join(thread_id, NULL);

    close(client_fd);
    free(data);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int *client_fd, server_fd, port;
    struct nodes *start_room;

    rooms = associations_new(20);
    start_room = nodes_new();
    associations_set(rooms, START_ROOM, start_room);

    pthread_mutex_init(&mutex, NULL);

    port = argc < 2 ? DEFAULT_PORT : atoi(*(argv + 1));

    server_fd = setup_server(port, DEFAULT_BACKLOG);

    printf("Server listening on port %d\n", port);

    while (1) {
        client_fd = malloc(sizeof(int));

        *client_fd = accept_new_connection(server_fd);

        if (*client_fd < 0) { perror("accept failed"); continue; }

        pthread_create(&thread_id, NULL, handle_client, (void *)client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    associations_free(rooms, free);
    return 0;
}
