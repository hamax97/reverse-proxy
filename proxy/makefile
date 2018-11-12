all:
	rm -rf ./bin
	mkdir ./bin
	g++ -c `mysql_config --cflags` src/database_controller.cpp src/tcp_server.cpp
	g++ -ggdb -o bin/server database_controller.o tcp_server.o `mysql_config --libs`
	rm tcp_server.o database_controller.o

clean:
	rm -rf *~
