/**********************************************************
* File Name : UDPDatagram.cc
*
* Last Modified : Mon 04 Mar 2013 05:32:10 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#include "UDPDatagram.h"

UDPDatagram::UDPDatagram(UDPDatagram* dgram) : Packet(dgram) {

    this->setSourceAddr(dgram->getSourceAddr());
    this->setDestinationAddr(dgram->getDestinationAddr());
    this->setSourcePort(dgram->getSourcePort());
    this->setDestinationPort(dgram->getDestinationPort());

}

UDPDatagram::~UDPDatagram() {

}

