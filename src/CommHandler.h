/* 
 * File          : CommHandler.h
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:13:44 PM CLT
 * Last Modified : Tue 16 Aug 2016 03:16:20 PM CLT
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
        std::atomic<bool>       running;
        InetAddress             bcast_addr;

        std::condition_variable cv;
        std::mutex              cv_m;

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
            this->running = false;
            this->setTimeout(0,300); // 300 ns for readining timeout
            this->start();
            this->wait();

            this->bcast_addr = InetAddress("127.255.255.255");

        } 

        ~CommHandler() {
        
            if (this->running) {
                this->stop();
            }
        }

        virtual void stop();
        virtual bool send(DLPacket* pkt);

        bool waitForPacket(unsigned long time);

        virtual void run();

};

#endif
