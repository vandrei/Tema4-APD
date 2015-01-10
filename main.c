#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"

#include "helpers.h"

#include "routing.h"
#include "messages.h"

void initMPIDataStructure() {
    MPI_Datatype oldMessageTypes[2];
    int messageBlockCounts[2];
    MPI_Aint messageOffsets[2], extent;
    
    // Define Message structure for MPI
    messageOffsets[0] = 0;
    oldMessageTypes[0] = MPI_INT;
    messageBlockCounts[0] = 3;
    MPI_Type_extent(MPI_INT, &extent);
    messageOffsets[1] = messageBlockCounts[0] * extent;
    oldMessageTypes[1] = MPI_CHAR;
    messageBlockCounts[1] = 255;
    MPI_Type_struct(2, messageBlockCounts, messageOffsets, oldMessageTypes, &MPI_MESSAGE);
    MPI_Type_commit(&MPI_MESSAGE);
}

void initConstantValues() {
    START_COMMUNICATION_TAG = 1;
    END_COMMUNICATION_TAG = 3;
    COMMUNICATION_TAG = 2;
    BROADCAST_DESTINATION = 'B';
    UNKNOWN_ROUTE = -1;
    UNKNOWN_BUNKER = -1;
    MESSAGE_ALLOCATION_CHUNK = 20;
    ROUTING_ALLOCATION_CHUNK = 20;
}

int main (int argc, char **argv) {
    MPI_Init(&argc, &argv);
        char *topologyFile = argv[1];
        char *messagesFile = argv[2];
        int pid;
        int *communicatingBunkers;
        
        RoutingTable routingTable;
        MessageArray messageArray;

        initConstantValues();

        MPI_Comm_rank(MPI_COMM_WORLD, &pid);
        initMPIDataStructure();
        
        readNeighbours(topologyFile, &routingTable, pid);
        
        if (pid == 0) {
            requestSTP(&routingTable, pid);
            sendTopology(&routingTable, pid, UNKNOWN_BUNKER);
        } else {
            helpSTP(&routingTable, pid);
            receiveTopology(&routingTable, pid);
        }

        communicatingBunkers = (int *)calloc(routingTable.count + 1, sizeof(int));

        printRoutingTable(routingTable, pid);
        
        readMessages(messagesFile, &messageArray, pid);
        //printMessages(&messageArray, pid);

        sendStartingBroadcast(routingTable, pid, messageArray);
        receiveStartingBroadcasts(communicatingBunkers, routingTable, pid);

        sendMessages(routingTable, &messageArray, pid);
        sendEndingBroadcast(routingTable, pid);
        
        receiveMessages(routingTable, communicatingBunkers, pid);

        // phase 2 complete

    MPI_Finalize();

    return 0;
}
