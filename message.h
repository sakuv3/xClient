#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#define KEY 1234L
#define PERM 0666

typedef struct _message{
    uint16_t len;
    char text[4095], name[32];
}MSG;

#endif /* MSGQUEUE_H */
