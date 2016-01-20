#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>

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

static int main_utid;			// utid for main routine
static int lastrun_utid = -1;
static int initialized = 0;

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
    uthread->context.uc_link = NULL;
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
    uthreads[utid].context.uc_link = &sched_thread.context;
    makecontext(&uthreads[utid].context, (pfunc_t)uthread_wrapper, 2, func, &uthreads[utid].status);
	return utid;
}

int uthread_self() {
	return lastrun_utid;
}

void uthread_yield() {
	swapcontext(&uthreads[uthread_self()].context, &sched_thread.context);
}

void uthread_exit() {
	printf("reaped a thread\n");
	uthreads[uthread_self()].status = FREE;
}

void uthread_cancel(int utid) {
	printf("reaped a thread\n");
	uthreads[utid].status = FREE;
}

void uthread_join(int utid) {
	while (uthreads[utid].status != FREE)
		uthread_yield();
}

static void scheduler() {
	printf("running scheduler\n");
	for (int i = 1; i <= UTHREAD_MAX_NUM; ++i)
		if (uthreads[(lastrun_utid + i) % UTHREAD_MAX_NUM].status == RUNNABLE) {
			lastrun_utid = (lastrun_utid + i) % UTHREAD_MAX_NUM;
			setcontext(&uthreads[lastrun_utid].context);
		}
	for (int utid = 0; utid < 3; ++utid)
		printf("uthreads[%d].status = %d\n", utid, uthreads[utid].status);
	printf("no runnable threads, return from scheduler\n");
}

static void sigalrm_handler(int sig) {
	printf("received signal SIGVTALRM\n");
	uthread_yield();
}

static struct itimerval timer;
static struct sigaction action, old_action;

static void uthread_init() {
	uthread_make(&sched_thread);
    makecontext(&sched_thread.context, scheduler, 0);
	main_utid = uthread_create(NULL);
	sched_thread.context.uc_link = &uthreads[main_utid].context;

	action.sa_handler = sigalrm_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &action, &old_action);

	initialized = 1;
}

void uthread_runall(int interval) {
	if (initialized == 0)
		uthread_init();
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = interval;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = interval;
	setitimer(ITIMER_REAL, &timer, NULL);
	swapcontext(&uthreads[main_utid].context, &sched_thread.context);
}
