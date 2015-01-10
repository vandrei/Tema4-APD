#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mpi.h"

#include "helpers.h"

#include "messages.h"
#include "routing.h"

// OK
void exchangeDestinations(Destination *bunker1, Destination *bunker2) {
    Destination auxDestination;
    auxDestination.nodeId = bunker1->nodeId;
    auxDestination.nextHop = bunker1->nextHop;

    bunker1->nodeId = bunker2->nodeId;
    bunker1->nextHop = bunker2->nextHop;

    bunker2->nodeId = auxDestination.nodeId;
    bunker2->nextHop = auxDestination.nextHop;
}

// OK
void sortRoutingTable(RoutingTable *routingTable) {
    int i;
    for (i = 0; i < routingTable->count; i++) {
        int j;
        for (j = i + 1; j < routingTable->count; j++) {
            if (routingTable->bunkers[i].nodeId > routingTable->bunkers[j].nodeId) {
                exchangeDestinations(&routingTable->bunkers[i], &routingTable->bunkers[j]);
            }
        }
    }
}

// OK
void checkTableMemorySpace(RoutingTable *routingTable) {
    if (routingTable->count >= routingTable->allocatedSpace) {
        int newSize = routingTable->allocatedSpace + ROUTING_ALLOCATION_CHUNK;
        routingTable->bunkers = realloc(routingTable->bunkers, newSize);
        routingTable->allocatedSpace = newSize;
    }
}

// OK
void addNeighbourToTable(int neighbourId, RoutingTable *routingTable) {
    checkTableMemorySpace(routingTable);

    routingTable->bunkers[routingTable->count].nodeId = neighbourId;
    routingTable->bunkers[routingTable->count].nextHop = neighbourId;
    routingTable->count++;
    sortRoutingTable(routingTable);
}

// OK
BOOL shouldAddBunker(RoutingTable *routingTable, int bunkerId) {
    int i = 0;
    while (i < routingTable->count && routingTable->bunkers[i].nodeId <= bunkerId) {
        if (routingTable->bunkers[i].nodeId == bunkerId) {
            return FALSE;
        }
        i++;
    }

    return TRUE;
}

// OK
BOOL bunkerIsNeighbour(Destination bunker) {
    if (bunker.nextHop == bunker.nodeId) {
        return TRUE;
    }

    return FALSE;
}

// OK
void addBunkerToTable(int bunkerId, int nextHop, RoutingTable *routingTable) {
    if (shouldAddBunker(routingTable, bunkerId)) {
        checkTableMemorySpace(routingTable);
    
        routingTable->bunkers[routingTable->count].nodeId = bunkerId;
        routingTable->bunkers[routingTable->count].nextHop = nextHop;
        routingTable->count++;
        sortRoutingTable(routingTable);
    }
}

// OK
void initRoutingTable(RoutingTable *routingTable) {
    routingTable->allocatedSpace = ROUTING_ALLOCATION_CHUNK;
    routingTable->count = 0;
    routingTable->bunkers = (Destination *)calloc(ROUTING_ALLOCATION_CHUNK, sizeof(Destination));
}

// OK
void readNeighbours(char *inputFilename, RoutingTable *routingTable, int myPid) {
    FILE *inputFile = fopen(inputFilename, "r");
    BOOL foundMyLine = FALSE;
    int pid;
    char *line = NULL;

    initRoutingTable(routingTable);
    
    while (!foundMyLine) {
        size_t len = 0;
        line = NULL;
        int read = getline(&line, &len, inputFile);
        if (read == 0) {
            break;
        }
        sscanf(line, "%d", &pid);
        if (pid == myPid) {
            foundMyLine = TRUE;
        } else {
            free(line);
        }
    }

    if (line != NULL) {
        int neighbourId;
        char *tokenPosition = strtok(line, "-");
        tokenPosition = strtok(NULL, " ");
        while (tokenPosition) {
            sscanf(tokenPosition, "%d", &neighbourId);
            addNeighbourToTable(neighbourId, routingTable);
            tokenPosition = strtok(NULL, " ");
        }
    }
}

// OK
void printRoutingTable(RoutingTable routingTable, int pid) {
    char outputFilename[20];
    sprintf(outputFilename, "RoutingTable - %d", pid);

    FILE * outputFile = fopen(outputFilename, "w");
    fprintf(outputFile, "%d\n", pid);
    int i;
    for (i = 0; i < routingTable.count; i++) {
        fprintf(outputFile, "%d %d\n", routingTable.bunkers[i].nextHop, 
                                       routingTable.bunkers[i].nodeId);
    }

    fclose(outputFile);
}

// OK
void parseTopologyMessage(Message *stpReply, int source, RoutingTable *routingTable) {
    StpReply *connections = (StpReply *)&stpReply->message;
    int i;
    for (i = 0; i < (int)connections->count; i++) {
        int bunker = (int)connections->neighbours[i];
        addBunkerToTable(bunker, source, routingTable);
    }
}

