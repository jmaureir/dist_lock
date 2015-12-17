/**********************************************************
* File Name : Packet.cc
*
* Last Modified : Mon 04 Mar 2013 05:30:55 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#include "Packet.h"
#include <stdio.h>
#include <string.h>

Packet::Packet() {
	this->buffer.clear();
	this->begin();
}

Packet::Packet(Packet* pkt) {
    *this << *pkt;
    this->begin();
}

Packet::~Packet() {
	this->buffer.clear();
}

void Packet::begin() {
	this->it = this->buffer.begin();
}

size_t Packet::length() {
	return this->buffer.size();
}

std::string Packet::toByte() {
	this->build();
	std::stringstream tmp;
	for(ByteArray::iterator it=this->buffer.begin();it!=this->buffer.end();it++) {
		tmp << (unsigned char)(*it);
	}

	return tmp.str();
}

std::string Packet::dump() {
    std::stringstream tmp;
    tmp << "[" << (void*)this << "] ";
    for(ByteArray::iterator it=this->buffer.begin();it!=this->buffer.end();it++) {
        unsigned int c = (unsigned int)(*it);
        tmp << c << " ";
    }

    return tmp.str();
}
