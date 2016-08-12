/* 
 * File          : DistributedLock.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 04:07:14 PM CLT
 * Last Modified : Thu 11 Aug 2016 10:46:13 PM GYT
 *
 * (c) 2015-2016 Juan Carlos Maureira
 */
#ifndef __DISTRIBUTEDLOCK__
#define __DISTRIBUTEDLOCK__

#include "ActionListener.h"
#include "CommHandler.h"

#include <random>
#include <climits>
#include <condition_variable>
#include <atomic>
#include <map>
#include <ctime>
#include <ratio>
#include <chrono>

#include "Debug.h"

class DistributedLock : public ActionListener, public Debug {
    private:
        CommHandler* ch;
        unsigned int id;

        unsigned long int beacon_time   = 100; // ms 
        unsigned int      sense_beacons = 3;
        unsigned int      backoff_time  = 500; // ms

        class Resource : public Thread, public Observable {
            public:
                enum State {
                    IDLE          = 0,
                    ADQUIRING     = 1,
                    ADQUIRED      = 2,
                    RELEASED      = 3,
                };
            private:
                DistributedLock*           parent;
                std::atomic<bool>          running;
                std::string                name;
               
                std::condition_variable    cv;
                std::mutex                 cv_m;
                std::atomic<unsigned int>  owner;
                std::atomic<State>         state; 

                std::atomic<unsigned int>  count;

                std::chrono::system_clock::time_point tp;

            public:

                Resource(DistributedLock* p, std::string name) : Thread() {
                    this->name       = name;
                    this->parent     = p;
                    this->running    = true;
                    this->state      = IDLE;
                    this->owner      = p->id;
                    this->count      = 1;
                }
                ~Resource() {
                    this->stop();
                    this->join();
                }
                virtual void run();
                virtual void stop();

                State getState() {
                    return this->state;
                }

                void setCount(unsigned int c) {
                    this->count = c;
                }

                void setState(State s) {
                    this->state = s;
                }

        };

        typedef std::map<std::string, Resource*> ResourceMap;

        ResourceMap resources;

        std::mutex                 res_m;
        std::condition_variable    cv;
        std::mutex                 cv_m;

        Resource* getResource(std::string res);
        Resource* createResource(std::string res);
        void removeResource(std::string res);

        bool require_lock(std::string res);
        void release_lock(std::string res);
        void query_lock(std::string res);

        unsigned int getRandomId();

        unsigned int getId();

    public:
        DistributedLock(unsigned int id=0, unsigned int port=5000) {
            try {
                this->ch = new CommHandler(port);
            } catch (Exception& e) {
                throw(e);
            }
            this->ch->addActionListener(this);

            if (id == 0) { 
                this->id = this->getRandomId();
            } else {
                this->id = id;
            }
       }

        ~DistributedLock() {

<<<<<<< HEAD
=======
            std::cout << "releasing resources" << std::endl;

>>>>>>> 436228823677d852129c7e50083ff855063b38a2
            for(auto it=this->resources.begin();it!=this->resources.end();it++) {
                std::string res = (*it).first;
                this->release_lock(res);
            }

            delete(this->ch);
        }

        bool adquire(std::string resource);
        bool release();
        bool defineResource(std::string resource,unsigned int count);

        virtual void actionPerformed(ActionEvent* evt);

};
#endif
