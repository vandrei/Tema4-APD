#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "mpi.h"

#include "helpers.h"

#include "messages.h"

Message *deserializeMessage(char *line) {
    int source, destination;
    char message[255];
    sscanf(line, "%d %d %s", &source, &destination, &message);

    Message *newMessage = (Message *)calloc(1, sizeof(Message));
    newMessage->source = source;
    newMessage->destination = destination;
    newMessage->type = INFORMATION;
    strcpy(newMessage->message, message); 

    return newMessage;
}

Message *createStpRequestMessage(int source) {
    Message *message = (Message *)calloc(1, sizeof(Message));
    message->source = source;
    message->destination = BROADCAST_DESTINATION;
    message->type = STP_REQUEST;

    return message;
}

Message *createStpReplyMessage(RoutingTable *routingTable, int source) {
    Message *message = (Message *)calloc(1, sizeof(Message));
    message->type = STP_REPLY;
    StpReply *replyData = (StpReply *)&message->message;

    replyData->count = routingTable->count - 1;
    int i;
    int j = 0;
    for (i = 0; i < routingTable->count; i++) {
        if (routingTable->bunkers[i].nodeId != source) {
            replyData->neighbours[j] = routingTable->bunkers[i].nodeId;
            j++;
        }
    }

    return message;
}
void addMessageToArray(MessageArray *messageArray, Message *message) {
    if (messageArray->count >= messageArray->allocatedSpace) {
        int newSize = messageArray->allocatedSpace + MESSAGE_ALLOCATION_CHUNK;
        messageArray->messages = realloc(messageArray->messages, newSize);
        messageArray->allocatedSpace = newSize;
    }

    messageArray->messages[messageArray->count] = message;
    messageArray->count++;
}

void readMessages(char *inputFilename, MessageArray *messageArray, int pid) {
    FILE *inputFile = fopen(inputFilename, "w");
    char *line = NULL;
    size_t len = 0;
    int lineCount;
    fscanf(inputFile, "%d\n", &lineCount);
    int i;
    for (i = 0; i < lineCount; i++) {
        getline(&line, &len, inputFile);
        Message *message = deserializeMessage(line);
        if (message->source == pid) {
            addMessageToArray(messageArray, message);
        } else {
            free(message);
        }

        free(line);
        size_t len = 0;
        line = NULL;
    }
}
