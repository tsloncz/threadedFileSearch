all: lab3.o
	g++ -g -pthread -o lab3 lab3.o 

lab3.o: lab3.cpp utilities.cpp 
	g++ -Wall -g -O2 -c -pthread lab3.cpp

clean:
	rm -rf lab3 *.o

