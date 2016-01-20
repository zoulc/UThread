#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE 4192
#define UTHREAD_MAX_NUM 256

enum uthread_stat {
	FREE = 0,
	RUNNABLE,
};

static struct uthread_struct {
	ucontext_t context;
	char stack[STACK_SIZE];
	void (*func)(void);
	uthread_stat status;
} uthreads[UTHREAD_MAX_NUM];

static const int sched_utid = 0;
static int lastrun_utid = 0;

static void uthread_create_utid(int utid, void (*func)(void)) {
	getcontext(&uthreads[utid].context);
    uthreads[utid].context.uc_stack.ss_sp = uthreads[utid].stack;
    uthreads[utid].context.uc_stack.ss_size = STACK_SIZE;
    uthreads[utid].context.uc_link = &uthreads[sched_utid].context;
    uthreads[utid].status = RUNNABLE;
    makecontext(&uthreads[utid].context, func, 0);
}

int uthread_create(void (*func)(void)) {
	int utid;
	for (utid = 1; utid < UTHREAD_MAX_NUM; ++utid)
		if (uthreads[utid].status == FREE)
			break;
	if (utid == UTHREAD_MAX_NUM) {
		printf(" [error] run out of available uthread slot \n");
		return -1;
	}
	uthread_create_utid(utid, func);
}

int uthread_self() {
	printf("running uthread_self() = %d\n", lastrun_utid);
	return lastrun_utid;
}

void uthread_yield() {
	printf("running uthread_yield()\n");
	swapcontext(&uthreads[uthread_self()].context, &uthreads[sched_utid].context);
}

void agent1() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent1, i = %d\n", i);
		uthread_yield();
	}
	uthreads[uthread_self()].status = FREE;
}

void agent2() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent2, i = %d\n", i);
		uthread_yield();
	}
	uthreads[uthread_self()].status = FREE;
}

void scheduler() {
	printf("running scheduler\n");
	for (int utid = lastrun_utid + 1; utid != lastrun_utid;
		utid = (utid + 1) % UTHREAD_MAX_NUM)
		if (utid != sched_utid && uthreads[utid].status == RUNNABLE) {
			printf("scheduling uthread[%d] to run\n", utid);
			lastrun_utid = utid;
			setcontext(&uthreads[utid].context);
		}
}

void uthread_init() {
	uthread_create_utid(sched_utid, scheduler);
}

int main() {
	uthread_init();
	uthread_create(agent1);
	uthread_create(agent2);
	printf("preparing to setcontext ...\n");
	scheduler();
	printf("back to main and all clear!\n");
	return 0;
}
