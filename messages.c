#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mpi.h"

#include "helpers.h"
#include "messages.h"
#include "routing.h"

FILE *messagesOutputFile = NULL;
// OK
Message *deserializeMessage(char *line) {
    int source, destination;
    sscanf(line, "%d %c", &source, &destination);

    if (destination != BROADCAST_DESTINATION) {
        sscanf(line, "%*d %d", &destination);
    }

    char *message = strchr(line, ' ') + 1;
    message = strchr(message, ' ') + 1;
    Message *newMessage = (Message *)calloc(1, sizeof(Message));
    newMessage->source = source;
    newMessage->destination = destination;
    newMessage->type = INFORMATION;
    strcpy(newMessage->message, message); 

    return newMessage;
}

// OK
Message *createStpRequestMessage(int source) {
    Message *message = (Message *)calloc(1, sizeof(Message));
    message->source = source;
    message->destination = BROADCAST_DESTINATION;
    message->type = STP_REQUEST;

    return message;
}

// OK
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

// OK
Message *createBroadcastMessage(int pid, MessageType messageType) {
    Message *message = (Message *)calloc(1, sizeof(Message));
    message->type = messageType;
    message->source = pid;
    message->destination = BROADCAST_DESTINATION;
    
    return message;
}

// OK
void sendStartingBroadcast(RoutingTable routingTable, int pid, MessageArray messageArray) {
    Message *message;
    
    if (messageArray.count == 0) {
        message = createBroadcastMessage(pid, BROADCAST_NONE);
    } else {
        message = createBroadcastMessage(pid, BROADCAST_START);
    }
   
    broadcastMessage(routingTable, message, START_COMMUNICATION_TAG, UNKNOWN_BUNKER);
}

// OK
void interpretReceivedBroadcast(Message *message, int *communicatingBunkers) {
    switch(message->type) {
        case BROADCAST_START:
            communicatingBunkers[message->source] = SENDING;
            break;
            
        case BROADCAST_NONE:
             communicatingBunkers[message->source] = NO_MESSAGE;
             break;
             
         case BROADCAST_STOP:
             communicatingBunkers[message->source] = DONE;
             break; 
    }
}

// OK
BOOL doneReceivingBroadcasts(int *communicatingBunkers, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (communicatingBunkers[i] == NONE) {
            return FALSE;
        }
    }

    return TRUE;
}

// OK
void receiveStartingBroadcasts(int *communicatingBunkers, RoutingTable routingTable, int pid) {
    BOOL phaseComplete = FALSE;
    communicatingBunkers[pid] = DONE;
    while (!phaseComplete) {
        MPI_Status sts;
        Message *receivedMessage = (Message *)calloc(1, sizeof(Message));
        MPI_Recv(receivedMessage, 1, MPI_MESSAGE, MPI_ANY_SOURCE, START_COMMUNICATION_TAG, MPI_COMM_WORLD, &sts);
        int source = sts.MPI_SOURCE;

        interpretReceivedBroadcast(receivedMessage, communicatingBunkers);
        routeMessage(routingTable, receivedMessage, START_COMMUNICATION_TAG, source);
        
        if (doneReceivingBroadcasts(communicatingBunkers, routingTable.count + 1)) {
            phaseComplete = TRUE;
        }
    }
}

// OK
void sendEndingBroadcast(RoutingTable routingTable, int pid) {
    Message *message = createBroadcastMessage(pid, BROADCAST_STOP);
    broadcastMessage(routingTable, message, END_COMMUNICATION_TAG, UNKNOWN_BUNKER);
}

// OK
void sendMessages(RoutingTable routingTable, MessageArray *messagesArray, int pid) {
    int i;
    //printf("%d messageCount = %d\n", pid, messagesArray->count);
    for (i = 0; i < messagesArray->count; i++) {
       routeMessage(routingTable, messagesArray->messages[i], COMMUNICATION_TAG, pid);
    }
}

