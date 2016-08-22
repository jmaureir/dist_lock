AUTOMAKE_OPTIONS = subdir-objects
AM_CXXFLAGS= -std=c++11 -O2 -DREENTRANT -fext-numeric-literals
ACLOCAL_AMFLAGS= ${ACLOCAL_FLAGS} ${AM_CXXFLAGS} -I m4 --install
AM_CPPFLAGS= -I$(top_srcdir)/src

lib_LTLIBRARIES             =   libdist_lock.la
libdist_lock_la_SOURCES     = 	\
			src/CommHandler.cc \
			src/Debug.cc \
			src/DistributedLock.cc \
			src/DLPacket.cc \
			src/InetAddress.cc \
			src/Packet.cc \
			src/StringTokenizer.cc \
			src/Thread.cc \
			src/UDPDatagram.cc \
            src/ActionEvent.h \
            src/ActionListener.h \
            src/CommHandler.h \
            src/Debug.h \
            src/DistributedLock.h \
            src/DLPacket.h \
            src/Exception.h \
            src/InetAddress.h \
            src/Observable.h \
            src/Packet.h \
            src/SocketErrorException.h \
            src/StringTokenizer.h \
            src/Thread.h \
            src/UDPDatagram.h \
            src/UDPSocket.h \
			src/UDPSocket.cc 

bin_PROGRAMS        =   dist_lock
dist_lock_SOURCES   =   src/dist_lock.cc 
dist_lock_LDADD     =   libdist_lock.la

TESTS               =   test_base_1 test_base_2 test_base_3
check_PROGRAMS      =   test_base_1 test_base_2 test_base_3

test_base_1_SOURCES =   test/main.cc
test_base_1_LDADD   =   libdist_lock.la

test_base_2_SOURCES =   test/main_multi.cc
test_base_2_LDADD   =   libdist_lock.la

test_base_3_SOURCES =   test/main_count.cc
test_base_3_LDADD   =   libdist_lock.la

TESTS              +=   test/test_10.sh test/test_200.sh test/test_1000.sh 
EXTRA_DIST          =   test/test_10.sh test/test_200.sh test/delay_job.sh test/test_query.sh

AM_LDFLAGS = @LDFLAGS@ $(CLOCK_LIB) $(PTHREAD_LIBS)

dist_noinst_SCRIPTS = autogen.sh
