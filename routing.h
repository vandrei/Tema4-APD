#include "helpers.h"
#ifndef ROUTING_HEADER
#define ROUTING_HEADER

void readNeighbours(char *inputFilename, RoutingTable *routingTable, int myPid);
void printRoutingTable(RoutingTable routingTable, int pid);
void helpSTP(RoutingTable *routingTable, int pid);
void sendTopology(RoutingTable *routingTable, int pid, int source);
void receiveTopology(RoutingTable *routingTable, int pid); 
void requestSTP(RoutingTable *routingTable, int pid);
void routeMessage(RoutingTable routingTable, Message *message, int tag, int source); 
BOOL bunkerIsNeighbour(Destination bunker);
int countNeighbours(RoutingTable routingTable); 
void broadcastMessage(RoutingTable routingTable, Message *message, int tag, int source);
 
#endif
