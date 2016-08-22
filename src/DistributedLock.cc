/* 
 * File          : DistributedLock.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 04:09:39 PM CLT
 * Last Modified : Fri 19 Aug 2016 06:54:14 PM CLT
 *
 * (c) 2015-2016 Juan Carlos Maureira
 * (c) 2016      Andrew Hart
 */
#include "DistributedLock.h"
#include "DLPacket.h"

#include <thread>
#include <chrono>
#include <unistd.h>


RegisterDebugClass(DistributedLock,NONE)

CommHandler* DistributedLock::s_ch = NULL;
std::atomic<int> DistributedLock::s_ch_count;

CommHandler* DistributedLock::getCommHandlerInstance(std::string bcast_addr, unsigned int port) {
    if (DistributedLock::s_ch == NULL) {
        try {
            DistributedLock::s_ch = new CommHandler(bcast_addr,port);
        } catch(Exception& e) {
            throw(e);
        }
    }
    DistributedLock::s_ch_count++;

    //std::cout << DistributedLock::s_ch_count << std::endl;

    return DistributedLock::s_ch;
}

/* Resource Implementation */

void DistributedLock::Resource::sendBeacon() {
    unsigned int id = this->parent->getId();

    DLPacket* beacon_pkt = new DLPacket(this->state,id);

    beacon_pkt->setResource(this->name);
    beacon_pkt->setCount(this->count);
    if (!this->parent->ch->send(beacon_pkt)) {
        debug << "Error sending the beacon packet" << std::endl;
    }
}

void DistributedLock::Resource::run() {
    std::unique_lock<std::mutex> lk(cv_m);

    while (this->running) {

        if (this->state == IDLE) {
            std::unique_lock<std::mutex> lock(beacon_m);
            this->beacon_cv.wait(lock);
        } else {
            this->sendBeacon();
        }
        if (cv.wait_for(lk, std::chrono::milliseconds(this->parent->beacon_time)) != std::cv_status::timeout) {
            break;
        }
    }

    //debug << "Resource thread finished" << std::endl;
}

void DistributedLock::Resource::updateMember(unsigned int id, State state, unsigned int count) {
    if (this->count == count || this->state == QUERYING) {
        // only account the members reporting the same count number
        Member* m = this->getMember(id);
        if (m==NULL) {
            m = this->addMember(id);
        }
        {
            std::lock_guard<std::mutex> lock(this->members_m);
            m->count = count;
            m->tp    = std::chrono::system_clock::now();
            m->state = state;
        }
    }
}

void DistributedLock::Resource::stop() {
    if (this->running) {

        this->running=false;
        cv.notify_all();
        try {
            this->join();
        } catch (Exception& e) {
            std::cerr << "could not join resource thread" << std::endl; 
        }
    }
}

DistributedLock::Resource::Member* DistributedLock::Resource::getMember(unsigned int id) {
    std::lock_guard<std::mutex> lock(this->members_m);
    if (this->members.find(id)!=this->members.end()) {
        return this->members[id];
    }
    return NULL;
}

DistributedLock::Resource::Member* DistributedLock::Resource::addMember(unsigned int id) {
    Member* member = this->getMember(id);
    if (member == NULL) {
        std::lock_guard<std::mutex> lock(this->members_m);
        member = new Member(id);
        //TODO use a mutex for changing map
        this->members[id] = member;
    }
    return member;
}

bool DistributedLock::Resource::removeMember(unsigned int id) {
    Member* member = this->getMember(id);
    if (member!=NULL) {
        std::lock_guard<std::mutex> lock(this->members_m);
        this->members.erase(id);
        delete(member);
        return true;
    }
    return false;
}

bool DistributedLock::Resource::isAcquirable() {
    unsigned int num_members = 0;
    for(auto it=this->members.begin();it!=this->members.end();it++) {
        Member* m = (*it).second;
        if (m->state == ACQUIRED) {
            num_members++;
        }
    }

    //debug << "current members " << num_members << " count " << this->count << std::endl;

    return (num_members < this->count);
}

void DistributedLock::Resource::reset() {
    this->setState(IDLE);
    std::lock_guard<std::mutex> lock(this->members_m);
    for(auto it=this->members.begin();it!=this->members.end();) {
        Member* m = (*it).second;
        this->members.erase(it++);
        delete(m);
    }
}

/* Distributed Lock implementation */

unsigned int DistributedLock::getId() {
    return this->id;
} 

void DistributedLock::setAdquireMaxRetry(unsigned int n) {
    this->retry_max = n;
}

bool DistributedLock::defineResource(std::string res, unsigned int count) {
    DistributedLock::Resource* resource = this->createResource(res);
    if (count>1) {
        resource->setCount(count);
    }

    return resource != NULL;
}

bool DistributedLock::acquire(std::string res) {
    if (res!="") {
        return this->adquire_lock(res);
    } else {
        return this->adquire_lock();
    }
}

bool DistributedLock::release(std::string res) {
    try {
        this->release_lock(res);
    } catch(Exception& e) {
        return false;
    }
    return true;
}

