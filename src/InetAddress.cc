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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
	inet_aton(this->theIPAddress.c_str(),&(this->in_address));
}

InetAddress::InetAddress(const char* ip_chr) {
	std::string ip = ip_chr;

	if (ip!="") {

		this->theIPAddress  = ip;

		struct hostent       theHostEntity;
		struct hostent*      theResult;

		char                 theBuffer[1024];
		int e = 0;
		int r = 0;

		r = gethostbyname_r(ip.c_str(),&theHostEntity,theBuffer,sizeof(theBuffer),&theResult ,&e);

		if (theResult==NULL) {
			throw(new Exception("Unknown host Address"));
			return;
		}

		unsigned int ip1,ip2,ip3,ip4;

		if ((int)theHostEntity.h_addr_list[0][0]<0) {
			ip1 = 256 + (int)theHostEntity.h_addr_list[0][0];
		} else {
			ip1 = (int)theHostEntity.h_addr_list[0][0];
		};
		if ((int)theHostEntity.h_addr_list[0][1]<0) {
			ip2 = 256 + (int)theHostEntity.h_addr_list[0][1];
		} else {
			ip2 = (int)theHostEntity.h_addr_list[0][1];
		};
		if ((int)theHostEntity.h_addr_list[0][2]<0) {
			ip3 = 256 + (int)theHostEntity.h_addr_list[0][2];
		} else {
			ip3 = (int)theHostEntity.h_addr_list[0][2];
		};
		if ((int)theHostEntity.h_addr_list[0][3]<0) {
			ip4 = 256 + (int)theHostEntity.h_addr_list[0][3];
		} else {
			ip4 = (int)theHostEntity.h_addr_list[0][3];
		};

		this->bytes = new unsigned char[InetAddress::LEN];
		this->bytes[0] = ip1;
		this->bytes[1] = ip2;
		this->bytes[2] = ip3;
		this->bytes[3] = ip4;

		std::stringstream tmp;
		tmp << ip1 << "." << ip2 << "." << ip3 << "." << ip4;
		this->theIPAddress = tmp.str();

		inet_aton(this->theIPAddress.c_str(),&(this->in_address));

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

