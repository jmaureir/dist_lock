#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <signal.h>
#include <string>
#include <sstream>
#include "Exception.h"

#include "InetAddress.h"


const InetAddress InetAddress::BROADCAST_ADDRESS("255.255.255.255");
const InetAddress InetAddress::UNDEFINED_ADDRESS;
const int InetAddress::LEN = 4;

void inetaddressErrorHandler(int signal) {
	printf("InetAddress Error %d\n",signal);
}

InetAddress::InetAddress(unsigned int addr) {

	this->bytes = new unsigned char[InetAddress::LEN];
	this->bytes[3] = addr & 0xFF;
	this->bytes[2] = (addr >> 8) & 0xFF;
	this->bytes[1] = (addr >> 16) & 0xFF;
	this->bytes[0] = (addr >> 24) & 0xFF;

	std::stringstream tmp;
	tmp << (unsigned int)this->bytes[0] << ".";
	tmp << (unsigned int)this->bytes[1] << ".";
	tmp << (unsigned int)this->bytes[2] << ".";
	tmp << (unsigned int)this->bytes[3];

	this->theIPAddress = tmp.str();
	this->in_address.s_addr = addr;

}

InetAddress::InetAddress(const unsigned char ip[4]) {

	this->bytes = new unsigned char[InetAddress::LEN];
	memcpy(this->bytes,ip,InetAddress::LEN);

	std::stringstream tmp;
	tmp << (unsigned int)ip[0] << "." << (unsigned int)ip[1] << "." << (unsigned int)ip[2] << "." << (unsigned int)ip[3];
	this->theIPAddress = tmp.str();
	inet_pton(AF_INET, this->theIPAddress.c_str(), &(this->in_address));
}

InetAddress::InetAddress(const char* ip_chr) {
	std::string ip = ip_chr;

	if (ip!="") {
		this->theIPAddress  = ip;
		struct addrinfo hints;
		struct addrinfo* entry;
// Fill hints struct
		memset(&hints, 0, sizeof (struct addrinfo));
		hints.ai_family = PF_INET;
		hints.ai_socktype = SOCK_DGRAM;
// Look up address info for host (ip)
		int r = getaddrinfo(ip_chr, NULL, &hints, &entry);
		if (r!=0 || entry==0) {
			throw(Exception("Unknown host Address"));
    	return;
		}
		sockaddr_in *addr = (sockaddr_in*)entry->ai_addr;
		this->bytes = new unsigned char[InetAddress::LEN];
		memcpy(this->bytes, &(addr->sin_addr.s_addr), InetAddress::LEN);
		freeaddrinfo(entry); // release addrinfo struct
// Construct standard dotted string address representation
		std::stringstream tmp;
		tmp << (unsigned int)this->bytes[0] << "." << (unsigned int)this->bytes[1]
		    << "." << (unsigned int)this->bytes[2] << "." << (unsigned int)this->bytes[3];
		this->theIPAddress = tmp.str();
// Convert the string address into an in_addr struct in network byte order
		inet_pton(AF_INET, this->theIPAddress.c_str(), &(this->in_address));

	} else {
		this->bytes = new unsigned char[InetAddress::LEN];
		this->bytes[0] = 0;
		this->bytes[1] = 0;
		this->bytes[2] = 0;
		this->bytes[3] = 0;
		this->theIPAddress = "";
	}
}

std::string InetAddress::getHostAddress() {
	if (this->theIPAddress!="") {
		return(this->theIPAddress);
	}
	return("<NotDefined>");
}

std::string InetAddress::getHostName() {
	if (this->theIPName!="") {
		return(this->theIPName);
	}
	return "<NotDefined>";
}

std::string InetAddress::toString() {
	std::stringstream tmp;
	tmp << this->theIPAddress;
	if (this->theIPName!="") {
		tmp << "(" << this->theIPName << ")";
	}
	return(tmp.str());
}

bool InetAddress::operator == (std::string ip) const {
	if (this->theIPAddress.compare(ip)==0) {
		return(true);
	}
	return(false);
}

bool InetAddress::operator == (const InetAddress& ip) const {
	unsigned int me = ((this->bytes[3] << 24) | (this->bytes[2] << 16) | (this->bytes[1] << 8) | (this->bytes[0]));
	unsigned int ot = ((ip.bytes[3] << 24) | (ip.bytes[2] << 16) | (ip.bytes[1] << 8) | (ip.bytes[0]));

	if (me == ot) {
		return true;
	}

	return(false);
}

