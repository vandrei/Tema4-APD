#include "helpers.h"
#ifndef MESSAGES_HEADER
#define MESSAGES_HEADER

Message *createStpRequestMessage(int source);
Message *createStpReplyMessage(RoutingTable *routingTable, int source);
void sendStartingBroadcast(RoutingTable routingTable, int pid, MessageArray messageArray);
void sendEndingBroadcast(RoutingTable routingTable, int pid);
void receiveStartingBroadcasts(int *communicatingBunkers, RoutingTable routingTable, int pid);
void sendMessages(RoutingTable routingTable, MessageArray *messagesArray, int pid);
  
#endif
