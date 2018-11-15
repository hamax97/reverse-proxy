all:
	rm -rf ./bin
	mkdir ./bin
	g++ -c src/tcp_server.cpp
	g++ -ggdb -o bin/server tcp_server.o
	rm tcp_server.o

clean:
	rm -rf *~
