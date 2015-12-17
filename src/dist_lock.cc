/* 
 * File          : dist_lock.cc
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Thu 17 Dec 2015 03:43:20 PM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */

#include <iostream>
#include <string>
#include <sstream>
#include <sys/wait.h>

#include "CommHandler.h"
#include "DistributedLock.h"

int usage() {
    std::cerr << "usage: dist_lock -r {resource_name} -- command_to_execute " <<  std::endl;   
    return 1;
}

int main(int argc, char **argv) {

    std::string resource;
    int c;

    while ((c = getopt (argc, argv, "r:")) != -1) {
        switch (c) {
            case 'r':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    resource = std::string(optarg);
                } else {
                    return usage();
                }
                break;

            default:
                return usage();
        }
    }

    // check resource name is provided
    if (resource == "") {
        return usage();
    }

    // check command to execute
    std::stringstream cmd;

    if (optind > 0) {
        if ((strcmp(argv[optind-1],"--")==0) && (argc - optind > 0)) {
            for (unsigned int i=optind;i < argc;i++) {
                cmd << argv[i] << " ";
            }
 
        } else {
            std::cerr << "no command given" << std::endl;
            return 2;
        }
    } else {
        return usage();
    }

    DistributedLock* dl = NULL;

    try {
        dl = new DistributedLock();
    } catch(Exception& e) {
        std::cerr << "DistributedLock error" << std::endl;
        return 3;
    }

    if (dl->adquire(resource)) {
        std::cout << "resource adquired!" << std::endl;
        std::cout << "executing: " << cmd.str() << std::endl;
        pid_t pid;
        int status = 0;

        pid = fork();

        if (pid == 0) {
            execl("/bin/sh", "sh", "-c", cmd.str().c_str(), NULL);
        } else  if (pid < 0) {
            std::cerr << "error forking the process" << std::endl;
            return 4;
        } else {
            if (waitpid (pid, &status, 0) != pid) {
                std::cerr << "error waiting for process" << std::endl;
            }
        }
    }

    if (dl->release()) {
        std::cout << "resource released" << std::endl;
    }

    delete(dl);

    return 0;
}

