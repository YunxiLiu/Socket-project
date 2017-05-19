#Makefile for the socket program

server_or: server_or.o
	g++ server_or.o -o server_or
	./server_or

server_and: server_and.o
	g++ server_and.o -o server_and
	./server_and

edge:
	g++ edge.o -o edge
	./edge

all: server_or.o server_and.o edge.o client

client: 
	g++ -o client client.cpp

server_or.o: server_or.cpp
	g++ -c server_or.cpp

server_and.o: server_and.cpp
	g++ -c server_and.cpp

edge.o: edge.cpp
	g++ -c edge.cpp