all: prodcon

clean:
	@rm -rf prodcon 
	@rm -rf *.log
	@rm -rf tands.o


prodcon: tands.o prodcon.cpp 
	g++ -std=c++11 prodcon.cpp tands.o -o prodcon -lstdc++ -lpthread
tands.o: tands.h tands.c
		gcc -c tands.c
