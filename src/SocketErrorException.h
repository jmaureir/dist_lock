/**********************************************************
* File Name : SocketErrorException.h
*
* Last Modified : Mon 04 Mar 2013 05:31:22 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __SOCKETERROREXCEPTION__
#define __SOCKETERROREXCEPTION__

#include "Exception.h"

class SocketErrorException : public Exception {
	public:
		SocketErrorException() : Exception("Socket Error") {

		}

		SocketErrorException(std::string what) : Exception("Socket Error:"+what) {

		}
};

#endif
