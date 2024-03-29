// Vasilescu Andrei
// 331 CC
// APD
// Tema 4

#include "mpi.h"
#ifndef HELPERS_FILE
#define HELPERS_FILE
typedef enum {
    STP_REQUEST,
    STP_REPLY,
    BROADCAST_START,
    BROADCAST_NONE,
    BROADCAST_STOP,
    INFORMATION,
    TOPOLOGY,
    LEADER_FINDER
} MessageType;

typedef enum {
    NONE,
    SENDING,
    NO_MESSAGE,
    DONE
} BunkerState;

typedef enum {
    FALSE = 0,
    TRUE = 1
} BOOL;

typedef struct {
    char count;
    char neighbours[254];
} StpReply;

typedef struct {
    int source;
    int destination;
    MessageType type;
    char message[255];
} Message;

typedef struct {
    int count;
    MPI_Request reqs[200];
} Requests;

typedef struct {
    int count;
    int allocatedSpace;
    Message **messages;
} MessageArray;

typedef struct {
    int nodeId;
    int nextHop;
} Destination;

typedef struct {
    int count;
    int allocatedSpace;
    Destination *bunkers;
} RoutingTable;

int BROADCAST_DESTINATION;
int START_COMMUNICATION_TAG;
int END_COMMUNICATION_TAG;
int COMMUNICATION_TAG;
int UNKNOWN_ROUTE;
int UNKNOWN_BUNKER;
int MESSAGE_ALLOCATION_CHUNK;
int ROUTING_ALLOCATION_CHUNK;

MPI_Datatype MPI_DESTINATION;
MPI_Datatype MPI_MESSAGE;
#endif
