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


static int name_is_valid(const char *name)
{
    int l;
    l = strlen(name);
    if(l < NAME_LENGTH_MIN || l > NAME_LENGTH_MAX) return 0;
    return 1;
}

char *get_rooms_names()
{
    int i, length; 
    char *rooms_names, *tmp1, *tmp2;
    struct nodes *rooms_names_list;
    
    pthread_mutex_lock(&mutex);
    rooms_names_list = (struct nodes *)associations_keys(rooms);

    length = 1;
    for (i = 0; i < nodes_length(rooms_names_list); i++) {
        tmp1 = (char *)nodes_get(rooms_names_list, i);
        length += strlen(tmp1) + 1;
    }

    rooms_names = malloc(length + 1);
    rooms_names[length] = '\0';

    *rooms_names = '\n';
    tmp1 = rooms_names + 1;
    for (i = 0; i < nodes_length(rooms_names_list); i++) {
        tmp2 = (char *)nodes_get(rooms_names_list, i);
        length = strlen(tmp2);
        memcpy(tmp1, tmp2, length); 
        tmp1[length] = '\n';
        tmp1 += length + 1;
    }
    pthread_mutex_unlock(&mutex);

    return rooms_names;
}

int room_exists(char *roomname)
{
    int exists, i, length;
    char *tmp;
    struct nodes *rooms_names_list;

    if(!name_is_valid(roomname)) {
        return 0;
    }
    
    length = strlen(roomname);
    exists = 0;

    pthread_mutex_lock(&mutex);
    rooms_names_list = (struct nodes *)associations_keys(rooms);

    for (i = 0; i < nodes_length(rooms_names_list); i++) {
        tmp = (char *)nodes_get(rooms_names_list, i); 

        if (length == strlen(tmp)) {
            if ((strncmp(tmp, roomname, length)) == 0) {
                exists = 1;  
                break;
            }
        }
    }

    pthread_mutex_unlock(&mutex);

    return exists;
}



void response_enter_room(message_t *message,
        unsigned char *buff, void *client_fd_ptr)
{
    int i;
    struct nodes *room;
    char roomname[NAME_LENGTH_MAX + 1]; 
    char *roomname_new;

    roomname_new = (char *)message->text + strlen(COMMAND_ENTER_ROOM);

    if (name_is_valid(roomname_new)) {

        memcpy(roomname, roomname_new, NAME_LENGTH_MAX);
        roomname[NAME_LENGTH_MAX] = '\0';

        /* Удаляем пользователя из текущей из комнаты */
        pthread_mutex_lock(&mutex);
        room = (struct nodes *)associations_get(rooms, (char *)message->room);
        for (i = 0; i < nodes_length(room); i++) {
            if (*((int*)nodes_get(room, i)) == *((int *)client_fd_ptr)) {
                nodes_remove(room, i, nodes_pass);
                break;
            }
        }
        pthread_mutex_unlock(&mutex);


        /* Если комната существует, добавляем в нее пользователя */
        if (room_exists(roomname)) {
            pthread_mutex_lock(&mutex);
            room = associations_get(rooms, roomname);
            nodes_push_back(room, client_fd_ptr);
            pthread_mutex_unlock(&mutex);
        }
        /* Иначе создаем новую комнату */
        else {
            pthread_mutex_lock(&mutex);
            room = nodes_new();
            associations_set(rooms, roomname, room);
            /* Добавляем дескриптор в новую комнату */
            nodes_push_back(room, client_fd_ptr);
            pthread_mutex_unlock(&mutex);
        }

        bzero(buff, MESSAGE_SIZE); 
        bzero(message, MESSAGE_SIZE);

        memcpy(message->text, SUCCESS, strlen(SUCCESS));

    }
    else {
        memcpy(roomname, message->room, NAME_LENGTH_MAX);

        bzero(buff, MESSAGE_SIZE); 
        bzero(message, MESSAGE_SIZE);

        memcpy(message->text, ERROR, strlen(ERROR));

    }

    memcpy(message->sender, SERVER_NAME, strlen(SERVER_NAME));
    memcpy(message->room, roomname, NAME_LENGTH_MAX);

    message_serialize(message, buff);
    write(*((int *)client_fd_ptr), buff, MESSAGE_SIZE); 
}

