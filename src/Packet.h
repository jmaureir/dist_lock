/**********************************************************
* File Name : Packet.h
*
* Last Modified : Mon 04 Mar 2013 05:30:59 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __PACKET__
#define __PACKET__

#include <sstream>
#include <iostream>
#include <vector>

#include <inttypes.h>

typedef std::vector<unsigned char> ByteArray;

class Packet {
protected:
	ByteArray buffer;
	ByteArray::iterator it;
public:

	Packet();

	Packet(Packet* pkt);

	~Packet();

	virtual std::string toByte();

	virtual void begin();
	virtual size_t length();

	/***** Readers *****/

	Packet& operator >> (unsigned char& data) {
		data = *(this->it);
		this->it++;
		return *this;
	}

	Packet& operator >> (unsigned int& data) {

		unsigned char byte_1 = 0;
		unsigned char byte_2 = 0; 
		unsigned char byte_3 = 0;
		unsigned char byte_4 = 0;

		*this >> byte_1;
		*this >> byte_2;
		*this >> byte_3;
		*this >> byte_4;

		data = (byte_1 << 24) | (byte_2 << 16 ) |  (byte_3 << 8 ) | byte_4; 

		return *this;
	}

	Packet& operator >> (short unsigned int& data) {
		unsigned char byte_1, byte_2;
		*this >> byte_1 >> byte_2;
		data = (byte_1 << 8 ) | byte_2; 

		return *this;
	}

	Packet& operator >> (uint64_t& data) {

		unsigned int int_1 = 0;
		unsigned int int_2 = 0; 

		*this >> int_1;
		*this >> int_2;

		data = ((uint64_t)(int_1) << 32) | int_2;

		return *this;
	}


	/***** Writers ******/


	/* canonical writer: unsigned char 1-byte */
	Packet& operator << (unsigned char data) {
		this->buffer.push_back(data);
		this->it = this->buffer.begin();
		return *this;
	}

	/* int 4-bytes */
	Packet& operator << (int data) {
		return *this << (unsigned int)data;
	}

	Packet& operator << (unsigned int data) {
		*this << (unsigned char)((data>>24)&255);
		*this << (unsigned char)((data>>16)&255);
		*this << (unsigned char)((data>>8)&255);
		*this << (unsigned char)data;
		return *this;
	}

	/* short int 2-bytes */
	Packet& operator << (short int data) {
		*this << (unsigned char)((data>>8)&255);
		*this << (unsigned char)(data&255);
		return *this;
	}

	/* long int 4-bytes */
	Packet& operator << (long int data) {
		*this << (unsigned char)((data>>8)&255);
		*this << (unsigned char)(data&255);
		return *this;
	}

	/* long long int 8-bytes */
	Packet& operator << (long long int data) {
		*this << (unsigned char)((data>>56)&255);
		*this << (unsigned char)((data>>48)&255);
		*this << (unsigned char)((data>>40)&255);
		*this << (unsigned char)((data>>32)&255);
		*this << (unsigned char)((data>>24)&255);
		*this << (unsigned char)((data>>16)&255);
		*this << (unsigned char)((data>>8)&255);
		*this << (unsigned char)data;
		return *this;
	}

	/* unsigned long int 8-bytes */
	Packet& operator << (uint64_t data) {
		*this << (unsigned char)((data>>56)&255);
		*this << (unsigned char)((data>>48)&255);
		*this << (unsigned char)((data>>40)&255);
		*this << (unsigned char)((data>>32)&255);
		*this << (unsigned char)((data>>24)&255);
		*this << (unsigned char)((data>>16)&255);
		*this << (unsigned char)((data>>8)&255);
		*this << (unsigned char)data;
		return *this;
	}

	/* char 1-bytes */
	Packet& operator << (char data) {
		*this << (unsigned char)data;
		return *this;
	}

	/* string 1 byte per character. */
	Packet& operator << (std::string data) {
		const char* d = data.c_str();
		for(unsigned int i=0;i<data.length();i++) {
			*this << (unsigned char)d[i];
		}
		return *this;
	}

	Packet& operator << (Packet& data) {
		if (data.buffer.empty()) {
			this->buffer.swap(data.buffer);
		} else {
			this->buffer.insert(this->buffer.end(),data.buffer.begin(),data.buffer.end());
		}
		this->it = data.it;
		return *this;
	}

	/* to be overwriten in derivate classes */
	virtual void build()=0;

	virtual std::string dump();
};

#endif
