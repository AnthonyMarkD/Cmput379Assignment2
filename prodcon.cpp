#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cstring>
#include <regex>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;
const int LINE_LENGTH = 100; // Max # of charactersinan input line
const int MAX_ARGS = 7; // Max number of arguments to a command
const int MAX_LENGTH = 20; //Max # of characters in an argument
const int MAX_PT_ENTRIES = 32; //Max entries in the Process Table

queue<int> workQueue;

struct rusage ru;
struct timeval user_time, system_time;

char input[100];
char* args[MAX_ARGS + 1];


// #define MAX_INPUT_LINE_LENGTH 10	// Max length of work line in input (e.g. T5 or S9)
// #define MAX_OUTPUT_LINE_LENGTH 50	// Max length of output line, chosen as 50 for safe number
condition_variable cv;
mutex lockMutex;
int numThreads, threadId;
void threadJob(int threadId);
void getInput();
void pushToQueue(int workAmount);
int popFromQueue();

int main(int argc, char *argv[]) {

	if (argc == 1) {
		// no arguement, error
		perror("No argument passed");
	} else if (argc == 2) {
		// only one paramter passed
		numThreads = atoi(argv[1]);
		threadId = 0;
	} else {
		//
		numThreads = atoi(argv[1]);
		threadId = atoi(argv[2]);
	}


	thread consumers[numThreads];

	for ( int i = 0;  i < numThreads; i++) {
		consumers[i] = thread(threadJob, i + 1);
	}
	getInput();


	for ( int i = 0;  i < numThreads; i++) {
		printf("Joining thread: %d\n", i);
		consumers[i].join();
	}
	return 0;
}



void threadJob(int threadId) {
	while (1) {
		printf("Thread %d asking for work\n",threadId);
		int workAmount = popFromQueue();
		printf("Got work:%d\n", workAmount);
	}

}

void getInput() {
	string inputLine;
	int length;
	while (fgets(input, 10, stdin) != NULL) {
		// get input from file or cmd line

		string inputLine = input;
		printf("Reading input:%s\n", input);
		const char* operation = inputLine.substr(0, 1).c_str();
		if (strcmp(operation, "T") == 0) {
			int workAmount = atoi(inputLine.substr(1).c_str());
			//Transaction
			pushToQueue(workAmount);
			//add to buffer queue
		} else if (strcmp(operation, "S") == 0) {
			// Producer sleeps Sleep
			// add to buffer queue
			length = atoi(inputLine.substr(1).c_str());

		}



	}
	printf("%s\n", "Done reading Input");
}
void pushToQueue(int workAmount) {
	unique_lock<mutex> lk(lockMutex);
	cv.wait(lk, []() {return (workQueue.size() != numThreads * 2);});

	workQueue.push(workAmount);

	lk.unlock();
	cv.notify_all();
}

int popFromQueue() {
	unique_lock<mutex> lk(lockMutex);
	cv.wait(lk, []() {return (workQueue.size() != 0);});

	int workAmount = workQueue.front();
	workQueue.pop();

	lk.unlock();
	cv.notify_all();
	return workAmount;
}


