all:	main.cpp
	g++ main.cpp tcpconnection.h tcpconnection.cpp -o AServer -std=c++11 -lpthread

clean:
	rm *.o
