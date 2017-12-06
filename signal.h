#ifndef THREADS_SIGNAL_H
#define THREADS_SIGNAL_H


#include <list.h>
#include <stdio.h>

#define SIG_CHLD	1
#define SIG_KILL	2
#define SIG_CPU		3
#define SIG_UNBLOCK	4
#define SIG_USER	5

#define SIG_IGN		6
#define SIG_DFL		7

#define SIG_BLOCK 	8
#define SIG_UNBLOCK_ 9	
#define SIG_SETMASK	 10

typedef struct _mysignal{
	int sentid;
	int receiveid;
	int sigtype;
	struct list_elem elem;
}mysignal;

typedef struct sigset
{
	int sigchild;
	int sigkill;
	int sigcpu;
	int sigunblock;
	int siguser;
}sigset;

void sigchild_handler(tid_t );
void sigkill_handler(tid_t );
void sigcpu_handler(tid_t );
void sigunblock_handler(tid_t );
void siguser_handler(tid_t );

/* User Code will call this function*/
int _signal(int ,int);
int kill(tid_t,int);

int sigemptyset(sigset *);
int sigfillset(sigset *);
int sigaddset(sigset *, int );
int sigdelset(sigset *, int );
int sigprocmask(int , const sigset * ,sigset *);

#endif