void response_show_rooms(message_t *message,
        unsigned char *buff, void *client_fd_ptr)
{
    char roomname[NAME_LENGTH_MAX];
    char *rooms_names;

    memcpy(roomname, message->room, NAME_LENGTH_MAX);
    rooms_names = get_rooms_names();

    bzero(buff, MESSAGE_SIZE); 
    bzero(message, MESSAGE_SIZE);

    memcpy(message->sender, SERVER_NAME, strlen(SERVER_NAME));
    memcpy(message->room, roomname, NAME_LENGTH_MAX);
    memcpy(message->text, rooms_names, strlen(rooms_names));

    free(rooms_names);

    message_serialize(message, buff);
    write(*((int *)client_fd_ptr), buff, MESSAGE_SIZE); 
}



void response_exit(message_t *message,
        unsigned char *buff, void *client_fd_ptr)
{
    int i;
    struct nodes *room;
    char roomname[NAME_LENGTH_MAX];

    memcpy(roomname, message->room, NAME_LENGTH_MAX);
    pthread_mutex_lock(&mutex);

    /* Получаем список дескрипторов участников комнаты */
    room = (struct nodes *)associations_get(rooms, roomname);
    /* Ищем дескриптор данного пользователя */
    for (i = 0; i < nodes_length(room); i++) {
        if (*((int*)nodes_get(room, i)) == *((int *)client_fd_ptr)) {
          nodes_remove(room, i, nodes_pass);
          break;
        }
    }
    /* Удаляем его из комнаты */
    pthread_mutex_unlock(&mutex);

    /* Отправляем сообщение о выходе */
    bzero(buff, MESSAGE_SIZE); 
    bzero(message, MESSAGE_SIZE);

    memcpy(message->sender, SERVER_NAME, strlen(SERVER_NAME));
    memcpy(message->room, roomname, NAME_LENGTH_MAX);
    memcpy(message->text, COMMAND_EXIT, strlen(COMMAND_EXIT));

    message_serialize(message, buff);
    write(*((int *)client_fd_ptr), buff, MESSAGE_SIZE); 
}


void response_send_message(message_t *message,
        unsigned char *buff, void *client_fd_ptr)
{
    int i, receiver_fd;
    struct nodes *room;

    pthread_mutex_lock(&mutex);

    /* Получаем список дескрипторов участников комнаты */
    room = (struct nodes *)associations_get(rooms, (char *)message->room);
    for (i = 0; i < nodes_length(room); i++) {
        receiver_fd = *((int*)nodes_get(room, i));
        if (receiver_fd != *((int *)client_fd_ptr)) {
            write(receiver_fd, buff, MESSAGE_SIZE); 
        }
    }
    pthread_mutex_unlock(&mutex);

    bzero(buff, MESSAGE_SIZE); 
    bzero(message, MESSAGE_SIZE);
}



void *func(void *client_fd_ptr) 
{ 
    unsigned char buff[MESSAGE_SIZE]; 
    message_t message;
    int client_fd; 

    client_fd = *((int *)client_fd_ptr);
    
    for (;;) { 

        bzero(buff, MESSAGE_SIZE); 
        bzero(&message, MESSAGE_SIZE);
   
        read(client_fd, buff, MESSAGE_SIZE); 
        message_deserialize(&message, buff);

        /* Войти в комнату */
        if ((strncmp((char *)message.text, COMMAND_ENTER_ROOM,
                        strlen(COMMAND_ENTER_ROOM))) == 0) {
            response_enter_room(&message, buff, client_fd_ptr);
        }
        /* Показать комнаты */
        else if ((strncmp((char *)message.text, COMMAND_SHOW_ROOMS,
                        strlen(COMMAND_SHOW_ROOMS))) == 0) {

            response_show_rooms(&message, buff, client_fd_ptr);
        }
        /* Выйти */
        else if ((strncmp((char *)message.text, COMMAND_EXIT,
                        strlen(COMMAND_EXIT))) == 0) {

            response_exit(&message, buff, client_fd_ptr);
            break;
        }
        /* Отправить сообщение */
        else {
            response_send_message(&message, buff, client_fd_ptr);
        }

    } 
    pthread_exit(NULL);
} 


void join_client(int *client_fd_ptr)
{
    unsigned char buff[MESSAGE_SIZE]; 
    message_t message;
    int client_fd;
    struct nodes *start_room;
    char *rooms_names;

    client_fd = *client_fd_ptr;
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
    nodes_push_back(start_room, client_fd_ptr);

    free(rooms_names);
    pthread_mutex_unlock(&mutex);
}


void *handle_client(void *client_fd_ptr) {
    int client_fd = *((int *)client_fd_ptr);

    printf("Got request for connection.\n");

    join_client(client_fd_ptr);

    pthread_create(&thread_id, NULL, func, client_fd_ptr);
    pthread_join(thread_id, NULL);

    close(client_fd);
    free(client_fd_ptr);
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
