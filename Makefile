build:
	mpicc main.c messages.c routing.c

clean:
	rm -f RoutingTable\ -\ * a.out
