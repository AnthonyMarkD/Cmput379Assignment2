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
#include "tands.h"
#include <chrono>
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
mutex workMutex, writeMutex;
int numThreads, threadId, workCount, sleepCount, receiveCount, askCount, completeCount;
FILE* output;
void threadJob(int threadId);
void getInput();
void pushToQueue(int workAmount);
int popFromQueue();
void printToFile(int id, int work, string operation, bool queue_size);
void closeFile();
void initFile(int threadId);
double getCurrRealTime();
chrono::nanoseconds start;
void printSummary();
int main(int argc, char *argv[]) {

	workCount = 0;
	sleepCount = 0;
	receiveCount = 0;
	askCount = 0;
	completeCount = 0;
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
	initFile(threadId);

	//Start timer
	// start = std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::steady_clock::now()).count();

	thread consumers[numThreads];

	for ( int i = 0;  i < numThreads; i++) {
		consumers[i] = thread(threadJob, i + 1);
	}
	getInput();


	for ( int i = 0;  i < numThreads; i++) {

		consumers[i].join();
		printf("Joining thread: %d\n", i);
	}
	printSummary();
	closeFile();
	return 0;
}



void threadJob(int threadId) {
	while (1) {
		askCount++;
		printToFile(threadId, NULL , "Ask", false);
		int workAmount = popFromQueue();


		if (workAmount == -1) {
			break;
		}
		receiveCount++;
		printToFile(threadId, workAmount , "Receive", true);
		Trans(workAmount);
		completeCount++;
		printToFile(threadId, workAmount , "Complete", false);
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
			workCount++;
			printToFile(0, workAmount , "Work", true);
			//add to buffer queue
		} else if (strcmp(operation, "S") == 0) {
			// Producer sleeps Sleep
			// add to buffer queue
			sleepCount++;
			length = atoi(inputLine.substr(1).c_str());
			printf("%s\n", "Main process is going to sleep" );
			printToFile(0, length , "Sleep", false);
			Sleep(length);

		}



	}
	printToFile(0, NULL , "End", false);
	pushToQueue(-1);
}
void pushToQueue(int workAmount) {
	unique_lock<mutex> lk(workMutex);
	cv.wait(lk, []() {return !(workQueue.size() >= numThreads * 2);});
	printf("Work Queue size:%lu\n", workQueue.size() );
	workQueue.push(workAmount);

	lk.unlock();
	cv.notify_all();
}

int popFromQueue() {
	unique_lock<mutex> lk(workMutex);
	cv.wait(lk, []() {return (workQueue.size() != 0);});

	int workAmount = workQueue.front();
	if (workAmount != -1) {
		workQueue.pop();
	}


	lk.unlock();
	cv.notify_all();
	return workAmount;
}
void initFile(int id) {
	string outputFileName;
	if (threadId == 0) {
		outputFileName = "prodcon.log";
	} else {
		outputFileName = "prodcon." + to_string(id) ;
		outputFileName += ".log";

	}
	output = fopen(outputFileName.c_str(), "w");

}
void closeFile() {
	fclose(output);
}
void printToFile(int id, int work, string operation, bool queue_size) {
	char buffer [100];
	unique_lock<mutex> lk(writeMutex);
	if (work == NULL) {
		sprintf (buffer, "%.3f ID= %d      %s\n", getCurrRealTime(), id, operation.c_str());
		fputs(buffer, output);
	} else if (queue_size) {
		sprintf (buffer, "%.3f ID= %d Q= %ld %s %4d\n", getCurrRealTime(), id, workQueue.size(), operation.c_str(), work);
		fputs(buffer, output);
	} else {
		sprintf (buffer, "%.3f ID= %d      %s %4d\n", getCurrRealTime(), id, operation.c_str(), work);
		fputs(buffer, output);
	}

	lk.unlock();
}
double getCurrRealTime() {
	// auto now = chrono::steady_clock::now();
	// chrono::nanoseconds end = std::chrono::duration<double, nano>(now).count();
	// chrono::nanoseconds curr = end - start;
	// return chrono::duration <double> (curr).count();
	return 0.00;
}
void printSummary() {
	char buffer [100];
	sprintf (buffer, "Summary:\n", workCount);
	fputs(buffer, output);
	sprintf (buffer, "	Work %d\n", workCount);
	fputs(buffer, output);
	sprintf (buffer, "	Ask %d\n", askCount );
	fputs(buffer, output);
	sprintf (buffer, "	Receive %d\n", receiveCount);
	fputs(buffer, output);
	sprintf (buffer, "	Complete %d\n", completeCount);
	fputs(buffer, output);
	sprintf (buffer, "	Sleep %d\n", sleepCount);
	fputs(buffer, output);


}