// OK
void helpSTP(RoutingTable *routingTable, int pid) {
    Message stpRequest;
    MPI_Status sts;
    // Receive initial request from parent
    MPI_Recv(&stpRequest, 1, MPI_MESSAGE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &sts);
    int source = sts.MPI_SOURCE;
    
    // Forward request to children
    MPI_Request reqs[routingTable->count];
    int i;
    for (i = 0; i < routingTable->count; i++) {
        if (routingTable->bunkers[i].nextHop != source) {
            MPI_Isend(&stpRequest, 1, MPI_MESSAGE, routingTable->bunkers[i].nextHop, 0, MPI_COMM_WORLD, &reqs[i]);
        }
    }

    // Obtain reply from children
    RoutingTable oldRoutingTable = *routingTable;
    for (i = 0; i < oldRoutingTable.count; i++) {
        MPI_Status sts;
        if (oldRoutingTable.bunkers[i].nextHop != source) {
            Message stpReply;
            MPI_Recv(&stpReply, 1, MPI_MESSAGE, oldRoutingTable.bunkers[i].nextHop, 0, MPI_COMM_WORLD, &sts);
            parseTopologyMessage(&stpReply, oldRoutingTable.bunkers[i].nextHop, routingTable);
        }
    }

    // Send connections to my parent
    Message *stpReply = createStpReplyMessage(routingTable, source);
    MPI_Send(stpReply, 1, MPI_MESSAGE, source, 0, MPI_COMM_WORLD);
    free(stpReply);
}

// OK
void sendTopology(RoutingTable *routingTable, int pid, int source) {
    int i;
    for (i = 0; i < routingTable->count; i++) {
        if (routingTable->bunkers[i].nodeId != source) {
            if (bunkerIsNeighbour(routingTable->bunkers[i])) {
                Message *topologyMessage = createStpReplyMessage(routingTable, routingTable->bunkers[i].nodeId);
                MPI_Send(topologyMessage, 1, MPI_MESSAGE, routingTable->bunkers[i].nextHop, 0, MPI_COMM_WORLD);
                free(topologyMessage);
            }
        }
    }
}

// OK
void receiveTopology(RoutingTable *routingTable, int pid) {
    MPI_Status sts;
    Message receivedTopologyMessage;
    MPI_Recv(&receivedTopologyMessage, 1, MPI_MESSAGE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &sts);
    int source = sts.MPI_SOURCE;
    
    parseTopologyMessage(&receivedTopologyMessage, source, routingTable);

    sendTopology(routingTable, pid, source);
}

// OK
void requestSTP(RoutingTable *routingTable, int pid) {
    int i;
    Message *stpRequest = createStpRequestMessage(pid);
    for (i = 0; i < routingTable->count; i++) {
        MPI_Send(stpRequest, 1, MPI_MESSAGE, routingTable->bunkers[i].nextHop, 0, MPI_COMM_WORLD);
    }
    free(stpRequest);

    Message stpReply;
    RoutingTable oldRoutingTable = *routingTable;
    for (i = 0; i < oldRoutingTable.count; i++) {
        MPI_Status sts;
        MPI_Recv(&stpReply, 1, MPI_MESSAGE, oldRoutingTable.bunkers[i].nextHop, 0, MPI_COMM_WORLD, &sts);
        parseTopologyMessage(&stpReply, oldRoutingTable.bunkers[i].nextHop, routingTable);
    }
}

int countNeighbours(RoutingTable routingTable) {
    int i;
    int count;
    for (i = 0; i < routingTable.count; i++) {
        if (bunkerIsNeighbour(routingTable.bunkers[i])) {
            count++;
        }
    }

    return count;
}

void broadcastMessage(RoutingTable routingTable, Message *message, int tag, int source) {
    MPI_Request reqs[routingTable.count]; 
    int i;
    for (i = 0; i < routingTable.count; i++) {
        if (bunkerIsNeighbour(routingTable.bunkers[i]) && routingTable.bunkers[i].nextHop != source) {
            MPI_Isend(message, 1, MPI_MESSAGE, routingTable.bunkers[i].nextHop, tag, MPI_COMM_WORLD, &reqs[i]);
        } 
    }
}

void routeMessage(RoutingTable routingTable, Message *message, int tag, int source) {
    if (message->destination == BROADCAST_DESTINATION) {
        broadcastMessage(routingTable, message, tag, source);
    } else {
        BOOL foundDestination = FALSE;

        int i;
        while (i < routingTable.count && !foundDestination) {
            if (routingTable.bunkers[i].nodeId == message->destination) {
                foundDestination = TRUE;
            } else {
                i++;
            }
        }
        
        if (foundDestination) {
            MPI_Request req;
            MPI_Isend(message, 1, MPI_MESSAGE, routingTable.bunkers[i].nextHop, tag, MPI_COMM_WORLD, &req);
        }
    }
}
