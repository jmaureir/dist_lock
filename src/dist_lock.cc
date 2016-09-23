/* 
 * File          : dist_lock.cc
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Fri 23 Sep 2016 12:31:24 PM CLT
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
#include <condition_variable>
#include <mutex>
#include <regex>

#include "CommHandler.h"
#include "DistributedLock.h"
#include "StringTokenizer.h"

// Version number and default parameter values
#define VERSION           "1.1.0"
#define BROADCASTADDRESS "127.255.255.255"
#define PORT              5000
#define BEACONTIME        50
#define RETRYNUM          0
#define RESOURCECOUNT     1
#define PREFIX_ENV        "DISTLOCK"

typedef std::map<std::string, std::string> EnvVarList;

std::condition_variable parent_wait;
std::mutex              parent_m;

pid_t                   pid_d;

EnvVarList              env_list;
char**                  env_array  = NULL;

bool detach_i = false;  // detach on initialization
bool acquired = false;

const std::string usage_msg(R"(
usage: %NAME% [-h] [-v] [-d] [-b beacon_time] [-B broadcast_network] [-p port]
    [-n max_tries] {-q resource | -Q resource -W resource | -r resource1[:count1 [-r resource2[:count2] ...]} --
    command_to_execute [args ...]
)");

const string help_msg(R"(
%NAME% (Distributed Lock)

SYNOPSIS

This command-line tool provides a light-weight simple-to-use distributed locking
mechanism to restrict access by bash scripts  to one or more shared (or scarce) 
resources. It uses a UDP-based broadcast protocol to enable multiple processes 
to negotiate, acquire and release shared resources while ensuring that each  may
obtain exclusive access to the resources it requires.

%NAME% can be run from user accounts without the need for installation and
it can be used to manage access to shared resources by various processes, even
they have different owners.

%USAGE%

OPTIONS

-h
Show this help and exit.

-v
Display version information and exit.

-q resource
Query the resource for busy state. In other words, it returns 1 when the resource
is acquired. Otherwise, it returns 0 in the exit code.

-Q resource
Query the resource for any state. In other words, it returns 1 when there is other
processes contending for the resource, no mater the state of each one of them.

-W resource
Query the resource for acquiring or starting state. Returns 1 when there is at least
one process trying to acquire the resource. 

-r resource[:countn]
The name of the n-th resource and the number of processes that should be allowed 
to simultaneously acquire access to it. At least one resource must be specified, 
but more than one may be specified, in which case all such resources must be 
acquired in order for command_to_execute to be run. The counts (count1, count2, 
...) are optional and only one instance of the resource  is assumed to be 
available whenever no count is  specified.

-n max-tries
The number of times to try acquiring the specified resource(s) before giving up 
and failing. The default value is %RETRYNUM%, which means that no limit is placed on the 
number of attempts to acquire the resource(s).

-p port
The port to be used by the distributed lock protocol. The default 
port is %PORT%.

-b beacon_time
Interval in milliseconds between beacon packets.  The default is %BEACONTIME%.

-B broadcast_network
Address of network to use for communication.  The default is %BROADCASTADDRESS%.
)");
std::string& stringReplace(std::string& s, const std::string& from, const std::string& to)
{
    if (!from.empty())
        for (size_t pos=0; (pos=s.find(from, pos))!=std::string::npos; pos+=to.size())
            s.replace(pos, from.size(), to);
    return s;
}

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        acquired = true;
        parent_wait.notify_all();
    }
    if (signal == SIGUSR2) {
        acquired = false;
        parent_wait.notify_all();
    }

    if (signal == SIGINT) {
        kill(-pid_d,SIGKILL);
    }
}

int usage(std::string& name) {
    std::string msg(usage_msg);
    msg = stringReplace(msg, "%NAME%", name);	
    msg = msg+"\nUse option -h for more detailed help.\n";
    std::cerr << msg << std::endl;
		return 1;
}

int showVersion(std::string& name) {
    std::cout << name << " " << VERSION << std::endl;
    return 0;
}

int showHelp(std::string& name) {
    std::string msg(help_msg);
    msg = stringReplace(msg, "%USAGE%", usage_msg);	
    msg = stringReplace(msg, "%NAME%", name);	
    msg = stringReplace(msg, "%BEACONTIME%", std::to_string(BEACONTIME));	
    msg = stringReplace(msg, "%PORT%", std::to_string(PORT));	
    msg = stringReplace(msg, "%BROADCASTADDRESS%", BROADCASTADDRESS);	
    msg = stringReplace(msg, "%RETRYNUM%", std::to_string(RETRYNUM));	
    std::cout << msg << std::endl;
    return 0;
}

void updateEnvironmentArray() {
    // updaye the environmental variables to expose
    // to the calling script
    if (env_array != NULL) {
        for(int i=0;i<env_list.size()-1;i++) {
            delete(env_array[i]);
        }
        delete[](env_array);
    }   
    env_array = new char*[env_list.size()+1];
    int count = 0;
    for(auto it=env_list.begin();it!=env_list.end();it++) {
        std::string key = (*it).first;
        std::string val = (*it).second;
        std::string env = key+"="+val;
        char* env_ch = new char[env.size()+1];
        strcpy(env_ch,env.c_str());
        env_ch[env.size()] = '\0';
        env_array[count] = env_ch;
        count++;
    }   
    env_array[count] = NULL;
}

void registerEnv(std::string name, std::string val) {
    env_list[name] = val;
    updateEnvironmentArray();
}


int main(int argc, char **argv) {

    std::string resource;
    int c;

    bool         is_any      = false;
    bool         query       = false;
    bool         is_waiting  = false;
    unsigned int port        = PORT;
    unsigned int retry_num   = RETRYNUM;   
    unsigned int beacon_time = BEACONTIME; // ms
    std::string  bcast_addr  = BROADCASTADDRESS;
    std::string  prefix_env  = PREFIX_ENV;

    if (getenv("DISTLOCK_PREFIX")) {
        prefix_env = getenv("DISTLOCK_PREFIX");
    }

    std::map<std::string,unsigned int> res_map;
 
    // Get executable name
    std::string name(basename(std::string(argv[0]).c_str()));

    // Process command-line arguments
    while ((c = getopt (argc, argv, "hvdr:n:p:b:B:q:Q:W:e:")) != -1) {
        switch (c) {
            case 'd':
                    detach_i = true;
        		    break;
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
            case 'e':
                    prefix_env = optarg;
                    stringReplace( prefix_env, " ","_");
        		    break;
            case 'q':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    resource = std::string(optarg);
                    is_any = false;
                    query = true;
                    is_waiting = false;
                    res_map[resource] = 1;
                }
                break;
            case 'Q':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    resource = std::string(optarg);
                    query = false;
                    is_any = true;
                    is_waiting = false;
                    res_map[resource] = 1;
                }
                break;
            case 'W':
                if (optarg!=NULL && strlen(optarg) > 0 ) {
                    resource = std::string(optarg);
                    query = false;
                    is_any = false;
                    is_waiting = true;
                    res_map[resource] = 1;
                }
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
                        res_map[r] = count;
                    } else {
                        res_map[resource] = RESOURCECOUNT;
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

    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGINT, signal_handler);

    int status = 0;

    pid_d = fork();

    if (pid_d==0) {

        if (detach_i) {    
            setsid();
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

        if (query && !is_any && !is_waiting) { 
            if (dl->isBusy(resource)) {
                status = 1;
            } else {
                status = 0;
            }
        } else if (!query && is_any && !is_waiting) {
            if (dl->isAny(resource)) {
                status = 1;
            } else {
                status = 0;
            }
        } else if (!query && !is_any && is_waiting) {
            if (dl->isWaiting(resource)) {
                status = 1;
            } else {
                status = 0;
            }
        } else if (!query && !is_any && !is_waiting) {
            // check command to execute
            char** cmd_argv = NULL;
            std::stringstream cmd;

            if (optind > 0) {                
                if ((strcmp(argv[optind-1],"--")==0) && (argc - optind > 0)) {

                    int num_args = argc - optind;
                    cmd_argv = new char*[num_args+1];
                    int j = 0;
                    for (unsigned int i=optind;i < argc;i++) {
                        cmd_argv[j] = argv[i];
                        cmd << argv[i] << " ";
                        j++;
                    }
                    cmd_argv[j] = NULL;        
 
                } else {
                    std::cerr << "No command specified." << std::endl;
                    return 2;
                }
            } else {
                return usage(name);
            }

            if (detach_i && retry_num == 0) {
                dl->onStart([&] {
                    pid_t p = getppid();
                    kill(p,SIGUSR1); 
                });
            }
         
            if (dl->acquire()) {
                std::cout << "Resource acquired!" << std::endl;
                std::cout << "Executing: " << cmd.str() << std::endl;
                pid_t pid;

                if (detach_i && retry_num > 0) {
                    pid_t p = getppid();
                    kill(p,SIGUSR1); 
                }

                pid = fork();

                if (pid == 0) {

                    registerEnv(prefix_env+"_ID",std::to_string(dl->getId()));
                    std::string str_res_list;
                    for(auto it=res_map.begin();it!=res_map.end();it++) {
                        std::string resource = (*it).first;
                        int count  = (*it).second;
                        std::string res = resource + ":" + std::to_string(count);
                        str_res_list = str_res_list + res + ",";
                    }
                    if (str_res_list.length() > 0 ) {
                        str_res_list = str_res_list.substr(0, str_res_list.length()-1);                   
                    }
                    registerEnv(prefix_env+"_RESOURCES",str_res_list);

                    int r = execvpe(cmd_argv[0],cmd_argv,env_array);
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
                if (detach_i) {
                    pid_t p = getppid();
                    kill(p,SIGUSR2); 
                }
                return 5;
            }

            if (!dl->releaseAll()) {
                std::cerr << "resource(s) not released" << std::endl;
            }
        }
        if (detach_i) {
             pid_t p = getppid();
             kill(p,SIGUSR2); 
        }

        delete(dl);

        return status;
    } else {

        if (detach_i) {

            // wait the children to notify us to exit 
            std::unique_lock<std::mutex> lk(parent_m);
            parent_wait.wait(lk);

            if (acquired) {
                // "detaching process on initialization" 
                return 0;
            }
        } 

        if (waitpid (pid_d, &status, 0) == pid_d) {

            if (WIFEXITED(status)) {
                status = WEXITSTATUS(status);
            } else {
                status=EXIT_FAILURE;
            }
        } else {
            std::cerr << "error waiting for process." << std::endl;
            status=EXIT_FAILURE;
        }
    }
    return status;
}
