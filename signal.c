#include "threads/thread.h"
#include "threads/signal.h"
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

void sigchild_handler(tid_t id){
	struct thread* t = get_thread(id);
	ASSERT(t!=NULL);
	ASSERT(is_thread(t));

	int temp = t->mask & (1 << 0);
	if (temp==0)
	{
		printf("Sigchild ignored\n");
		return;
	}

	(t->died_so_far) = (t->died_so_far) + 1;

	//printf("Thread Name : %s",t->name);
	printf("Total Number of Children created so far : %d\n",t->created_so_far);
	printf("Total Number of Children still alive : %d\n",(t->created_so_far)-(t->died_so_far));
}

void sigkill_handler(tid_t id){
	struct thread* t = get_thread(id);
	ASSERT(t!=NULL);
	ASSERT(is_thread(t));

	printf("Thread terminated by SIG_KILL");

	thread_exit();
}

void sigcpu_handler(tid_t id){
	struct thread* t = get_thread(id);
	ASSERT(t!=NULL);
	ASSERT(is_thread(t));
	
	int temp = t->mask & (1 << 1);
	if (temp==0)
	{
		printf("Sigcpu ignored\n");
		return;
	}

	if(t->cpu_limit_over){

		printf("Thread is terminating : lifetime %lld : mylife %lld\n",t->lifetime,t->mylife);

		thread_exit();
	}
}


void sigunblock_handler(tid_t id){
	struct thread* t = get_thread(id);
	ASSERT(t!=NULL);
	ASSERT(is_thread(t));

	int temp = t->mask & (1 << 2);
	if (temp==0)
	{
		printf("Sigunblock ignored\n");
		return;
	}

	if(t->status == THREAD_BLOCKED){
		printf("Unblocking thread\n");
		thread_unblock(t);
	}
}

void siguser_handler(tid_t id){
	struct thread* t = get_thread(id);
	ASSERT(t!=NULL);
	ASSERT(is_thread(t));

	int temp = t->mask & (1 << 3);
	if (temp==0)
	{
		printf("Siguser ignored\n");
		return;
	}

	//tid_t tig;
	struct list_elem* lelem;
	for(lelem = list_begin(&t->signal_list) ; lelem != list_end(&t->signal_list) ; lelem = list_next(lelem)){
		mysignal* new = list_entry(lelem,mysignal,elem);
		if(new->sigtype == SIG_USER){
			printf("Thread %d sent this signal to thread %d\n",new->sentid,id);
			break;
		}
	}
}

int _signal(int signum, int sigtype){
	struct thread* t = thread_current();
	if(sigtype == SIG_IGN){
		if(signum != SIG_KILL){
			if(signum == SIG_CHLD){
				t->mask = (t->mask) & (~(1<<0));
			}else if(signum == SIG_CPU){
				t->mask = (t->mask) & (~(1<<1));
			}else if(signum == SIG_UNBLOCK){
				t->mask = (t->mask) & (~(1<<2));
			}else if(signum == SIG_USER){
				t->mask = (t->mask) & (~(1<<3));
			}
		}
	}else if(sigtype == SIG_DFL){
		if(signum != SIG_KILL){
			if(signum == SIG_CHLD){
				t->mask = (t->mask) | ((1<<0));
			}else if(signum == SIG_CPU){
				t->mask = (t->mask) | ((1<<1));
			}else if(signum == SIG_UNBLOCK){
				t->mask = (t->mask) | ((1<<2));
			}else if(signum == SIG_USER){
				t->mask = (t->mask) | ((1<<3));
			}
		}
	}
	return 1;
}

int kill(tid_t id, int signum){
	struct thread* t = get_thread(id);
	// ASSERT(t!=NULL);
	
	if (t==NULL)
	{
		return 0;
	}
	ASSERT(is_thread(t));
	int val = 1;
	// if(signum != SIG_KILL){
	// 	if(signum == SIG_CHLD){
	// 			val = (t->mask) & (1<<0);
	// 		}else if(signum == SIG_CPU){
	// 			val = (t->mask) & (1<<1);
	// 		}else if(signum == SIG_UNBLOCK){
	// 			val = (t->mask) & (1<<2);
	// 		}else if(signum == SIG_USER){
	// 			val = (t->mask) & (1<<3);
	// 	}
	// }

	if(val == 1){
		int stat = 0;
		struct list_elem* lelem1;
		struct thread* threadcurr = thread_current();
		for(lelem1 = list_begin(&threadcurr->child_list) ; lelem1 != list_end(&threadcurr->child_list) ; lelem1 = list_next(lelem1)){
			struct thread* ti = list_entry(lelem1,struct thread,childelem);
			if(ti->tid == id){
				stat = 1;
				break;
			}
		}
		if(signum == SIG_CPU){
			t->cpu_limit_over = 1;
			//printf("limit set\n");
			return 0;
		}

		if(signum == SIG_UNBLOCK){
			push_into_unblock(id);
			return 0;
		}
		if(signum == SIG_KILL && stat == 0)
			return 0;
		mysignal *newsignal;
		newsignal = (mysignal*)malloc(sizeof(mysignal));
		newsignal->sentid = thread_current()->tid;
		newsignal->receiveid = t->tid;
		newsignal->sigtype = signum;
		if(signum == SIG_CHLD)
			list_push_back(&t->signal_list,&newsignal->elem);
		else{
			struct list_elem *lelem;
			for(lelem = list_begin(&t->signal_list) ; lelem != list_end(&t->signal_list);){
				mysignal* sig = list_entry(lelem,mysignal,elem);
				if(sig->sigtype == signum){
					lelem = list_remove(lelem);
					break;
				}else{
					lelem = list_next(lelem);
				}
			}
			list_push_back(&t->signal_list,&newsignal->elem);
		}
	}
	return 1;
}

