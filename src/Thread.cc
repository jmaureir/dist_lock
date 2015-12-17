/**********************************************************
* File Name : Thread.cc
*
* Last Modified : Mon 04 Mar 2013 05:31:55 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __Thread_CPP__
#define __Thread_CPP__

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "Thread.h"

void micro_sleep(long millis) {
	usleep(millis);
}

void sec_sleep(long sec) {
	sleep(sec);
}

void Thread::start() {
	srand((u_int)pthread_self());
	int e = pthread_create(&this->theThread, &this->tattr, &_run, this);
	if (e!= 0) {
		std::cout << "unable to create Thread : " << strerror(e) << std::endl;
		throw(Exception("Unable to create Thread"));
	}
	e = pthread_attr_setscope(&this->tattr, PTHREAD_SCOPE_SYSTEM);
}

void Thread::join() {
	if (pthread_join(this->theThread, NULL) != 0) {
		throw(Exception("Unable to join Thread"));
	}
}

int Thread::usleep(long micros) {

	int retval = -1;
	if (pthread_mutex_lock(&(this->sleep_mutex))==0) {
		//struct timespec tm;

		//clock_gettime(CLOCK_REALTIME, &tm);
		//tm.tv_nsec = tm.tv_nsec + (long int)(micros * 1000.0);

		timeval now; gettimeofday(&now, NULL); 
		long int abstime_ns_large = now.tv_usec*1000 +  (long int)(micros * 1000.0); 
		timespec abstime = { now.tv_sec + (abstime_ns_large / 1000000000), abstime_ns_large % 1000000000 };

		retval = pthread_cond_timedwait(&(this->cancel_cond), &(this->sleep_mutex),&abstime);
		pthread_mutex_unlock(&(this->sleep_mutex));
	} else {
		std::cout << "usleep could not get the mutex" << std::endl;
	}
	return retval;
}

int Thread::sleep(int sec) {

	int retval = -1;
	if (pthread_mutex_lock(&(this->sleep_mutex))==0) {
		struct timespec tm;
		clock_gettime(CLOCK_REALTIME, &tm);
		tm.tv_sec = tm.tv_sec + sec;
		retval = pthread_cond_timedwait(&(this->cancel_cond), &(this->sleep_mutex),&tm);
		pthread_mutex_unlock(&(this->sleep_mutex));
	} else {
		std::cout << "usleep could not get the mutex" << std::endl;
	}
	return retval;


}

void Thread::wait() {
	if (pthread_mutex_lock(&(this->wait_mutex))==0) {
		pthread_cond_wait(&(this->notify_cond), &(this->wait_mutex));
		pthread_mutex_unlock(&(this->wait_mutex));
	}
}

void Thread::timedWait(int sec,unsigned long int nsec) {
	// WARNING: Bug reported for nanoseconds waiting. pthread_cond_timedwait returns inmmediately

	if (pthread_mutex_lock(&(this->wait_mutex))==0) {
		struct timespec tm;
		clock_gettime(CLOCK_REALTIME, &tm);
		tm.tv_sec = tm.tv_sec + sec;
		tm.tv_nsec = tm.tv_nsec + nsec;
		pthread_cond_timedwait(&(this->notify_cond), &(this->wait_mutex),&tm);
	} else {
		std::cout << "pthread error: wait mutex not adquired." << std::endl;
	}
	pthread_mutex_unlock(&(this->wait_mutex));
}

bool Thread::isWaiting() {

	int ret = pthread_mutex_trylock(&(this->wait_mutex));

	//std::cout << "ret: " << ret << " ebusy " << EBUSY << " einval " << EINVAL  << std::endl;

	if (ret==EBUSY) {
		pthread_mutex_unlock(&(this->wait_mutex));
		//std::cout << "wait mutex locked failed: isWaiting true" << std::endl;
		return true;
	}
	//std::cout << "wait mutex locked ok: isWaiting false" << std::endl;
	pthread_mutex_unlock(&(this->wait_mutex));

	return false;
}

void Thread::notify() {
	pthread_cond_signal(&(this->notify_cond));
}

#endif
