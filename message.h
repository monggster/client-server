#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MESSAGE_SIZE 1000
#define NAME_SIZE 20
#define TEXT_SIZE MESSAGE_SIZE - 2 * NAME_SIZE

#define COMMAND_ENTER_ROOM "#enter-room "
#define COMMAND_SHOW_ROOMS "#show-rooms"
#define COMMAND_EXIT "#exit"

#define NAME_LENGTH_MIN 4
#define NAME_LENGTH_MAX 20

#if NAME_LENGTH_MIN > NAME_LENGTH_MAX
#error NAME_LENGTH_MAX must be bigger than NAME_LENGTH_MIN
#elif NAME_LENGTH_MAX > NAME_SIZE
#error NAME_LENGTH_MAX can not be bigger than NAME_SIZE
#endif

#if TEXT_SIZE < 200
#error TEXT_SIZE is too small
#endif

typedef struct {
    unsigned char sender[NAME_SIZE];
    unsigned char room[NAME_SIZE];
    unsigned char text[TEXT_SIZE];
} message_t;

void message_serialize(const message_t *message, unsigned char *buffer);

void message_deserialize(message_t *message, const unsigned char *buffer);

void message_print(const message_t *message);

#endif
