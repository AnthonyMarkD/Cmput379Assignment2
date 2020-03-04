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
chrono::time_point<std::chrono::system_clock> start;


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
		// set values to variables
		numThreads = atoi(argv[1]);
		threadId = atoi(argv[2]);
	}
	initFile(threadId);
	start = std::chrono::system_clock::now();

	// Start consumer threads
	thread consumers[numThreads];

	for ( int i = 0;  i < numThreads; i++) {
		consumers[i] = thread(threadJob, i + 1);
	}
	getInput();
	// end consumer threads by joining them.
	for ( int i = 0;  i < numThreads; i++) {

		consumers[i].join();
		
	}
	//print summary
	printSummary();
	// close output file like good smart people
	closeFile();
	return 0;
}

// What our threads do, simply grab stuff from the queue and print to a file.

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
			
			printToFile(0, length , "Sleep", false);
			Sleep(length);

		}



	}
	printToFile(0, NULL , "End", false);
	// push to queue variable which causes threads to finish and allows them to join
	pushToQueue(-1);
}
// pushes work to queue, queue is protected buy a unique lock mutex which waits for the queue to not be full
void pushToQueue(int workAmount) {
	unique_lock<mutex> lk(workMutex);
	cv.wait(lk, []() {return !(workQueue.size() >= numThreads * 2);});
	
	workQueue.push(workAmount);

	lk.unlock();
	cv.notify_all();
}
// pushes work to queue, queue is protected buy a unique lock mutex which waits for the queue to not be empty
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
// initializes our output file
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
//closes the output file
void closeFile() {
	fclose(output);
}
// print to output file, use a mutex to prevent garbled garbage from occuring.
void printToFile(int id, int work, string operation, bool queue_size) {
	char buffer [100];
	unique_lock<mutex> lk(writeMutex);
	if (work == NULL) {
		sprintf (buffer, "%.3f ID= %d      %8s\n", getCurrRealTime(), id, operation.c_str());
		fputs(buffer, output);
	} else if (queue_size) {
		sprintf (buffer, "%.3f ID= %d Q= %ld %8s %5d\n", getCurrRealTime(), id, workQueue.size(), operation.c_str(), work);
		fputs(buffer, output);
	} else {
		sprintf (buffer, "%.3f ID= %d      %8s %5d\n", getCurrRealTime(), id, operation.c_str(), work);
		fputs(buffer, output);
	}

	lk.unlock();
}
//Gets the current time compared to start of the program.
double getCurrRealTime() {

	chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();

	double difference = chrono::duration<double>(now - start).count();
	return difference;

}
void printSummary() {
	char buffer [100];
	sprintf (buffer, "Summary:\n");
	fputs(buffer, output);
	sprintf (buffer, "	Work     %d\n", workCount);
	fputs(buffer, output);
	sprintf (buffer, "	Ask      %d\n", askCount );
	fputs(buffer, output);
	sprintf (buffer, "	Receive  %d\n", receiveCount);
	fputs(buffer, output);
	sprintf (buffer, "	Complete %d\n", completeCount);
	fputs(buffer, output);
	sprintf (buffer, "	Sleep    %d\n", sleepCount);
	fputs(buffer, output);
	sprintf (buffer, "Transactions per second: %.2f\n", double(workCount) / getCurrRealTime());
	fputs(buffer, output);


}
