#ifndef __InetAddress__
#define __InetAddress__

#include <string>
#include <string.h>
#include <sstream>

#include <arpa/inet.h>

#include "Exception.h"

class InetAddress {

	private:

		unsigned char*       bytes;
		in_addr              in_address;

		std::string          theIPName;
		std::string          theIPAddress;

	public:

		static const int LEN;
	    static const InetAddress BROADCAST_ADDRESS;
	    static const InetAddress UNDEFINED_ADDRESS;

		InetAddress() {
			this->bytes = new unsigned char[4];
			this->bytes[0] = 0;
			this->bytes[1] = 0;
			this->bytes[2] = 0;
			this->bytes[3] = 0;
			this->theIPAddress = "";
		}

		InetAddress(const InetAddress& ip) {
			this->bytes = new unsigned char[4];
			this->bytes[0] = ip.bytes[0];
			this->bytes[1] = ip.bytes[1];
			this->bytes[2] = ip.bytes[2];
			this->bytes[3] = ip.bytes[3];

			this->theIPAddress = ip.theIPAddress;
		}

		InetAddress(unsigned int addr);
		InetAddress(const char* ip_chr);
		InetAddress(const unsigned char[4]);

		~InetAddress() {
			delete[](this->bytes);
		}
		std::string getHostAddress();
		std::string getHostName();
		std::string toString();

		const unsigned char* getBytes() {
			return this->bytes;
		}

		unsigned int getAsUnsignedInteger() {
			return ((this->bytes[3] << 24) | (this->bytes[2] << 16) | (this->bytes[1] << 8) | (this->bytes[0])); 
		}	

		in_addr& getInAddress() {
			this->in_address.s_addr = this->getAsUnsignedInteger();
			return this->in_address;
		}

		InetAddress& operator = (const InetAddress& ip) {
			memcpy(this->bytes,ip.bytes,InetAddress::LEN);
			this->theIPAddress = ip.theIPAddress;
			this->theIPName = ip.theIPName;
			return *this;
		}
		bool operator == (std::string ip) const;
		bool operator == (const InetAddress& ip) const;

		friend std::ostream& operator << (std::ostream& os, InetAddress* addr) {
			os << addr->toString();
			return os;
		}

		friend std::ostream& operator << (std::ostream& os, InetAddress& addr) {
			os << addr.toString();
			return os;
		}

};

#endif
