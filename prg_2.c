#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include  <signal.h>

#define SUPPORTED_REFERENCES 100

volatile bool interruptRecieved = false;

void interruptHandler(int i) {
	interruptRecieved = true;
}

bool contains(int arr[], int length, int value) {
	for (int i = 0; i < length; i++) {
		if (arr[i] == value) {
			return true;
		}
	}

	return false;
}

void printDetails(int faults, int frames[], int nbFrames) {
	printf("Faults: %d, Frame: [%d", faults, frames[0]);
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
	for (int i = 0; i < nbFrames; i++) {
		if (!contains(frames, nbFrames, reference[i])) {
			frames[fIndex] = reference[i];
			fIndex = (fIndex + 1) % nbFrames;
			pageFaults++;
		}

		printDetails(pageFaults, frames, nbFrames);
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
	for (int i = 0; buffer[i] != '\0'; i += 2) {
		if (buffer[i] < '0' || buffer[i] > '9') {
			printf("Invalid char at %d. Expected number got %c (%d)", i, buffer[i], (int)buffer[i]);
			return -1;
		}

		reference[rIndex] = buffer[i] - '0';

		if (buffer[i + 1] != ',' && buffer[i + 1] != '\0') {
			printf("Invalid char at %d. Expected comma got %c", i + 1, buffer[i + 1]);
		}
	}

	return rIndex;
}

int main(void) {
	printf("Please enter the number of frames (< 10): ");

	int nbFrames;
	if (scanf("%d", &nbFrames) != 1 || nbFrames > 9 || nbFrames < 1) {
		perror("Not a valid frame count (1-9). Please run the program again.\n");
		return 1;
	}

	int reference[SUPPORTED_REFERENCES];
	int nbRefs = readReferenceString(reference);
	if (nbRefs <= 0) {
		perror("Bad reference string. Please run the program again.\n");
		return 2;
	}

	printf("Press Ctrl + C when you're ready to see the results!\n");
	signal(SIGINT, interruptHandler);

	while (!interruptRecieved) {
		sleep(0);
	}

	calculatePageFaults(nbFrames, reference, nbRefs);
	return 0;
}