// OK
BOOL hasMoreMessages(int *communicatingBunkers, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (communicatingBunkers[i] != DONE) {
            return TRUE;
        }
    }

    return FALSE;
}

// OK
void didReceiveMessage(Message *message, int *communicatingBunkers, int pid, int source, RoutingTable routingTable) {
    // print details in file
// TODO resulting output

    if (message->destination == pid ||
        message->destination == BROADCAST_DESTINATION) {

        if (message->type == BROADCAST_STOP) {
            communicatingBunkers[message->source] = DONE;
        }

       if (message->type == BROADCAST_STOP) {
           printf("%0.2d received DONE broadcast from source %d. Received it from bunker %d. Broadcasting it to all of my children. \n", pid, message->source, source);
       } else {
           printf("%0.2d I received a message from %d. Original source: %d, Content: %s\n", pid, source, message->source, message->message);
       } 
       
    } else {
        int destinationBunker;
        int i;
        for (i = 0; i < routingTable.count; i++) {
            if (message->destination == routingTable.bunkers[i].nodeId) {
                destinationBunker = routingTable.bunkers[i].nextHop;
                break;
            }
        }
        printf("%0.2d received a message from source %d to destination %d. Received it from bunker %d. Sending it to %d.\n", pid, message->source, message->destination, source, destinationBunker);
    }
}

// OK
void receiveMessages(RoutingTable routingTable, int *communicatingBunkers, int pid) {
    while (hasMoreMessages(communicatingBunkers, routingTable.count + 1)) {
        Message *receivedMessage = (Message *)calloc(1, sizeof(Message));
        MPI_Status sts;
        MPI_Recv(receivedMessage, 1, MPI_MESSAGE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &sts);
        int source = sts.MPI_SOURCE;
        int tag = sts.MPI_TAG;
        //printf("%0.2d received message from %d\n", pid, receivedMessage->source);
        didReceiveMessage(receivedMessage, communicatingBunkers, pid, source, routingTable);
        if (receivedMessage->destination != pid) {
            routeMessage(routingTable, receivedMessage, tag, source);
        } else {
            free(receivedMessage);
        }
    }

    if (messagesOutputFile != NULL) {
        fclose(messagesOutputFile);
        messagesOutputFile = NULL;
    }
}

// OK
void checkMessageArrayMemorySize(MessageArray *messageArray) {
    if (messageArray->count >= messageArray->allocatedSpace) {
        int newSize = messageArray->allocatedSpace + MESSAGE_ALLOCATION_CHUNK;
        messageArray->messages = realloc(messageArray->messages, newSize);
        messageArray->allocatedSpace = newSize;
    }
}

// OK
void addMessageToArray(MessageArray *messageArray, Message *message) {
    checkMessageArrayMemorySize(messageArray);

    messageArray->messages[messageArray->count] = message;
    messageArray->count++;
}

// OK
void initMessagesArray(MessageArray *messageArray) {
    messageArray->allocatedSpace = ROUTING_ALLOCATION_CHUNK;
    messageArray->count = 0;
    messageArray->messages = (Message **)calloc(ROUTING_ALLOCATION_CHUNK, sizeof(Message *));
}

// OK
void readMessages(char *inputFilename, MessageArray *messageArray, int pid) {
    FILE *inputFile = fopen(inputFilename, "r");
    char *line = NULL;
    size_t len = 0;
    int lineCount;

    initMessagesArray(messageArray);

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
        len = 0;
        line = NULL;
    }
}

// OK
void printMessages(MessageArray *messageArray, int pid) {
    char outputFilename[20];
    sprintf(outputFilename, "mesaje - %d", pid);

    FILE *outputFile = fopen(outputFilename, "w");

    int i;
    for (i = 0; i < messageArray->count; i++) {
        Message *currentMessage = messageArray->messages[i];
        fprintf(outputFile, "%d %d %s", currentMessage->source, currentMessage->destination, currentMessage->message);
    }

    fclose(outputFile);
}
