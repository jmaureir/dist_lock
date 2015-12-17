/**********************************************************
* File Name : Exception.h
*
* Last Modified : Mon 04 Mar 2013 05:29:19 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __Exception__
#define __Exception__

#include <iostream>
#include <exception>
#include <string>

#define EXCEPTION_HEADER    "Exception Raised:"

class Exception : public std::exception {

	private:
		std::string error;

	public:

		Exception():exception() {
			this->error = "uncaught exception";
		}

		Exception(std::string e):exception() {
			this->error = e;
		}

		~Exception() throw () {

		}

		virtual const char* what() const throw() {
			return(this->error.c_str());
		}

		virtual void what(std::string w) {
			this->error = w;
		}

		std::string toString() {
			return(this->error);
		}

		void printStackTrace() {
			std::cout << EXCEPTION_HEADER << this->error << std::endl;
		}

		friend std::ostream& operator << (std::ostream& os, Exception* e) {
			os << e->toString();
			return os;
		}

		friend std::ostream& operator << (std::ostream& os, Exception& e) {
			os << e.toString();
			return os;
		}

};

#endif
