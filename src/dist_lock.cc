/* 
 * File          : dist_lock.cc
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Thu 18 Aug 2016 11:31:15 AM CLT
 *
 * (c) 2015-2016 Juan Carlos Maureira
 * (c) 2016      Andrew Hart
 */

#include <unistd.h> 
#include <iostream>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <map>

#include "CommHandler.h"
#include "DistributedLock.h"
#include "StringTokenizer.h"

#define VERSION "1.1.0"

const std::string usage_msg(R"(
usage: %NAME% [-h] [-v] [-b beacon_time] [-B broadcast_network] [-p port]
    [-n max_tries] -r resource1[:count1 [-r resource2[:count2] ...] --
    command_to_execute [args ...]
Use option -h for more detailed help.)");
	

std::string& stringReplace(std::string& s, const std::string& from, const std::string& to)
{
    if (!from.empty())
        for (size_t pos=0; (pos=s.find(from, pos))!=std::string::npos; pos+=to.size())
            s.replace(pos, from.size(), to);
    return s;
}


int usage(std::string& name) {
    std::string msg(usage_msg);
    msg = stringReplace(msg, "%NAME%", name);	
    std::cerr << msg << std::endl;
		return 1;
}

int showVersion(std::string& name) {
    std::cerr << name << " " << VERSION << std::endl;
    return 0;
}

int showHelp(std::string& name) {
    std::cout << name << " (Distributed Lock)" << std::endl;
    std::cout << std::endl;
    std::cout << "This command-line tool provides a light-weight simple-to-use distributed locking" << std::endl;
    std::cout << "mechanism to restrict access by bash scripts  to one or more shared (or scarce)" << std::endl;
    std::cout << "resources. It uses a UDP-based broadcast protocol to enable multiple processes" << std::endl;
    std::cout << "to negotiate, acquire and release shared resources while ensuring that each  may" << std::endl;
    std::cout << "obtain exclusive access to the resources it requires." << std::endl;
    std::cout << std::endl;
    std::cout << name << " can be run from user accounts without the need for installation and it" << std::endl;
    std::cout << "can be used to manage access to shared resources by various processes, even if" << std::endl;
    std::cout << "they have different owners." << std::endl;
    std::cout << std::endl;

    usage(name);

    std::cout << std::endl;
    std::cout << "-h" << std::endl;
    std::cout << "Show this help and exit." << std::endl;
    std::cout << std::endl;
    std::cout << "-v" << std::endl;
    std::cout << "Display version information and exit." << std::endl;
    std::cout << std::endl;
    std::cout << "-r resourcen[:countn]" << std::endl;
    std::cout << "The name of the n-th resource and the number of processes that should be allowed" << std::endl;
    std::cout << "to simultaneously acquire access to it. At least one resource must be specified," << std::endl;
    std::cout << "but more than one may be specified, in which case all such resources must be" << std::endl;
    std::cout << "acquired in order for command_to_execute to be run. The counts (count1, count2," << std::endl;
    std::cout << "...) are optional and only one instance of the resource  is assumed to be" << std::endl;
    std::cout << "available whenever no count is  specified." << std::endl;
    std::cout << std::endl;
    std::cout << "-n max-tries" << std::endl;
    std::cout << "The number of times to try acquiring the specified resource(s) before giving up" << std::endl;
    std::cout << "and failing. The default value is 0, which means that no limit is placed on the" << std::endl;
    std::cout << "number of attempts to acquire the resource(s)." << std::endl;
    std::cout << std::endl;
    std::cout << "-p port" << std::endl;
    std::cout << "The port to be used by the distributed lock protocol. The default" << std::endl;
    std::cout << "port is 5000." << std::endl;
    return 0;
}


int main(int argc, char **argv) {

    std::string resource;
    int c;

    int status = 0;
    unsigned int port = 5000;
    unsigned int retry_num = 0;   
    unsigned int beacon_time = 50; // ms
    std::string  bcast_addr  = "127.255.255.255";

    std::map<std::string,unsigned int> res_map;
 
    // Get executable name
    std::string name(basename(std::string(argv[0]).c_str()));

    // Process command-line arguments
    while ((c = getopt (argc, argv, "hvr:n:p:b:B:")) != -1) {
        switch (c) {
            case 'h':
        		    return showHelp(name);
        		    break;
            case 'v':
        		    return showVersion(name);
        		    break;
            case 'b':
                    beacon_time = atoi(optarg);
        		    break;
            case 'B':
                    bcast_addr = optarg;
        		    break;
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
                    } else {
                        res_map[resource] = 1;
                    }

                } else {
                    return usage(name);
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
                return usage(name);
        }
    }

    DistributedLock* dl = NULL;

    try {
        dl = new DistributedLock(0,port,beacon_time, bcast_addr);

        dl->setAdquireMaxRetry(retry_num);

        if (res_map.size() > 0 ) {

            for(auto it=res_map.begin();it!=res_map.end();it++) {
                std::string resource = (*it).first;
                unsigned int count  = (*it).second;
                if (!dl->defineResource(resource,count)) {
                    std::cerr << "DistributedLock: error registering resource " << resource << "." << std::endl;
                    return 4;
                }
            }
        } else {
            exit(usage(name));
        }
    } catch(Exception& e) {
        std::cerr << "DistributedLock: initialization error." << std::endl;
        std::cerr << e.what() << std::endl;
        return 3;
    }


    // check command to execute
    std::stringstream cmd;

    if (optind > 0) {
        if ((strcmp(argv[optind-1],"--")==0) && (argc - optind > 0)) {
            for (unsigned int i=optind;i < argc;i++) {
                cmd << argv[i] << " ";
            }
 
        } else {
            std::cerr << "No command specified." << std::endl;
            return 2;
        }
    } else {
        return usage(name);
    }
 
    if (dl->acquire(resource)) {
        std::cout << "Resource acquired!" << std::endl;
        std::cout << "Executing: " << cmd.str() << std::endl;
        pid_t pid;

        pid = fork();

        if (pid == 0) {
            int r = execl("/bin/sh", "sh", "-c", cmd.str().c_str(), NULL);
            return r;
        } else  if (pid < 0) {
            std::cerr << "error forking the process." << std::endl;
            return 4;
        } else {
            if (waitpid (pid, &status, 0) != pid) {
                std::cerr << "error waiting for process." << std::endl;
            }
            if ( WIFEXITED(status) ) {
                int exit_code = WEXITSTATUS(status);
                return exit_code;
            }
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "Resource(s) not acquired." << std::endl;
        return 5;
    }

    if (!dl->releaseAll()) {
        std::cerr << "resource(s) not released" << std::endl;
    }

    delete(dl);

    return status;
}

