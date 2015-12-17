/**********************************************************
* File Name : UDPSocket.cc
*
* Last Modified : Thu 17 Dec 2015 03:42:44 PM CLT
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#include "UDPSocket.h"
#include "SocketErrorException.h"

#include <errno.h>

#if defined IP_RECVDSTADDR
# define DSTADDR_SOCKOPT IP_RECVDSTADDR
# define DSTADDR_DATASIZE (CMSG_SPACE(sizeof(struct in_addr)))

# define dstaddr(x) (CMSG_DATA(x))
#elif defined IP_PKTINFO
# define DSTADDR_SOCKOPT IP_PKTINFO
# define DSTADDR_DATASIZE (CMSG_SPACE(sizeof(struct in_pktinfo)))

# define dstaddr(x) (&(((struct in_pktinfo *)(CMSG_DATA(x)))->ipi_addr))
#else
# error "can't determine socket option"
#endif

void _close_socket(int fd) {
	close(fd);
}

union control_data {
    struct cmsghdr  cmsg;
    u_char          data[DSTADDR_DATASIZE];
};

int UDPSocket::create_socket() {
	// create the parent socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sockfd < 0) {
		throw SocketErrorException("could not open the socket");
	}

	// avoid the error: "Address already in use"

	optval = 1;
    if (setsockopt(sockfd, IPPROTO_IP, DSTADDR_SOCKOPT, &optval, sizeof optval) == -1) {
		_close_socket(this->sockfd);
		throw SocketErrorException(strerror(errno));
    }

	/* Enable SO_REUSEADDR to allow multiple instances of this
	* application to receive copies of the multicast datagrams. */

	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		_close_socket(this->sockfd);
		throw SocketErrorException(strerror(errno));
    }

	/*
	int sz = 128 * 50000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0) {
		std::cout << "Unable to set event socket SO_RCVBUFFORCE option: " <<  strerror(errno) << std::endl;
        return;
    }
	*/

	// enable socket to broadcast messages
	int bcast_enable = 1;	
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST , &bcast_enable, sizeof(int)) < 0) {
		_close_socket(this->sockfd);
		throw SocketErrorException(strerror(errno));
    }

	/* Initialize the timeout data structure. */
	struct timeval s_timeout;
	s_timeout.tv_sec = 1;  // default 1 sec
	s_timeout.tv_usec = 0;

	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&s_timeout, sizeof(s_timeout)) < 0) {
		_close_socket(this->sockfd);
		throw SocketErrorException(strerror(errno));
	}

	return sockfd;
}

UDPSocket::UDPSocket(int port) {
	this->port = port;

	try {
		this->sockfd = this->create_socket();
	} catch (Exception& e) {
		throw(e);
	}

	// prepare the server address to bind the socket
	bzero((char *) &(this->serveraddr), sizeof(this->serveraddr));
	this->serveraddr.sin_family = AF_INET;
	this->serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	this->serveraddr.sin_port = htons(this->port);

	// bind the socket
	if (::bind(sockfd,reinterpret_cast<struct sockaddr*>(&(this->serveraddr)),sizeof(this->serveraddr)) < 0) {
		_close_socket(this->sockfd);
		throw SocketErrorException(strerror(errno));
	}
}

UDPSocket::UDPSocket(unsigned int s_addr, int port) {
	this->port = port;

	try {
		this->sockfd = this->create_socket();
	} catch (Exception& e) {
		throw(e);
	}

	// prepare the server address to bind the socket
	bzero((char *) &(this->serveraddr), sizeof(this->serveraddr));
	this->serveraddr.sin_family = AF_INET;
	this->serveraddr.sin_addr.s_addr = s_addr;
	this->serveraddr.sin_port = htons(this->port);

	// bind the socket
	if (::bind(sockfd,reinterpret_cast<struct sockaddr*>(&(this->serveraddr)),sizeof(this->serveraddr)) < 0) {
		throw SocketErrorException("could not bind the socket to any interface");
	}
}

UDPSocket::~UDPSocket() {
	_close_socket(this->sockfd);
}

/* Set socket receive timeout */
void UDPSocket::setTimeout(unsigned int to_s, unsigned long to_ns=0) {
	/* Initialize the timeout data structure. */
	struct timeval s_timeout;
	s_timeout.tv_sec = to_s; 
	s_timeout.tv_usec = to_ns;

	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&s_timeout, sizeof(s_timeout)) < 0) {
		_close_socket(this->sockfd);
		throw SocketErrorException(strerror(errno));
	}
}


/* send an UDP Datagram */
bool UDPSocket::send(UDPDatagram* pkt) {

	pkt->setSourceAddr(this->serveraddr.sin_addr);
	pkt->setSourcePort(this->serveraddr.sin_port);

	// build the packet
	pkt->build();

	// set destination 
	sockaddr_in dstaddr;
	bzero((char *) &dstaddr, sizeof(dstaddr));
	dstaddr.sin_family = AF_INET;
	dstaddr.sin_addr.s_addr = pkt->getDestinationAddr().s_addr;
	dstaddr.sin_port = htons(pkt->getDestinationPort());

	if (sendto(sockfd, pkt->toByte().c_str(), pkt->length(), 0, (struct sockaddr*)&dstaddr, sizeof(dstaddr))>0) {
		return true;
	}

	return false;
}


/* receive an UDP Datagram */
UDPDatagram* UDPSocket::receive() {

	// clean the receiving buffer
	bzero(recv_buf, BUFSIZE);

	// wait for an incomming udp packet
	struct sockaddr_in clientaddr;    /* client addr */
	socklen_t clientlen;              /* byte size of client's address */
	clientlen = sizeof(clientaddr);

	struct in_addr recvaddr;          /* receiver addr */
	recvaddr.s_addr = 0;

	union control_data  cmsg;
	struct cmsghdr     *cmsgptr;

	struct msghdr       msg;
	struct iovec        iov[1];

	iov[0].iov_base = recv_buf;
	iov[0].iov_len  = sizeof(recv_buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name       = &clientaddr;
	msg.msg_namelen    = sizeof(clientaddr);
	msg.msg_iov        = iov;
	msg.msg_iovlen     = 1;
	msg.msg_control    = &cmsg;
	msg.msg_controllen = sizeof cmsg;

	while (true) {
		//ssize_t len = recvfrom(sockfd, recv_buf, BUFSIZE, MSG_WAITALL,(struct sockaddr *)&clientaddr, &clientlen);
		ssize_t len = recvmsg(sockfd, &msg ,0);

		if (len > 0) {
			// obtain the receiver address
			for (cmsgptr = CMSG_FIRSTHDR(&msg);	cmsgptr != NULL; cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {
				if (cmsgptr->cmsg_level == IPPROTO_IP && cmsgptr->cmsg_type == DSTADDR_SOCKOPT) {
					recvaddr.s_addr = ((struct in_addr *)dstaddr(cmsgptr))->s_addr;
					break;
				}
			}
			// build the UDPDatagram abstraction
			UDPDatagram* pkt = new UDPDatagram(clientaddr.sin_addr,recvaddr,clientaddr.sin_port,this->port, recv_buf, len);
			return pkt;	
		} else if (len < 0) {
			// timeout 
			if (errno == EAGAIN || errno == EINTR) {
				return NULL;
			}
		} else {
			// error
			break;
		}
	}
	// error in recvfrom
	throw SocketErrorException("error in recvmsg");
	return NULL;
 }
