#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"

#include "helpers.h"

#include "routing.h"
#include "messages.h"

void initMPIDataStructure() {
    MPI_Datatype oldMessageTypes[2], oldDestinationTypes[1];
    int messageBlockCounts[2], destinationBlockCounts[1];
    MPI_Aint messageOffsets[2], destinationOffsets[1], extent;
    
    // Define Message structure for MPI
    messageOffsets[0] = 0;
    oldMessageTypes[0] = MPI_INT;
    messageBlockCounts[0] = 3;
    MPI_Type_extent(MPI_INT, &extent);
    messageOffsets[1] = messageBlockCounts[0] * extent;
    oldMessageTypes[1] = MPI_CHAR;
    messageBlockCounts[1] = 255;
    MPI_Type_struct(1, messageBlockCounts, messageOffsets, oldMessageTypes, &MPI_MESSAGE);
    MPI_Type_commit(&MPI_MESSAGE);
}

void initConstantValues() {
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
        
        RoutingTable routingTable;
        MessageArray messageArray;

        initConstantValues();

        MPI_Comm_rank(MPI_COMM_WORLD, &pid);
        initMPIDataStructure();
        
        readNeighbours(topologyFile, &routingTable, pid);
        
        //
        //
        if (pid == 0) {
            requestSTP(&routingTable, pid);
            sendTopology(&routingTable, pid, UNKNOWN_BUNKER);
        } else {
            helpSTP(&routingTable, pid);
            receiveTopology(&routingTable, pid);
        }

        printRoutingTable(routingTable, pid);
        
//        readMessages(messagesFile, &messageArray, pid);

// begin phase 2
/*
        sendStartingBroadcast();
        receiveStartingBroadcasts();
        
        sendMessages(&messageArray);
        receiveMessages();
        
        receiveEndingBroadcast();
        sendEndingBroadcast();
 */       
    MPI_Finalize();

    return 0;
}
