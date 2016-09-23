/* 
 * File          : DistributedLock.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 04:07:14 PM CLT
 * Last Modified : Fri 23 Sep 2016 11:47:54 AM CLT
 *
 * (c) 2015-2016 Juan Carlos Maureira
 * (c) 2016      Andrew Hart
 */
#ifndef __DISTRIBUTEDLOCK__
#define __DISTRIBUTEDLOCK__

#include "ActionListener.h"
#include "CommHandler.h"
#include "Debug.h"

#include <random>
#include <climits>
#include <condition_variable>
#include <atomic>
#include <map>
#include <ctime>
#include <ratio>
#include <chrono>

#define MAX_BACKOFF      6
#define SENSE_BEACONS    3

class DistributedLock : public ActionListener {
    private:
        class Resource : public Thread, public Observable {
            public:
                enum State {
                    IDLE          = 0,
                    STARTING      = 1,
                    ACQUIRING     = 2,
                    ACQUIRED      = 3,
                    RELEASED      = 4,
                    QUERYING      = 5,
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
                    Member(const Member& m) {
                        this->id    = m.id;
                        this->tp    = m.tp;
                        this->count = m.count;
                        this->state = m.state;
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
                std::mutex                 members_m;

                std::condition_variable    beacon_cv;
                std::mutex                 beacon_m;

                std::condition_variable    started_cv;
                std::mutex                 started_m;

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

                    this->start();
                }
                ~Resource() {

                    for(auto it=this->members.begin();it!=this->members.end();) {
                        Member* m = (*it).second;
                        this->members.erase(it++);
                        delete(m);
                    }

                    if (this->running) {
                        this->stop();
                    }
                }
                virtual void run();
                virtual void stop();
                void waitForReady();

                State getState() {
                    return this->state;
                }

                void setCount(unsigned int c) {
                    this->count = c;
                }

                std::string getName() {
                    return this->name;
                }

                unsigned int getCount() {
                    return this->count;
                }

                void setState(State s) {
                    this->state = s;
                    this->sendBeacon();
                    this->beacon_cv.notify_all();
                }

                void reset();

                MemberList& getMemberList() {
                    return this->members;
                }

                void updateMember(unsigned int id, State state, unsigned int count);
                bool isAcquirable();
                void sendBeacon();

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

        static CommHandler* s_ch;
        static std::atomic<int> s_ch_count;

        CommHandler* ch;
        unsigned int id;

        unsigned long int beacon_time   = 50; // ms 
        unsigned int      sense_beacons = SENSE_BEACONS;
        unsigned int      max_backoff   = MAX_BACKOFF;

        unsigned int      retry_max     = 0; // undefined

        typedef std::map<std::string, Resource*> ResourceMap;

        ResourceMap resources;

        std::mutex                 res_m;
        std::mutex                 update_m;

        std::condition_variable    listen_cv;
        std::mutex                 listen_m;

        std::condition_variable    any_cv;
        std::mutex                 any_m;

        std::condition_variable    cv;
        std::mutex                 cv_m;

        std::condition_variable    backoff_cv;
        std::mutex                 backoff_m;

        Resource* getResource(std::string res);
        Resource* createResource(std::string res);
        void removeResource(std::string res);

        bool adquire_lock(std::string res);
        bool adquire_lock();
        void release_lock(std::string res);
        Resource::MemberList query_lock(std::string res);

        std::function<void()> on_start;

        unsigned int getRandomId();


        static CommHandler* getCommHandlerInstance(std::string bcast_addr, unsigned int port);

        bool listenForPacket(unsigned long int wt);
        bool listenForAnyPacket(unsigned long int wt);

    public:
        DistributedLock(unsigned int id=0, unsigned int port=5000,unsigned int beacon_time = 50, std::string bcast_addr = "127.255.255.255") {
            try {
                this->ch = DistributedLock::getCommHandlerInstance(bcast_addr,port);
            } catch (Exception& e) {
                throw(e);
            }
            this->ch->addActionListener(this);

            if (id == 0) { 
                this->id = this->getRandomId();
            } else {
                this->id = id;
            }

            this->beacon_time = beacon_time;
       }

        ~DistributedLock() {
            this->releaseAll();
            this->ch->removeActionListener(this);
            {
                std::lock_guard<std::mutex> lock(this->update_m);

                for(auto it=this->resources.begin();it!=this->resources.end();) {
                    std::string res = (*it).first;
                    Resource* r = (*it).second;
                    r->stop();

                    this->resources.erase(it++);
                    delete(r);
                }
            }

            DistributedLock::s_ch_count--;
            //std::cout << DistributedLock::s_ch_count  << std::endl;
            if (DistributedLock::s_ch_count==0) {
                delete(this->ch);
            }
        }

        unsigned int getId();

        bool acquire(std::string resource="");
        bool release(std::string resource);
        bool releaseAll();

        bool isBusy(std::string resource);
        bool isAny(std::string resource);
        bool isWaiting(std::string resource);

        bool defineResource(std::string resource,unsigned int count=1);
    
        void setAdquireMaxRetry(unsigned int n);

        void onStart(std::function<void()> handler) {
            this->on_start = handler;
        }

        virtual void actionPerformed(ActionEvent* evt);

};
#endif
