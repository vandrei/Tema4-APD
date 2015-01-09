#include <stdio.h>
#include <stdlib.h>

//#include "routing.c"
//#include "messages.c"

#include "helpers.h"

int main (void) {
    RoutingTable routingTable;

    readNeighbours("input1", &routingTable, 8);
    printf("did read neighbours\n");
    printRoutingTable(routingTable, 8);
    printf("did print routing table\n");
    return 0;
}
