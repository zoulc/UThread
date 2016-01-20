#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

void handler(int) {
	printf("hello from handler\n");
}

int main() {
	signal(SIGVTALRM, handler);
	struct itimerval tick;
	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = 1;
	tick.it_interval.tv_sec = 1;
	tick.it_interval.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &tick, NULL);
	while (true) ;
	return 0;
}
