#ifndef LAST_MESSAGES_ID_H
#define LAST_MESSAGES_ID_H

#define BUFFER_SIZE 16

typedef struct {
    int last_ids[BUFFER_SIZE];
    int current_idx;
} LastMessagesIds;

#endif