#include "helpers.h"
#ifndef MESSAGES_HEADER
#define MESSAGES_HEADER

Message *createStpRequestMessage(int source);
Message *createStpReplyMessage(RoutingTable *routingTable, int source);

#endif
