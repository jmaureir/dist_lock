/**********************************************************
* File Name : UDPDatagram.h
*
* Last Modified : Mon 04 Mar 2013 05:32:07 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __UDPDATAGRAM__
#define __UDPDATAGRAM__

#include "Packet.h"

#include <netinet/in.h>
#include <arpa/inet.h>

/* UDP Datagram abstraction           */
/* containing also the IP information */

class UDPDatagram : public Packet {
protected:
	in_addr       src;
	in_addr       dst;
	unsigned int  s_port;
	unsigned int  d_port;

public:

	UDPDatagram() : Packet() {
	}

	UDPDatagram(UDPDatagram* udp_pkt);

	UDPDatagram(in_addr src, in_addr dst, unsigned int sport, unsigned int dport, unsigned char* payload, int p_len) : Packet() {

		this->src    = src;
		this->dst    = dst;
		this->s_port = sport;
		this->d_port = dport;

		// COPY the payload from the given pointer.
		// payload is the pointer to the receving buffer
		for(int i=0;i<p_len;i++) {
			unsigned char byte = payload[i];
			*this << byte;
		}
	}

	~UDPDatagram();

	in_addr& getSourceAddr() {
		return this->src;
	}

	in_addr& getDestinationAddr() {
		return this->dst;
	}

	void setSourceAddr(in_addr src) {
		this->src = src;
	}

	void setDestinationAddr(in_addr dst) {
		this->dst = dst;
	}

	void setSourcePort(unsigned int sport) {
		this->s_port = sport;
	}

	void setDestinationPort(unsigned int dport) {
		this->d_port = dport;
	}

	unsigned int getSourcePort() {
		return this->s_port;
	}

	unsigned int getDestinationPort() {
		return this->d_port;
	}

	virtual void build() {

	}

};

#endif
