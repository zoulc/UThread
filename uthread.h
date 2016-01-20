#ifndef UTHREAD_H_INCLUDED
#define UTHREAD_H_INCLUDED

int uthread_create(void (*func)(void));
int uthread_self(void);
void uthread_yield(void);
void uthread_runall(int interval = 500000);
void uthread_exit(void);
void uthread_cancel(int utid);
void uthread_join(int utid);

#endif
