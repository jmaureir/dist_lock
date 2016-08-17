/* 
 * File          : CommHandler.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:17:29 PM CLT
 * Last Modified : Wed 17 Aug 2016 11:35:33 AM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */
#include "CommHandler.h"
#include "Debug.h"
#include <chrono>

RegisterDebugClass(CommHandler,NONE)

void CommHandler::run() {
    this->running = true;
    while (this->running) {

        this->notify();

        UDPDatagram* pkt = NULL;
        try {
            pkt = this->receive();
        } catch (Exception& e) {
            debug << e.what() << std::endl;
        }
        if (pkt != NULL) {
            DLPacket* dl_pkt = new DLPacket(pkt);

            if (this->running) {
                this->cv.notify_all(); 
                this->actionPerformed(new PacketArrivedEvent(dl_pkt));
            }
            delete(dl_pkt);
            delete(pkt);
        }
    }
    debug << "CommHandler finising" << std::endl;
}

void CommHandler::stop() {
    debug << "stopping CommHandler" << std::endl;
    this->running = false;
    this->removeAllActionListeners();
    this->notify();
    this->cv.notify_all();
    this->close();
    try {
        this->join();
        debug << "CommHandler Joined" << std::endl;
    } catch(Exception& e) {
        debug << "Could not join commhandler." << std::endl;
    }
}

bool CommHandler::send(DLPacket* pkt) {
    pkt->setDestinationPort(this->getPort());
    pkt->setDestinationAddr(this->bcast_addr.getInAddress());
    bool r = this->UDPSocket::send(pkt);
    delete(pkt);
    return r;
}

bool CommHandler::waitForPacket(unsigned long time) {
    std::unique_lock<std::mutex> lk(cv_m);

    auto wait_time = std::chrono::milliseconds(time);

    if (cv.wait_for(lk, wait_time) == std::cv_status::timeout) {
        return false;
    }

    return true;
}
