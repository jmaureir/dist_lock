/* 
 * File          : DistributedLock.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 04:07:14 PM CLT
 * Last Modified : Wed 17 Aug 2016 11:57:40 AM CLT
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

class DistributedLock : public ActionListener {
    private:
        class Resource : public Thread, public Observable {
            public:
                enum State {
                    IDLE          = 0,
                    ACQUIRING     = 1,
                    ACQUIRED      = 2,
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
                std::mutex                 members_m;

                std::condition_variable    beacon_cv;
                std::mutex                 beacon_m;

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

                     Debug::getInstance().registerClass<DistributedLock::Resource>(ALL);

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

                State getState() {
                    return this->state;
                }

                void setCount(unsigned int c) {
                    this->count = c;
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

        unsigned long int beacon_time   = 10; // ms 
        unsigned int      sense_beacons = 3;

        unsigned int      retry_max     = 0; // undefined

        typedef std::map<std::string, Resource*> ResourceMap;

        ResourceMap resources;

        std::mutex                 res_m;
        std::mutex                 update_m;

        std::condition_variable    listen_cv;
        std::mutex                 listen_m;

        std::condition_variable    cv;
        std::mutex                 cv_m;

        std::condition_variable    backoff_cv;
        std::mutex                 backoff_m;

        Resource* getResource(std::string res);
        Resource* createResource(std::string res);
        void removeResource(std::string res);

        bool adquire_lock(std::string res);
        void release_lock(std::string res);
        void query_lock(std::string res);

        unsigned int getRandomId();

        unsigned int getId();

        static CommHandler* getCommHandlerInstance(unsigned int port);

        bool listenForPacket(unsigned long int wt);

    public:
        DistributedLock(unsigned int id=0, unsigned int port=5000) {
            try {
                this->ch = DistributedLock::getCommHandlerInstance(port);
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

        bool acquire(std::string resource);
        bool release(std::string resource);
        bool releaseAll();

        bool defineResource(std::string resource,unsigned int count);
    
        void setAdquireMaxRetry(unsigned int n);

        virtual void actionPerformed(ActionEvent* evt);

};
#endif
