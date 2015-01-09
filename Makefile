build:
	mpicc main.c messages.c routing.c

clean:
	rm -f RoutingTable* mesaje* a.out receivedMessages*

junk:
	rm -f RoutingTable* mesaje* receivedMessage* file* logfile*
