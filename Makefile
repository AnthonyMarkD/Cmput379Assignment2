all: prodcon

clean:
	@rm -rf prodcon 
	@rm -rf *.log

tands.o: tands.h tands.c
		gcc -c tands.c tands.h

prodcon: prodcon.cpp tands.o
	g++ -std=c++11 prodcon.cpp tands.o -o prodcon -lstdc++ -lpthread
