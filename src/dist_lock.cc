/* 
 * File          : dist_lock.cc
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Fri 12 Aug 2016 10:12:27 AM GYT
 *
 * (c) 2015-2016 Juan Carlos Maureira
 * (c) 2016      Andrew Hart
 */

#include <iostream>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <map>

#include "CommHandler.h"
#include "DistributedLock.h"
#include "StringTokenizer.h"

int usage() {
    std::cerr << "usage: dist_lock -r {resource_name:[port]} -n {retry_max} -p {port} -- command_to_execute " <<  std::endl;   
    return 1;
}

int main(int argc, char **argv) {

    std::string resource;
    int c;

    int status = 0;
    unsigned int port = 5000;
    unsigned int retry_num = 0;   
    std::map<std::string,unsigned int> res_map;
 
    while ((c = getopt (argc, argv, "r:n:p:")) != -1) {
        switch (c) {
            case 'r':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    resource = std::string(optarg);

                    int count = 1;
                    if (resource.find(':') != std::string::npos) {
                        StringTokenizer st(resource,":");
                        std::string r = st.nextToken();
                        count = st.nextIntToken();
                        if (count<1) {
                            std::cerr << "resource counter must be an integer greater than 0" << std::endl;
                            exit(1);
                        }
                        res_map[resource] = count;
                    }

                } else {
                    return usage();
                }
                break;
            case 'n':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    retry_num = atoi(optarg);
                }
                break;
            case 'p':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    port = atoi(optarg);
                }
                break;

            default:
                return usage();
        }
    }

    DistributedLock* dl = NULL;

    try {
        dl = new DistributedLock(0,port);

        dl->setAdquireMaxRetry(retry_num);

        for(auto it=res_map.begin();it!=res_map.end();it++) {
            std::string resource = (*it).first;
            unsigned int count  = (*it).second;
            if (!dl->defineResource(resource,count)) {
                std::cerr << "error defininf resource " << resource << std::endl;
                return 4;
            }
        }

    } catch(Exception& e) {
        std::cerr << "DistributedLock initialization error" << std::endl;
        return 3;
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
    } else {
        std::cerr << "Resource not adquired." << std::endl;
        return 5;
    }

    if (!dl->releaseAll()) {
        std::cerr << "resource not released" << std::endl;
    }

    delete(dl);

    return status;
}