bool DistributedLock::isBusy(std::string res) {
    bool ret = false;
    try {
        Resource::MemberList m_list = this->query_lock(res);

        for(auto it=m_list.begin();it!=m_list.end();it++) {
            Resource::Member* m = (*it).second;
            if (m->state == Resource::ACQUIRED) {
                ret = true;
                break;
            }
        }

        // dispose returned list since it is a copy
        for(auto it=m_list.begin();it!=m_list.end();) {
            Resource::Member* m = (*it).second;
            m_list.erase(it++);
            delete(m);
        }

    } catch(Exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return ret;
}

bool DistributedLock::releaseAll() {
    bool r = true;
    for(auto it=this->resources.begin();it!=this->resources.end();it++) {
        std::string res = (*it).first;
        try {
            this->release_lock(res);
        } catch(Exception& e) {
            r = false;
        }
    }
    return r;
}

unsigned int DistributedLock::getRandomId() {
    // generate random id
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,INT_MAX);
    return dist(rng);
}

DistributedLock::Resource* DistributedLock::getResource(std::string res) {
    std::lock_guard<std::mutex> lock(this->res_m);
    if (this->resources.find(res) != this->resources.end()) {
        // resource already in our map 
        Resource* resource = this->resources[res];
        return resource;
    }
    return NULL;
}

void DistributedLock::removeResource(std::string res) {
    std::lock_guard<std::mutex> lock(this->res_m);

    if (this->resources.find(res) != this->resources.end()) {
        //debug << this->id << " removing resource " << res << std::endl;
        Resource* resource = this->resources[res];
        this->resources.erase(res);
        delete(resource);
    }
}

DistributedLock::Resource* DistributedLock::createResource(std::string res) {
    Resource* resource = NULL;
    std::lock_guard<std::mutex> lock(this->res_m);
 
    if (this->resources.find(res) != this->resources.end()) {
        // resource already in our map 
        resource = this->resources[res];
    } else {
        resource = new Resource(this,res);
        this->resources.insert(std::make_pair(res, resource));
    }
    return resource;
}

void DistributedLock::release_lock(std::string res) {
    Resource* resource = this->getResource(res);
    if (resource != NULL) {
        try {
            resource->setState(Resource::RELEASED);
            resource->stop();
        } catch(Exception& e) {
            throw(e);
        }
    }
}

DistributedLock::Resource::MemberList DistributedLock::query_lock(std::string res) {
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<> dist(0,1);

    unsigned int retry = 0;

    Resource* resource = this->getResource(res);
    if (resource == NULL) {
        resource = createResource(res);
    }

    resource->setState(Resource::QUERYING);

    auto long_listen_time = this->beacon_time * 2 * this->max_backoff;

    auto short_listen_time = this->beacon_time * (this->sense_beacons + 1);

    Resource::MemberList n_list;

    if (!this->listenForAnyPacket(short_listen_time)) {
        debug << "short listen complete" << std::endl;
    } else {
        while (long_listen_time > 0) {
            const auto start = std::chrono::system_clock::now();
            debug << "short listen interrupted. extending for " << long_listen_time << std::endl;
            if (!this->listenForAnyPacket(long_listen_time)) {
                debug << "long listen complete" << std::endl;
                break;
            } else {
                debug << "long listen time interrupted" << std::endl;
                const auto stop = std::chrono::system_clock::now();
                double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
                long_listen_time = long_listen_time - elapsed;
                
            }
        }
    }
  
    Resource::MemberList& m_list = resource->getMemberList();
    for(auto it=m_list.begin();it!=m_list.end();it++) {
        Resource::Member* m_o = (*it).second;
        Resource::Member* m = new Resource::Member(*m_o);
        // debug << "m " << m->id << " " << m->state << " " << m << " " << m_o << std::endl;
        n_list[m->id] = m;
    }

    return n_list;
}

bool DistributedLock::adquire_lock(std::string res) {
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<> dist(0,1);

    unsigned int retry = 0;

    Resource* resource = this->getResource(res);
    if (resource == NULL) {
        resource = createResource(res);
    }


    while((this->retry_max == 0) || retry < this->retry_max) {
        resource->setState(Resource::STARTING);

        unsigned long int wait_time = this->beacon_time * (this->sense_beacons * (1+dist(rng)));  

        //debug << this->id << " trying to adquire " << res << " " << wait_time << " " << resource->getState() <<std::endl;

        if (!this->listenForPacket(wait_time)) {
            resource->setState(Resource::ACQUIRING);

            // adquiring time for collision detection
            auto adquiring_time = std::chrono::milliseconds(  this->beacon_time  * this->sense_beacons );
            std::unique_lock<std::mutex> lk(cv_m);
            if (cv.wait_for(lk, adquiring_time) == std::cv_status::timeout) {

                if (resource->isAcquirable()) {
                    resource->setState(Resource::ACQUIRED);

                    //debug << this->id << " *** Resource Adquired" << std::endl;
                    return true;
                } else {
                    debug << this->id << " *** Resource complete!. Falling in Backoff" << std::endl;
                }
            }
            resource->reset();

            retry++;
        }
        unsigned long int backoff_time = (this->beacon_time * this->max_backoff) * (1+dist(rng)) ;
        //debug << this->id << " failed attempt to adquire " << res << " backoff " << backoff_time << std::endl;
        std::unique_lock<std::mutex> bk_lk(cv_m);
        this->backoff_cv.wait_for(bk_lk, std::chrono::milliseconds(backoff_time));
    }

    //debug << "not adquired" << std::endl;

    return false;
}

