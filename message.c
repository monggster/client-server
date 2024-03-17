#include <stdio.h>
#include <string.h>
#include "message.h"


// Функция сериализации сообщения
void message_serialize(const message_t *message, unsigned char *buffer) {
    bzero(buffer, MESSAGE_SIZE);

    memcpy(buffer, message->sender, NAME_SIZE);
    buffer += NAME_SIZE;

    memcpy(buffer, message->room, NAME_SIZE);
    buffer += NAME_SIZE;

    memcpy(buffer, message->text, TEXT_SIZE);
}

// Функция десериализации сообщения
void message_deserialize(message_t *message, const unsigned char *buffer) {
    memcpy(message->sender, buffer, NAME_SIZE);
    buffer += NAME_SIZE;

    memcpy(message->room, buffer, NAME_SIZE);
    buffer += NAME_SIZE;

    memcpy(message->text, buffer, TEXT_SIZE);
}

void message_print(const message_t *message)
{
    printf("Sender: %s\n", message->sender);
    printf("Room: %s\n", message->room);
    printf("Text: %s\n", message->text);
}

