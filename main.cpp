#include <stdio.h>
#include <stdlib.h>
#include "uthread.h"

int utid1, utid2;

void agent1() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent1, i = %d\n", i);
		for (int j = 0; j < 100000000; ++j)
			;
	}
}

void agent2() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent2, i = %d\n", i);
		for (int j = 0; j < 100000000; ++j)
			;
		if (i == 3)
			uthread_join(utid1);
	}
}

int main() {
	utid1 = uthread_create(agent1);
	utid2 = uthread_create(agent2);
	printf("preparing to run concurrent threads ...\n");
	uthread_runall();
	for (int i = 0; i < 20; ++i) {
		printf("hello from main thread, i = %d\n", i);
		for (int j = 0; j < 100000000; ++j)
				;
	}
	printf("back to main and all clear!\n");
	return 0;
}
