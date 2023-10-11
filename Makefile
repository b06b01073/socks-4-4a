all:
	g++ socks_server.cpp -o socks_server -std=c++14 -pedantic -pthread -lboost_system
	g++ console.cpp -o hw4.cgi -std=c++14 -pedantic -pthread -lboost_system