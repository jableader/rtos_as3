/***********************************
 * RTOS Assignment 3, Prg 1
 * Written by Jacob Dunk, 11654718
 *
 * Compile with "gcc -pthread -o main main.c"
 ***********************************/

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

const int quantum = 4;

/* The details for a record in the input file
 */
typedef struct {
	int pid, arriveTime, burstTime;
} CpuBlock;

typedef struct {
	int pid, remainingTime;
} RunningProcess;

typedef struct {
	int pid, t_start;
} GanttCell;

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

void appendBlock(RunningProcess units[], CpuBlock block) {
	for (int i = 0; ; i++) {
		if (units[i].pid == 0) {
			units[i].pid = block.pid;
			units[i].remainingTime = block.burstTime;
			return;
		}
	}
}

int nextReadyUnit(RunningProcess procs[], int nbUnits, int lastUnit) {
	for (int i = (lastUnit + 1) % nbUnits; i != lastUnit; i = (i + 1) % nbUnits) {
		if (procs[i].remainingTime > 0) {
			return i;
		}
	}

	return -1;
}

void appendUnit(GanttCell chart[], int* index, int lim, int pid, int time) {
	if (*index > lim) {
		printf("Chart buffer limit has been exceeded!\n");
	}

	chart[*index].pid = pid;
	chart[*index].t_start = time;
	*index = *index + 1;
}

int createGanttChart(CpuBlock blocks[], int nbBlocks, GanttCell chart[], int chartLim) {
	RunningProcess procs[nbBlocks];
	memset(procs, 0, nbBlocks * sizeof(RunningProcess));
	memset(chart, 0, chartLim * sizeof(int));

	int nextArrivingBlock = 0, chartIndex = 0, t = blocks[0].arriveTime;
	for (int unitToProcess = nbBlocks - 1; unitToProcess != -1; ) {
		if (nextArrivingBlock < nbBlocks && blocks[nextArrivingBlock].arriveTime <= t) {
			appendBlock(procs, blocks[nextArrivingBlock]);
			nextArrivingBlock++;
		}

		unitToProcess = nextReadyUnit(procs, nbBlocks, unitToProcess);
		if (unitToProcess != -1) {
			RunningProcess *proc = &procs[unitToProcess];
			appendUnit(chart, &chartIndex, chartLim, proc->pid, t);

			int duration = MIN(proc->remainingTime, quantum);
			t += duration;
			proc->remainingTime -= duration;
		}
	}

	appendUnit(chart, &chartIndex, chartLim, -1, t);
	return chartIndex;
}

int findCompletionTime(GanttCell chart[], int nbCells, int pid) {
	for (int i = nbCells - 1; i > 0; i--)
		if (chart[i - 1].pid == pid)
			return chart[i].t_start;
	return -1;
}

void printGanttChart(GanttCell cells[], int n) {
	printf("pid\t| t\n");
	for (int i = 0; i < n; i++) {
		printf("%d\t| %d\n", cells[i].pid, cells[i].t_start);
	}
}

/* The main method.
 */
int main(void) {
	CpuBlock blocks[10];
	int nbBlocks = loadCpuBlocks(blocks, 10);
	if (nbBlocks <= 0) {
		perror("There was a problem reading the input file\n");
		return 1;
	}

	GanttCell ganttArr[100];
	int nbCells = createGanttChart(blocks, nbBlocks, ganttArr, 100);
	printGanttChart(ganttArr, nbCells);
}
