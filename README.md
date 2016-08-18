Distributed Lock
Version 1.1

2015-2016 (c) Juan Carlos Maureira
2015      (c) Andrew Hart

Center for Mathematical Modeling
University of Chile

This software is a distributed locking mechanism to signal the
use of a shared resources accessed from a bash script. It uses a 
UDP broadcast protocol for signaling, adquiring and releasing the shared 
resource in order to ensure an exclusive access to a shared resource. 

Ex: 
$ dist_lock -h -v -r resource1:[count] -r resource2:[count] -n retry_max -p port -b beacon_time -B bcast_addr -- my_command arg1 arg2 arg3 ...

this exceute "my_command arg1 arg2 arg3 ..." when the resource identified by "my_resource" is locked by me.
all other scripts running the same command will hold until they adquire the lock over the same resource.

Features
- bash client and test unit
- tested up-to 1000 instances
- resource name based
- multiple resource support
- socket busy exceptions (to be tested)
- broadcast over 127.255.255.255 port 5000 as default 
- resources handle a count for counting semaphore resource adquisition
- resource adquisition retries infinite times (until succesfully adquisition), or 
  until retry_max times before to exit with exit_code=5 (failing to adquire)

Work-In-Progress
- more unit tests for new functionalities

Missing
- test the socket busy exception
- implement a self test to realize when a firewall is preventing
  other processes to receive the UDP beacon
- handle exceptions when the executed command fails

