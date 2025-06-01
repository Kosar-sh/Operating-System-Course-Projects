all : server.out client.out
server.out: server.o 
	g++ server.o -o server.out

client.out: client.o
	g++ client.o -o client.out

client.o: client.cpp
	g++ -c client.cpp

server.o: server.cpp 
	g++ -c server.cpp
clean:
	rm *.o
