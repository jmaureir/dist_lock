Distributed Lock
Version 1.0

2015 (c) Juan Carlos Maureira
Center for Mathematical Modeling
University of Chile

This software is a distributed locking mechanism to signal the
use of a shared resources accessed from a bash script. It uses a 
UDP broadcast protocol for signaling, adquiring and releasing the shared 
resource in order to ensure an exclusive access to a shared resource. 

Ex: 
$ dist_lock -c my_resource:[count] -n retry_max -p port -- my_command arg1 arg2 arg3 ...

this exceute "my_command arg1 arg2 arg3 ..." when the resource identified by "my_resource" is locked by me.
all other scripts running the same command will hold until they adquire the lock over the same resource.

Features
- bash client and test unit
- tested up-to 1000 instances
- resource name based
- socket busy exceptions (to be tested)
- broadcast over 255.255.255.255 port 5000 as default (-p flag allow to change it)
- resources handle a count for counting semaphore resource adquisition
- resource adquisitio retries infinite times (until succesfully adquisition), or 
  until retry_max times before to exit with exit_code=5 (failing to adquire)

Work-In-Progress
- counting semaphore resource adquisition alg.
- make port variable form cmd line (implement -p flag)
- implement multiple resource adquisition
- more unit tests for new functionalities

Missing
- test the socket busy exception
- implement a self test to realize when a firewall is preventing
  other processes to receive the UDP beacon
- make the algorithm's paramenters variable as arguments in the cmd
- handle exceptions when the executed command fails

