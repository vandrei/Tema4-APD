build:
	mpicc main.c messages.c routing.c

clean:
	rm -f RoutingTable* mesaje* a.out receivedMessages*

cleanOutput:
	rm -f RoutingTable*
