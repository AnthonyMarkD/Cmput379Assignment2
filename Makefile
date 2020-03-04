all: prodcon

clean:
	@rm -rf prodcon 

prodcon: prodcon.cpp 
	g++ -std=c++11 prodcon.cpp -o prodcon -lpthread