bool DistributedLock::adquire_lock() {
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<> dist(0,1);

    unsigned int retry = 0;

    if (this->resources.size() == 0) {
        debug << "no resources defined" << std::endl;
        return false;
    }

    if (this->on_start) {
        this->on_start();
    }

    while((this->retry_max == 0) || retry < this->retry_max) {

        for(auto it=this->resources.begin();it!=this->resources.end();it++) {
            Resource* resource = (*it).second;
            resource->setState(Resource::STARTING);

            debug << this->id << " starting " << resource->getName() << std::endl;

        }

        unsigned long int wait_time = this->beacon_time * (this->sense_beacons * (1+dist(rng)));  

        debug << this->id << " trying to adquire " << wait_time << std::endl;

        if (!this->listenForPacket(wait_time)) {

            debug << this->id << " issuing adquiring" << std::endl;

            for(auto it=this->resources.begin();it!=this->resources.end();it++) {
                Resource* resource = (*it).second;
                resource->setState(Resource::ACQUIRING);
            }

            // adquiring time for collision detection
            auto adquiring_time = std::chrono::milliseconds(  this->beacon_time  * this->sense_beacons );
            std::unique_lock<std::mutex> lk(cv_m);
            if (cv.wait_for(lk, adquiring_time) == std::cv_status::timeout) {

                debug << this->id << " checking whether or not are acquirable" << std::endl;

                bool all_acquirable = true;
                std::string busy_res = "";
                for(auto it=this->resources.begin();it!=this->resources.end();it++) {
                    Resource* resource = (*it).second;
                    if (resource->isAcquirable()) {
                        resource->setState(Resource::ACQUIRED);
                    } else {
                        all_acquirable = false; 
                        busy_res = (*it).first;
                        break;
                    }
                }
                if (all_acquirable) {
                    debug << this->id << " *** Resource Adquired" << std::endl;
                    return true;
                } else {
                    debug << this->id << " *** Resource " << busy_res << " complete!. Falling in Backoff" << std::endl;
                }
            }
            for(auto it=this->resources.begin();it!=this->resources.end();it++) {
                Resource* resource = (*it).second;
                resource->reset();
            }
            retry++;
        }
        unsigned long int backoff_time = (this->beacon_time * 2 * this->sense_beacons) * (1+dist(rng)) ;
        debug << this->id << " failed attempt to adquire backoff " << backoff_time << std::endl;
        std::unique_lock<std::mutex> bk_lk(cv_m);
        this->backoff_cv.wait_for(bk_lk, std::chrono::milliseconds(backoff_time));
    }

    //debug << "not adquired" << std::endl;

    return false;
}

bool DistributedLock::listenForPacket(unsigned long int wt) {
    std::unique_lock<std::mutex> lk(listen_m);

    auto wait_time = std::chrono::milliseconds(wt);
    if (listen_cv.wait_for(lk, wait_time) == std::cv_status::timeout) {
        return false;
    }
    return true;
}

bool DistributedLock::listenForAnyPacket(unsigned long int wt) {
    std::unique_lock<std::mutex> lk(any_m);

    auto wait_time = std::chrono::milliseconds(wt);
    if (any_cv.wait_for(lk, wait_time) == std::cv_status::timeout) {
        return false;
    }
    return true;
}

void DistributedLock::actionPerformed(ActionEvent* evt) {
    if (dynamic_cast<CommHandler::PacketArrivedEvent*>(evt)) {
        CommHandler::PacketArrivedEvent* pkt_evt = dynamic_cast<CommHandler::PacketArrivedEvent*>(evt);
        DLPacket* pkt = pkt_evt->getPacket();

        if (pkt->getMemberId() != this->id) {

            std::string res = pkt->getResource();
            {
                std::lock_guard<std::mutex> lock(this->update_m);
                Resource* resource = this->getResource(res);
                if (resource != NULL) {
                    if (resource->getState()!=Resource::IDLE) {
                        Resource::State state = (Resource::State)pkt->getState();

                        resource->updateMember(pkt->getMemberId(), state, pkt->getCount());
                    }

                    any_cv.notify_one();

                    if (pkt->getState() == Resource::ACQUIRING) {
                        listen_cv.notify_one();

                        if (resource->getState() == Resource::ACQUIRING || resource->getState() == Resource::ACQUIRED) {
                            cv.notify_one();
                        }
                    }
                }
            }
        } else {
            //debug << "Arrived packet is from myself. discarding it" << std::endl;
        }
    }
}
