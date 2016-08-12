/* 
 * File          : DistributedLock.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 04:09:39 PM CLT
 * Last Modified : Thu 11 Aug 2016 10:00:53 PM GYT
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
        this->parent->ch->send(beacon_pkt);

        if (cv.wait_for(lk, std::chrono::milliseconds(this->parent->beacon_time)) != std::cv_status::timeout) {
            break;
        }
    }
}

void DistributedLock::Resource::stop() {
    this->running=false;
    cv.notify_all();
}

/* Distributed Lock implementation */

unsigned int DistributedLock::getId() {
    return this->id;
} 

bool DistributedLock::defineResource(std::string res) {
    DistributedLock::Resource* resource = this->createResource(res);
    return resource != NULL;
}

bool DistributedLock::adquire(std::string res) {
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<> dist(0,1);

    while(true) {

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
    }
    return false;
}

bool DistributedLock::release() {
    return true;
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
        //resource->tp    = std::chrono::system_clock::now();
        this->resources.insert(std::make_pair(res, resource));
    }
    return resource;
}

void DistributedLock::release_lock(std::string res) {
    Resource* resource = this->getResource(res);
    if (resource != NULL) {
        resource->stop();
    }
}

void DistributedLock::query_lock(std::string res) {

}

bool DistributedLock::require_lock(std::string res) {

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
                if (resource->getState() == Resource::ADQUIRING && pkt->getState() == Resource::ADQUIRING) {
                    // collision!!!
                    cv.notify_all();
                }
            }

        } else {
            //std::cout << "Arrived packet is from myself. discarding it" << std::endl;
        }
    }
}
