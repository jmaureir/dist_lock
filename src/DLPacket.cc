#include "DLPacket.h"

/* build the update packet from an upd datagram */
DLPacket::DLPacket(UDPDatagram* dgram) : UDPDatagram(dgram) {

    *this >> this->state;
    *this >> this->member_id;
    *this >> this->options;

    std::stringstream res("");
    unsigned char byte;
    for(int i=0;i < RES_BUFF;i++) {
        *this >> byte;
        res << byte;
        if (byte == 0) {
            break;
        }
    }

    this->resource = res.str().c_str();
}

DLPacket::~DLPacket() {
}

unsigned char DLPacket::getState() {
	return this->state;
}

unsigned int DLPacket::getMemberId() {
	return this->member_id;
}

std::string DLPacket::getResource() {
    return this->resource;
}

void DLPacket::setResource(std::string res) {
    this->resource = res;
}

bool DLPacket::hasOption(DLPacket::Option opt) {
    if (this->options & opt) {
        return true;
    }
    return false;
}

void DLPacket::setOption(DLPacket::Option opt) {
    this->options = this->options | opt;
}

void DLPacket::build() {

	/* build the packet : Bit more significative first */

	this->buffer.clear();

	*this << this->state;
    *this << this->member_id;
    *this << this->options;
    for(int i=0;i < (this->resource.length() > RES_BUFF ? RES_BUFF : this->resource.length()+1);i++) {
        *this << (unsigned char)this->resource[i];
        
    }
}
