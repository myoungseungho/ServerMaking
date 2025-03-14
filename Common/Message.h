#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstring>

struct Message {
    char command[BUFFER_SIZE];

    Message() {
        memset(command, 0, BUFFER_SIZE);
    }

    Message(const char* cmd) {
        strncpy(command, cmd, BUFFER_SIZE);
    }
};

#endif // MESSAGE_H
