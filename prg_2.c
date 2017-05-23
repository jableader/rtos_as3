#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define SUPPORTED_REFERENCES 100

typedef enum { NotReady, WaitingForSignal, Signalled } ReadyState;

volatile ReadyState state = NotReady;

void interruptHandler(int i) {
	if (state == NotReady) {
		printf("\n\nYou haven't provided the information needed for the calculations.\nThe process will now terminate\n");
		exit(1);
	}

	putchar('\n');
	state = Signalled;
}

bool contains(int arr[], int length, int value) {
	for (int i = 0; i < length; i++) {
		if (arr[i] == value) {
			return true;
		}
	}

	return false;
}

void printDetails(int ref, int faults, int frames[], int nbFrames) {
	printf("Hit: %d, Faults: %d, Frame: [%d", ref, faults, frames[0]);
	for (int i = 1; i < nbFrames; i++) {
		if (frames[i] < 0)
			printf(", X");
		else
			printf(", %d", frames[i]);
	}

	printf("]\n");
}

void calculatePageFaults(int nbFrames, int reference[], int nbRefs) {
	int frames[nbFrames];
	memset(frames, -1, sizeof(int) * nbFrames);
	int fIndex = 0;

	int pageFaults = 0;
	for (int i = 0; i < nbRefs; i++) {
		if (!contains(frames, nbFrames, reference[i])) {
			frames[fIndex] = reference[i];
			fIndex = (fIndex + 1) % nbFrames;
			pageFaults++;
		}

		printDetails(reference[i], pageFaults, frames, nbFrames);
	}
}

int readReferenceString(int reference[]) {
	printf("Please enter the reference string (comma seperated numbers without spaces):\n");

	// Single digit # and a comma, we should get up to 100 from this buffer size
	char buffer[SUPPORTED_REFERENCES * 2 + 1];
	memset(buffer, 0, sizeof(buffer));

	if (fgets(buffer, SUPPORTED_REFERENCES * 2, stdin) == NULL) {
		printf("Couldn't read your reference string.");
	}

	int rIndex = 0;
	for (int i = 0; buffer[i] != '\0' ; i += 2) {
		if (buffer[i] < '0' || buffer[i] > '9') {
			printf("Invalid char at %d. Expected number got '%c' (%d)\n", i, buffer[i], (int)buffer[i]);
			return -1;
		}

		reference[rIndex++] = buffer[i] - '0';

		char next = buffer[i + 1];
		if (next != ',' && next != '\n' && next != '\0') {
			printf("Invalid char at %d. Expected comma got '%c'\n", i + 1, buffer[i + 1]);
		}
	}

	return rIndex;
}

void flushStdin(void) {
	char ch;
	while ((ch = getchar()) != '\n' && ch != EOF);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, interruptHandler);
	if (argc != 2) {
    printf("Usage: %s number_of_frames\nFor example: %s 4\n", argv[0], argv[0]);
    return 0;
  }

  int nbFrames = atoi(argv[1]);
	if (nbFrames < 1 || nbFrames > 9) {
		perror("Not a valid frame count (1-9). Please run the program again.\n");
		return 1;
	}

	int reference[SUPPORTED_REFERENCES];
	int nbRefs = readReferenceString(reference);
	if (nbRefs <= 0) {
		printf("Bad reference string. Please run the program again.\n");
		return 2;
	}

	state = WaitingForSignal;
	printf("Press Ctrl + C when you're ready to see the results!\n");
	while (state != Signalled) {
		sleep(0);
	}

	calculatePageFaults(nbFrames, reference, nbRefs);
	return 0;
}
