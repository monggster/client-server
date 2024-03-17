#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MESSAGE_SIZE 1000
#define NAME_SIZE 20
#define TEXT_SIZE MESSAGE_SIZE - 2 * NAME_SIZE

typedef struct {
    unsigned char sender[NAME_SIZE];
    unsigned char room[NAME_SIZE];
    unsigned char text[TEXT_SIZE];
} message_t;

void message_serialize(const message_t *message, unsigned char *buffer);

void message_deserialize(message_t *message, const unsigned char *buffer);

void message_print(const message_t *message);

#endif
