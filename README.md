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
$ dist_lock -c my_resource -- my_command arg1 arg2 arg3 ...

this exceute "my_command arg1 arg2 arg3 ..." when the resource identified by "my_resource" is locked by me.
all other scripts running the same command will hold until they adquire the lock over the same resource.

Features
- bash client and test unit
- tested up-to 1000 instances
- resource name based
- socket busy exceptions (to be tested)
- broadcast over 255.255.255.255 port 5000 fixed at the moment

Missing
- test the socket busy exception
- implement a selft test to realize when a firewall is preventing
  other processes to receive the UDP beacon
- make the algorithm's paramenters variable as arguments in the cmd
- handle exceptions when the executed command fails

