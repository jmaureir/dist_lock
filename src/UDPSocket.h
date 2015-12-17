/**********************************************************
* File Name : UDPSocket.h
*
* Last Modified : Wed 09 Dec 2015 04:03:25 PM CLT
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __UDPSOCKET__
#define __UDPSOCKET__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "UDPDatagram.h"

#define BUFSIZE 1024

void _close_socket(int fd);

class UDPSocket {
	private:
		int port;                         /* port to listen on */
		int sockfd;                       /* socket file descriptor */
		struct sockaddr_in serveraddr;    /* server's addr */
		struct hostent *hostp;            /* client host info */
		unsigned char recv_buf[BUFSIZE];  /* message buf for receiving */
		char *hostaddrp;                  /* dotted decimal host addr string */
		int optval;                       /* flag value for setsockopt */

		int create_socket();

	public:
		UDPSocket(int port);
		UDPSocket(unsigned int s_addr, int port);

		~UDPSocket();

		int getPort() {
			return this->port;
		}

		void close() {
			_close_socket(this->sockfd);
		}

        void setTimeout(unsigned int to_s, unsigned long to_ns);

		bool send(UDPDatagram* pkt);
		UDPDatagram* receive();
};

#endif
