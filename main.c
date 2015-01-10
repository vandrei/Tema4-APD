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

int findLeader(RoutingTable routingTable, int pid, int mainLeader) {
    int receivedWakeups = 0;

    int leader = pid;
    
    int neighbours = countNeighbours(routingTable);
    int recvNeighbours[neighbours];

    while (receivedWakeups < neighbours - 1) {
        Message receivedMessage;
        MPI_Status sts;
        MPI_Recv(&receivedMessage, 1, MPI_MESSAGE, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &sts);
        recvNeighbours[receivedWakeups] = sts.MPI_SOURCE;
        receivedWakeups++;
        
        int recvLeader = receivedMessage.message[0];
        if (recvLeader < leader) {
            leader = recvLeader;
        } else if (leader == mainLeader) {
            leader = recvLeader;
        }
    }
    
    Message sendMessage;
    sendMessage.message[0] = leader;
    int destination = 0;
    int i;
    for (i = 0; i < routingTable.count; i++) {
        if (bunkerIsNeighbour(routingTable.bunkers[i])) {
            BOOL foundDestination = TRUE;
            destination = routingTable.bunkers[i].nextHop;

            int j;
            for (j = 0; j < neighbours - 1; j++) {
                if (destination == recvNeighbours[j]) {
                    foundDestination = FALSE;
                }
            }

            if (foundDestination) {
                break;
            }
        }
    }

    MPI_Send(&sendMessage, 1, MPI_MESSAGE, destination, 4, MPI_COMM_WORLD);

    Message *resultMessage = (Message *)calloc(1, sizeof(Message));
    MPI_Status sts;
    MPI_Recv(resultMessage, 1, MPI_MESSAGE, destination, 4, MPI_COMM_WORLD, &sts);
    
    if (leader == mainLeader) {
        leader = resultMessage->message[0];
    } else if (resultMessage->message[0] < leader && resultMessage->message[0] > mainLeader) {
        leader = resultMessage->message[0];
    } else {
        resultMessage->message[0] = leader;
    }

    broadcastMessage(routingTable, resultMessage, 4, destination);
    
    return leader;
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

        sendStartingBroadcast(routingTable, pid, messageArray);
        receiveStartingBroadcasts(communicatingBunkers, routingTable, pid);

        sendMessages(routingTable, &messageArray, pid);
        sendEndingBroadcast(routingTable, pid);
        
        receiveMessages(routingTable, communicatingBunkers, pid);

        int leader = findLeader(routingTable, pid, UNKNOWN_BUNKER);
        int secondLeader = findLeader(routingTable, pid, leader);

        printf("%0.2d's leader is %d and deputy %d\n", pid, leader, secondLeader);
    MPI_Finalize();

    return 0;
}
