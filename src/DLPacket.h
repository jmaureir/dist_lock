#ifndef __DLPACKET__
#define __DLPACKET__

#include "UDPDatagram.h"

#include <netinet/in.h>
#include <string.h>

#define RES_BUFF  64

class DLPacket : public UDPDatagram {

	public:
        enum Option {
            RETRY  = 1,
            BEACON = 2,
        };

	private:
		unsigned char     state;
        unsigned int      member_id;
        unsigned char     options;
        unsigned int      count;
        std::string       resource;

	public:

		DLPacket(unsigned char s,unsigned int id) : UDPDatagram() {
			this->state     = s;
            this->member_id = id;
            this->options   = 0;
            this->count     =1;
		}

		DLPacket(const DLPacket& p) {
			this->state     = p.state;
            this->member_id = p.member_id;
            this->options   = p.options;
            this->resource  = p.resource;
            this->count     = p.count;
			this->src       = p.src;
			this->dst       = p.dst;
			this->s_port    = p.s_port;
			this->d_port    = p.d_port;
		}

		DLPacket(UDPDatagram* dgram);

		~DLPacket();

		unsigned char getState();
        unsigned int getMemberId();
        std::string getResource();
        unsigned int getCount();

        bool hasOption(Option opt);
        void setOption(Option opt);
        void setResource(std::string res);
        void setCount(unsigned int c);

		virtual void build();
};

#endif
