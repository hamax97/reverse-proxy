all:
	rm -rf ./bin
	mkdir ./bin
	g++ -c src/proxy.cpp
	g++ -ggdb -o bin/server proxy.o
	rm proxy.o

clean:
	rm -rf *~
