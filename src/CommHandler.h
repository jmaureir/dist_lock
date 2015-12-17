/* 
 * File          : CommHandler.h
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:13:44 PM CLT
 * Last Modified : Mon 14 Dec 2015 06:44:00 PM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */

#ifndef __COMMHANDLER__
#define __COMMHANDLER__

#include "UDPSocket.h"
#include "Thread.h"
#include "Observable.h"
#include "InetAddress.h"
#include "DLPacket.h"

#include <condition_variable>
#include <atomic>

class CommHandler : public UDPSocket, public Thread, public Observable {
    private:
        bool          running = false;
        InetAddress   bcast_addr;

        std::condition_variable cv;
        std::mutex cv_m;

    public:

        class PacketArrivedEvent : public ActionEvent {
            private:
                DLPacket* pkt;
            public:
                PacketArrivedEvent(DLPacket* pkt) {
                    this->pkt = pkt;
                }
                DLPacket* getPacket() {
                    return this->pkt;
                }
        };

        CommHandler(int port) : UDPSocket(port), Thread(), Observable() {
            this->setTimeout(0,300); // 300 ns for readining timeout
            this->start();
            this->wait();

            this->bcast_addr = InetAddress("255.255.255.255");

        } 

        ~CommHandler() {
            this->running = false;
            this->close();
            this->join();
        }

        virtual bool send(DLPacket* pkt);

        bool waitForPacket(unsigned long time);

        virtual void run();

};

#endif
