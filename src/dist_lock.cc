/* 
 * File          : dist_lock.cc
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Thu 11 Aug 2016 10:25:15 PM GYT
 *
 * (c) 2015-2016 Juan Carlos Maureira
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

   DistributedLock* dl = NULL;

    try {
        dl = new DistributedLock();
    } catch(Exception& e) {
        std::cerr << "DistributedLock initialization error" << std::endl;
        return 3;
    }

    int status = 0;

    while ((c = getopt (argc, argv, "r:")) != -1) {
        switch (c) {
            case 'r':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    resource = std::string(optarg);

                    if (!dl->defineResource(resource)) {
                        std::cerr << "error defininf resource " << resource << std::endl;
                        return 4;
                    }

                } else {
                    return usage();
                }
                break;

            default:
                return usage();
        }
    }

    exit(1);

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

 
    if (dl->adquire(resource)) {
        std::cout << "resource adquired!" << std::endl;
        std::cout << "executing: " << cmd.str() << std::endl;
        pid_t pid;

        pid = fork();

        if (pid == 0) {
            int r = execl("/bin/sh", "sh", "-c", cmd.str().c_str(), NULL);
            return r;
        } else  if (pid < 0) {
            std::cerr << "error forking the process" << std::endl;
            return 4;
        } else {
            if (waitpid (pid, &status, 0) != pid) {
                std::cerr << "error waiting for process" << std::endl;
            }
            if ( WIFEXITED(status) ) {
                int exit_code = WEXITSTATUS(status);
                return exit_code;
            }
            return EXIT_FAILURE;
        }
    }

    if (dl->release()) {
        std::cout << "resource released" << std::endl;
    }

    delete(dl);

    return status;
}

