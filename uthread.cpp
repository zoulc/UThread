#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE 4192
#define UTHREAD_MAX_NUM 256

enum uthread_stat {
	FREE = 0,
	RUNNABLE,
};

struct uthread_struct {
	ucontext_t context;
	char stack[STACK_SIZE];
	void (*func)(void);
	uthread_stat status;
};

static struct uthread_struct sched_thread;	// scheduler
static struct uthread_struct uthreads[UTHREAD_MAX_NUM];

static const int main_utid = 0;		// utid for main routine
static int lastrun_utid = -1;

typedef void (*pfunc_t)(void);

static void uthread_wrapper(void (*func)(void), uthread_stat *status) {
	func();
	printf("reaped a thread\n");
	*status = FREE;
}

static void uthread_make(struct uthread_struct *uthread) {
	getcontext(&uthread->context);
    uthread->context.uc_stack.ss_sp = uthread->stack;
    uthread->context.uc_stack.ss_size = STACK_SIZE;
    uthread->context.uc_link = &sched_thread.context;
    uthread->status = RUNNABLE;
}

int uthread_create(void (*func)(void)) {
	int utid;
	for (utid = 0; utid < UTHREAD_MAX_NUM; ++utid)
		if (uthreads[utid].status == FREE)
			break;
	if (utid == UTHREAD_MAX_NUM) {
		printf(" [error] run out of available uthread slot \n");
		return -1;
	}
	uthread_make(&uthreads[utid]);
    makecontext(&uthreads[utid].context, (pfunc_t)uthread_wrapper, 2, func, &uthreads[utid].status);
	return utid;
}

int uthread_self() {
	return lastrun_utid;
}

void uthread_yield() {
	swapcontext(&uthreads[uthread_self()].context, &sched_thread.context);
}

void agent1() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent1, i = %d\n", i);
		uthread_yield();
	}
}

void agent2() {
	for (int i = 0; i < 10; ++i) {
		printf("hello, world! --from agent2, i = %d\n", i);
		uthread_yield();
	}
}

void scheduler() {
	printf("running scheduler\n");
	for (int utid = lastrun_utid + 1; utid != lastrun_utid;
		utid = (utid + 1) % UTHREAD_MAX_NUM)
		if (uthreads[utid].status == RUNNABLE) {
			lastrun_utid = utid;
			setcontext(&uthreads[utid].context);
		}
	printf("no runnable threads, return from scheduler\n");
}

void uthread_init() {
	uthread_make(&sched_thread);
    makecontext(&sched_thread.context, scheduler, 0);
}

int main() {
	uthread_init();
	uthread_create(agent1);
	uthread_create(agent2);
	printf("preparing to setcontext ...\n");
	scheduler();
	for (int i = 0; i < 2; ++i)
		printf("uthreads[%d].status = %d\n", i, (int)(uthreads[i].status));
	printf("sched_thread.status = %d\n", (int)(sched_thread.status));
	printf("back to main and all clear!\n");
	return 0;
}
