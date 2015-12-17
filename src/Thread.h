/**********************************************************
* File Name : Thread.h
*
* Last Modified : Mon 04 Mar 2013 05:31:59 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __Thread__
#define __Thread__

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <signal.h>
#include <sched.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "Exception.h"

class Thread {

public:
	private:

		pthread_t       theThread;
		pthread_attr_t  tattr;
		pthread_mutex_t mutex;

		pthread_cond_t  cancel_cond;
		pthread_mutex_t sleep_mutex;

		pthread_cond_t  notify_cond;
		pthread_mutex_t wait_mutex;

		bool done;
		bool notified;

	protected:

		inline static void* _run(void* arg) {
			Thread* t = (Thread*) arg;
			t->thePid = getpid();
			pthread_testcancel();
			t->run();
			t->done = true;

			return t;
		}

		int  thePid;

	public:

		Thread() {
			this->theThread = 0;

			pthread_mutex_init (&(this->mutex) , 0);

			pthread_mutex_init (&(this->wait_mutex) , 0);
			pthread_cond_init (&(this->notify_cond) , 0);

			pthread_mutex_init (&(this->sleep_mutex) , 0);
			pthread_cond_init (&(this->cancel_cond) , 0);

			if (pthread_attr_init(&this->tattr)!=0) {
				throw(Exception("could not initialize the pthread attr"));
			}

			if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)) {
				throw(Exception("could not set the pthread setcancel state"));
			}

			this->done     = false;
			this->notified = false;
		}

		~Thread() {
			//std::cout << "thread destructor" << std::endl;
			pthread_attr_destroy(&(this->tattr));
			pthread_mutex_destroy(&(this->mutex));

			pthread_mutex_destroy(&(this->wait_mutex));
			pthread_cond_destroy(&(this->notify_cond));

			pthread_mutex_destroy(&(this->sleep_mutex));
			pthread_cond_destroy(&(this->cancel_cond));

		}

		pid_t getPid() {
			pid_t tid = syscall(SYS_gettid);
			return tid;
		}

		pthread_t getThreadID() {
			return(pthread_self());
		}


		bool getMutex() {

			//cout << "thread " << this->getThreadID() << " getting mutex" <<  endl;
			if (pthread_mutex_lock(&(this->mutex))!=0) {
				throw(Exception("could not get the mutex"));
				//return false;
			}
			return true;
		}

		void releaseMutex() {
			//cout << "thread " << this->getThreadID() << " releasing mutex" << endl;
			if (pthread_mutex_unlock(&(this->mutex))!=0) {
				throw(Exception("could not release the mutex"));
			}
		}

		bool isDone() {
			return this->done;
		}

		void setDetached() {
			if (pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)!=0) {
				throw(Exception("could not set detached state"));
			}
		}

		virtual void run() {}

		void start();
		void join();
		int usleep(long micros);
		int sleep(int sec=0);

		void wait();
		void notify();

		void timedWait(int sec=0, unsigned long int nsec=0);
		void timedNotify();

		bool isWaiting();

		void setPriority() {

			int sched = SCHED_OTHER;
			//int min = sched_get_priority_min(sched);
			int max = sched_get_priority_max(sched);
			
			struct sched_param main_param;
			memset(&main_param, 0, sizeof(sched_param));
			main_param.sched_priority = max;

			if (pthread_setschedparam(pthread_self(), sched, &main_param)!=0) {
				std::cout << "could not set priority" << std::endl;
			}
		}

		inline void detach() {
			if (this->theThread!=0) {
				if (pthread_detach(this->theThread)!=0) {
					throw(Exception("Unable to cancel the Thread"));
				}
			}
		}

		inline void kill() {
			this->done = true;
			if (this->theThread!=0) {
				if (pthread_cancel(this->theThread)!=0) {
					throw(Exception("Unable to cancel the Thread"));
				}
			}
		}

		bool sendSignal(int signal) {
			if (pthread_kill(this->theThread, signal)!=0) {
				return false;
			}
			return true;
		}

		inline bool cancel() {
			if (pthread_cond_signal(&(this->cancel_cond))!=0) {
				return false;
			}
			return true;
		}

		inline void exit() {
			pthread_exit(NULL);
		}
};

#endif
