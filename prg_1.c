/***********************************
 * RTOS Assignment 3, Prg 1
 * Written by Jacob Dunk, 11654718
 *
 * Compile with "gcc -pthread -o main main.c"
 ***********************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define FIFO_NAME "HeyLookItsTheFifo"

// Some variables to be passed to the threads. Normally I would use arguments
// but this is too trivial a case to warrent it.
int quantum = 4;
char* outputFilePath;

/* The details for a record in the input file
 */
typedef struct {
	int pid, arriveTime, burstTime, completionTime;
} CpuBlock;

typedef struct {
	int pid, remainingTime;
	CpuBlock * block;
} RunningProcess;

/* A union to help with remembering the correct read & write indexes
 */
typedef union {
	int fd[2];
	struct {
		int read;
		int write;
	} s;
} PipeDescriptor;

/* Used to sort the CPU Blocks by their arrival times
 */
int sortByArrival(const void * a, const void * b) {
	return ((CpuBlock*)a)->arriveTime - ((CpuBlock*)b)->arriveTime;
}

/* Loads the records from the input file into the provided array until the file
 * has no more records, or 'limit' has been reached. Returns the number of
 * records loaded, or -1 on error
 */
int loadCpuBlocks(CpuBlock blocks[], int limit) {
	FILE* fp = fopen("prg1_input.txt", "r");
	if (fp == NULL)
		return -1;

	int i;
	for (i = 0; i < limit; i++) {
		if (fscanf(fp, "%d %d %d\n", &blocks[i].pid, &blocks[i].arriveTime, &blocks[i].burstTime) != 3) {
			break;
		}
	}

	fclose(fp);
	qsort(blocks, i, sizeof(CpuBlock), sortByArrival);

	return i;
}

/* Append this block to the running stack. Find the first free slot and populate
 * it
 */
void appendBlock(RunningProcess units[], CpuBlock* block) {
	for (int i = 0; ; i++) {
		if (units[i].block == NULL) {
			units[i].remainingTime = block->burstTime;
			units[i].block = block;
			return;
		}
	}
}

/* Find and return the index of the next process that is waiting to be run.
 * start the search at lastUnit
 */
int nextReadyUnit(RunningProcess procs[], int nbUnits, int lastUnit) {
	for (int i = (lastUnit + 1) % nbUnits; i != lastUnit; i = (i + 1) % nbUnits) {
		if (procs[i].remainingTime > 0) {
			return i;
		}
	}

	return -1;
}

/* Calculate and populate the completionTime field for everything in `blocks`
 * using round robin
  */
void calculateCompletionTimes(CpuBlock blocks[], int nbBlocks) {
	RunningProcess procs[nbBlocks];
	memset(procs, 0, nbBlocks * sizeof(RunningProcess));

	int nextArrivingBlock = 0, t = blocks[0].arriveTime;
	for (int unitToProcess = nbBlocks - 1; unitToProcess != -1; ) {
		if (nextArrivingBlock < nbBlocks && blocks[nextArrivingBlock].arriveTime <= t) {
			appendBlock(procs, &blocks[nextArrivingBlock]);
			nextArrivingBlock++;
		}

		unitToProcess = nextReadyUnit(procs, nbBlocks, unitToProcess);
		if (unitToProcess != -1) {
			RunningProcess *proc = &procs[unitToProcess];

			int duration = MIN(proc->remainingTime, quantum);
			t += duration;
			proc->remainingTime -= duration;

			if (proc->remainingTime <= 0) {
				proc->block->completionTime = t;
			}
		}
	}
}

double getTurnaround(CpuBlock * block) {
	return block->completionTime - block->arriveTime;
}

double getWaitTime(CpuBlock * block) {
	return getTurnaround(block) - block->burstTime;
}

double findAverage(CpuBlock blocks[], int nbBlocks, double(*getN)(CpuBlock*)) {
	double sum = 0.0D;
	for (int i = 0; i < nbBlocks; i++)
		sum += getN(&blocks[i]);
	return sum / nbBlocks;
}

void threadTwo(void) {
  int fifo = open(FIFO_NAME, O_RDONLY);
  if (fifo == -1) {
    printf("Error opening the FIFO for reading.\n");
    exit(1);
  }

  double results[2];
	if (read(fifo, results, sizeof(results)) <= 0) {
		perror("Error reading from the pipe\n");
		return;
	}
	close(fifo);

	FILE* f = fopen(outputFilePath, "w");
	if (f == NULL) {
		perror("Error opening output file\n");
		return;
	}

	fprintf(f, "Average Turnaround: %.2f\nAverage Wait: %.2f\n", results[0], results[1]);
	fclose(f);
}

void threadOne(void) {
	CpuBlock blocks[10];
	int nbBlocks = loadCpuBlocks(blocks, 10);
	if (nbBlocks <= 0) {
		perror("There was a problem reading the input file\n");
		return;
	}

	calculateCompletionTimes(blocks, nbBlocks);

	if (mkfifo(FIFO_NAME, 0666) != 0) {
    // Probably a FIFO with that name exists. We can ignore this so long as the
    // fifo can be opened futher down
		printf("Warning: There was an error when creating the FIFO.\n");
	}

	pthread_t t2;
	if (pthread_create(&t2, NULL, (void *)threadTwo, NULL) != 0) {
		perror("Error creating thread two\n");
		return;
	}

	double avg[2] = {
		findAverage(blocks, nbBlocks, getWaitTime),
		findAverage(blocks, nbBlocks, getTurnaround)
	};

  int fifo = open(FIFO_NAME, O_WRONLY);
  if (fifo == -1) {
    printf("Error opening the FIFO for writing.\n");
    exit(1);
  }

	write(fifo, avg, sizeof(avg));
	close(fifo);

	pthread_join(t2, NULL);
}

/* The main method.
 */
int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s quantum outputPath\nFor example: %s 4 output.txt\n", argv[0], argv[0]);
    return 0;
  }

  quantum = atoi(argv[1]);
  if (quantum <= 0) {
    printf("Quantum must be greater than zero.");
    return 1;
  }

  outputFilePath = argv[2];
  if (strlen(outputFilePath) <= 0) {
    printf("File path cannot be empty\n");
    return 1;
  }

	pthread_t t1;
	if (pthread_create(&t1, NULL, (void *)threadOne, NULL) != 0) {
		perror("Problem creating thread one\n");
		return -1;
	}

	pthread_join(t1, NULL);
}
