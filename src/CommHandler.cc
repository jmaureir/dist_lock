/* 
 * File          : CommHandler.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:17:29 PM CLT
 * Last Modified : Mon 14 Dec 2015 11:43:42 PM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */
#include "CommHandler.h"
#include <chrono>

void CommHandler::run() {
    this->running = true;
    while (this->running) {

        this->notify();

        UDPDatagram* pkt = NULL;
        try {
            pkt = this->receive();
        } catch (Exception& e) {
            std::cout << e.what() << std::endl;
        }
        if (pkt != NULL) {
            this->cv.notify_all(); 
            DLPacket* dl_pkt = new DLPacket(pkt);
            this->actionPerformed(new PacketArrivedEvent(dl_pkt));
            delete(dl_pkt);
            delete(pkt);
        }
    }
}

bool CommHandler::send(DLPacket* pkt) {
    pkt->setDestinationPort(this->getPort());
    pkt->setDestinationAddr(this->bcast_addr.getInAddress());
    this->UDPSocket::send(pkt);
    delete(pkt);
}

bool CommHandler::waitForPacket(unsigned long time) {
    std::unique_lock<std::mutex> lk(cv_m);

    auto wait_time = std::chrono::milliseconds(time);

    if (cv.wait_for(lk, wait_time) == std::cv_status::timeout) {
        return false;
    }

    return true;
}
