/* 
 * File          : DistributedLock.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 04:09:39 PM CLT
 * Last Modified : Fri 12 Aug 2016 12:44:37 AM GYT
 * Last Modified : Fri 12 Aug 2016 12:44:37 AM GYT
 *
 * (c) 2015-2016 Juan Carlos Maureira
 */
#include "DistributedLock.h"
#include "DLPacket.h"
#include <thread>

/* Resource Implementation */
void DistributedLock::Resource::run() {
    std::unique_lock<std::mutex> lk(cv_m);

    unsigned int id = this->parent->getId();

    while (this->running) {
        
        DLPacket* beacon_pkt = new DLPacket(this->state,id);

        beacon_pkt->setResource(this->name);
        beacon_pkt->setCount(this->count);
        this->parent->ch->send(beacon_pkt);

        if (cv.wait_for(lk, std::chrono::milliseconds(this->parent->beacon_time)) != std::cv_status::timeout) {
            break;
        }
    }
}

void DistributedLock::Resource::updateMember(unsigned int id, State state, unsigned int count) {

    Member* m = this->getMember(id);
    if (m==NULL) {
        m = this->addMember(id);
    }
    m->count = count;
    m->tp    = std::chrono::system_clock::now();
    m->state = state;
}

void DistributedLock::Resource::stop() {
    this->running=false;
    cv.notify_all();
}

DistributedLock::Resource::Member* DistributedLock::Resource::getMember(unsigned int id) {
    if (this->members.find(id)!=this->members.end()) {
        return this->members[id];
    }
    return NULL;
}

DistributedLock::Resource::Member* DistributedLock::Resource::addMember(unsigned int id) {
    Member* member = this->getMember(id);
    if (member == NULL) {
        member = new Member(id);
        //TODO use a mutex for changing map
        this->members[id] = member;
    }
    return member;
}

bool DistributedLock::Resource::removeMember(unsigned int id) {
    Member* member = this->getMember(id);
    if (member!=NULL) {
        this->members.erase(id);
        delete(member);
        return true;
    }
    return false;
}

/* Distributed Lock implementation */

unsigned int DistributedLock::getId() {
    return this->id;
} 

void DistributedLock::setAdquireMaxRetry(unsigned int n) {
    this->retry_max = n;
}

bool DistributedLock::defineResource(std::string res, unsigned int count=1) {
    DistributedLock::Resource* resource = this->createResource(res);
    if (count>1) {
        resource->setCount(count);
    }

    return resource != NULL;
}

bool DistributedLock::adquire(std::string res) {
    return this->adquire_lock(res);
}

bool DistributedLock::release(std::string res) {
    try {
        this->release_lock(res);
    } catch(Exception& e) {
        return false;
    }
    return true;
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
    if (this->resources.find(res) != this->resources.end()) {
        // resource already in our map 
        Resource* resource = this->resources[res];
        return resource;
    }
    return NULL;
}

void DistributedLock::removeResource(std::string res) {
    if (this->resources.find(res) != this->resources.end()) {
        //std::cout << this->id << " removing resource " << res << std::endl;
        Resource* resource = this->resources[res];
        this->resources.erase(res);
        delete(resource);
    }
}

DistributedLock::Resource* DistributedLock::createResource(std::string res) {
    Resource* resource = NULL;
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
            resource->stop();
        } catch(Exception& e) {
            throw(e);
        }
    }
}

void DistributedLock::query_lock(std::string res) {
    // TODO: to be implemented
}

bool DistributedLock::adquire_lock(std::string res) {
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<> dist(0,1);

    unsigned int retry = 0;

    while((this->retry_max == 0) || retry < this->retry_max) {

        unsigned long int wait_time = this->beacon_time * (this->sense_beacons * (1+dist(rng)));  
        //std::cout << this->id << " trying to adquire " << res << " " << wait_time <<std::endl;

        if (!this->ch->waitForPacket(wait_time)) {
            // no packet arrive for 1000 ms
            Resource* resource = this->createResource(res);
            resource->setState(Resource::ADQUIRING);
            resource->start(); 

            // adquiring time for collision detection
            auto adquiring_time = std::chrono::milliseconds(  this->beacon_time  * this->sense_beacons );
            std::unique_lock<std::mutex> lk(cv_m);
            if (cv.wait_for(lk, adquiring_time) == std::cv_status::timeout) {
                resource->setState(Resource::ADQUIRED);
                return true;
            }
            this->removeResource(res);
            std::cout << "*** Colision Detected!!! Forcing Backoff" << std::endl;
        }
        
        unsigned long int backoff_time = (this->beacon_time * 2 * this->sense_beacons) * (1+dist(rng)) ;
        //std::cout << this->id << " failed attempt to adquire " << res << " backoff " << backoff_time << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_time));

        retry++;
    }

    //std::cout << "not adquired" << std::endl;

    return false;
}

void DistributedLock::actionPerformed(ActionEvent* evt) {
    if (dynamic_cast<CommHandler::PacketArrivedEvent*>(evt)) {
        CommHandler::PacketArrivedEvent* pkt_evt = dynamic_cast<CommHandler::PacketArrivedEvent*>(evt);
        DLPacket* pkt = pkt_evt->getPacket();

        if (pkt->getMemberId() != this->id) {
            std::string res = pkt->getResource();

            Resource* resource = this->getResource(res);
            if (resource != NULL) {
                Resource::State state = (Resource::State)pkt->getState();
                resource->updateMember(pkt->getMemberId(), state, pkt->getCount());

                if (resource->getState() == Resource::ADQUIRING && pkt->getState() == Resource::ADQUIRING) {
                    // collision!!!
                    cv.notify_all();
                }

                std::cout << *resource << std::endl;

            }

        } else {
            //std::cout << "Arrived packet is from myself. discarding it" << std::endl;
        }
    }
}