int sigemptyset(sigset * set)
{
	if (set==NULL)
	{
		return 0;
	}
	set->sigchild = 0;
	set->sigkill = 0;
	set->sigcpu = 0;
	set->siguser = 0;
	set->sigunblock = 0;
	return 1;
}

int sigfillset(sigset * set)
{
	if (set==NULL)
	{
		return 0;
	}
	set->sigchild = 1;
	set->sigkill = 1;
	set->sigcpu = 1;
	set->siguser = 1;
	set->sigunblock = 1;
	return 1;
}

int sigaddset(sigset * set, int signum)
{
	if (set==NULL)
	{
		return 0;
	}
	if (signum == SIG_CHLD)
	{
		set->sigchild = 1;
		return 1;
	}
	else if (signum == SIG_KILL)
	{
		set->sigkill = 1;
		return 1;
	}
	else if (signum == SIG_UNBLOCK)
	{
		set->sigunblock = 1;
		return 1;
	}
	else if (signum == SIG_CPU)
	{
		set->sigcpu = 1;
		return 1;
	}
	else if (signum == SIG_USER)
	{
		set->siguser = 1;
		return 1;
	}
	return 0;
}

int sigdelset(sigset * set, int signum)
{
	if (set==NULL)
	{
		return 0;
	}
	if (signum == SIG_CHLD)
	{
		set->sigchild = 0;
		return 1;
	}
	else if (signum == SIG_KILL)
	{
		set->sigkill = 0;
		return 1;
	}
	else if (signum == SIG_UNBLOCK)
	{
		set->sigunblock = 0;
		return 1;
	}
	else if (signum == SIG_CPU)
	{
		set->sigcpu = 0;
		return 1;
	}
	else if (signum == SIG_USER)
	{
		set->siguser = 0;
		return 1;
	}
	return 0;
}

int sigprocmask(int how, const sigset * set ,sigset * oldset)
{
	if (set == NULL)
	{
		return 0;
	}
	struct thread* t = thread_current();
	ASSERT(t!=NULL);
	ASSERT(is_thread(t));
	if (oldset != NULL)
	{
		oldset->sigchild = 0;
		oldset->sigkill = 0;
		oldset->sigcpu = 0;
		oldset->siguser = 0;
		oldset->sigunblock = 0;
		if ((t->blockmask & (1<<0))==0)
		{
			oldset->sigchild = 1;
		}
		if ((t->blockmask & (1<<1))==0)
		{
			oldset->sigcpu = 1;
		}
		if ((t->blockmask & (1<<2))==0)
		{
			oldset->sigunblock = 1;
		}
		if ((t->blockmask & (1<<3))==0)
		{
			oldset->siguser = 1;
		}
	}
	
	int a = (1<<4)-1;
	if (how == SIG_BLOCK)
	{
		if (set->sigchild==1)
		{
			a = a & (~(1<<0));
		}
		if (set->sigcpu==1)
		{
			a = a & (~(1<<1));
		}
		if (set->sigunblock==1)
		{
			a = a & (~(1<<2));
		}
		if (set->siguser==1)
		{
			a = a & (~(1<<3));
		}
		t->blockmask = (t->blockmask) & a;
		return 1;
	}
	else if (how == SIG_UNBLOCK_)
	{
		if (set->sigchild==0)
		{
			a = a & (~(1<<0));
		}
		if (set->sigcpu==0)
		{
			a = a & (~(1<<1));
		}
		if (set->sigunblock==0)
		{
			a = a & (~(1<<2));
		}
		if (set->siguser==0)
		{
			a = a & (~(1<<3));
		}
		t->blockmask = (t->blockmask) | a;
		return 1;
	}
	else if (how == SIG_SETMASK)
	{
		if (set->sigchild==1)
		{
			a = a & (~(1<<0));
		}
		if (set->sigcpu==1)
		{
			a = a & (~(1<<1));
		}
		if (set->sigunblock==1)
		{
			a = a & (~(1<<2));
		}
		if (set->siguser==1)
		{
			a = a & (~(1<<3));
		}
		t->blockmask = a;
		return 1;
	}
	return 0;
}







