#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE 4192
#define UTHREAD_MAX_NUM 256

enum thread_stat {
	FREE = 0,
	RUNNABLE,
};

struct uthread_struct {
	ucontext_t context;
	char stack[STACK_SIZE];
	void (*func)(void);
	thread_stat status;
} uthreads[UTHREAD_MAX_NUM];

void agent1() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent1, i = %d\n", i);
		swapcontext(&uthreads[1].context, &uthreads[0].context);
	}
	uthreads[1].status = FREE;
}

void agent2() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent2, i = %d\n", i);
		swapcontext(&uthreads[2].context, &uthreads[0].context);
	}
	uthreads[2].status = FREE;
}

void scheduler() {
	for (int i = 1; i < UTHREAD_MAX_NUM; ++i)
		if (uthreads[i].status == RUNNABLE)
			setcontext(&uthreads[i].context);
}

int main() {
	for (int i = 0; i < UTHREAD_MAX_NUM; ++i) {
		getcontext(&uthreads[i].context);
		uthreads[i].context.uc_stack.ss_sp = uthreads[i].stack;
		uthreads[i].context.uc_stack.ss_size = STACK_SIZE;
		uthreads[i].context.uc_link = &uthreads[0].context;
	}
	for (int i = 1; i < 3; ++i)
		uthreads[i].status = RUNNABLE;
	makecontext(&uthreads[0].context, scheduler, 0);
	makecontext(&uthreads[1].context, agent1, 0);
	makecontext(&uthreads[2].context, agent2, 0);
	printf("preparing to setcontext ...\n");
	scheduler();
	printf("back to main and all clear!\n");
	return 0;
}
