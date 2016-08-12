/* 
 * File          : DistributedLock.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 04:07:14 PM CLT
 * Last Modified : Fri 12 Aug 2016 12:45:28 AM GYT
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
        class Resource : public Thread, public Observable {
            public:
                enum State {
                    IDLE          = 0,
                    ADQUIRING     = 1,
                    ADQUIRED      = 2,
                    RELEASED      = 3,
                };

                struct Member {
                    unsigned int id;
                    std::chrono::system_clock::time_point tp;
                    unsigned int count;
                    State state;
                    Member(unsigned int _id) : id(_id) {
                        this->count = 0;
                        this->tp = std::chrono::system_clock::now();
                        this->state = IDLE;
                    }
                };

                typedef std::map<unsigned int,Member*> MemberList;
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

                MemberList                 members;

                Member* getMember(unsigned int id);
                Member* addMember(unsigned int id);
                bool removeMember(unsigned int id);

            public:

                Resource(DistributedLock* p, std::string name) : Thread() {
                    this->name       = name;
                    this->parent     = p;
                    this->running    = true;
                    this->state      = IDLE;
                    this->owner      = p->id;
                    this->count      = 1;

                    this->tp         = std::chrono::system_clock::now();
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

                void updateMember(unsigned int id, State state, unsigned int count);

                friend std::ostream& operator << (std::ostream& os, Resource& obj) {
                    os << obj.name << ", " << obj.state << ", " << obj.members.size() << "[";
                    for(auto it=obj.members.begin();it!=obj.members.end();it++) {
                        Member* m = (*it).second;
                        os << m->id << ", " << m->state << ", " << m->count << ", " ;
                    }
                    os << "]";
                    return os;
                }
        };

    private:
        CommHandler* ch;
        unsigned int id;

        unsigned long int beacon_time   = 100; // ms 
        unsigned int      sense_beacons = 3;
        unsigned int      backoff_time  = 500; // ms

        unsigned int      retry_max     = 0; // undefined

        typedef std::map<std::string, Resource*> ResourceMap;

        ResourceMap resources;

        std::mutex                 res_m;
        std::condition_variable    cv;
        std::mutex                 cv_m;

        Resource* getResource(std::string res);
        Resource* createResource(std::string res);
        void removeResource(std::string res);

        bool adquire_lock(std::string res);
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
            for(auto it=this->resources.begin();it!=this->resources.end();it++) {
                std::string res = (*it).first;
                this->release_lock(res);
            }

            delete(this->ch);
        }

        bool adquire(std::string resource);
        bool release(std::string resource);
        bool releaseAll();

        bool defineResource(std::string resource,unsigned int count);
    
        void setAdquireMaxRetry(unsigned int n);

        virtual void actionPerformed(ActionEvent* evt);

};
#endif
