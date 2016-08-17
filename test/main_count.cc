/* main.cc
* 
 * File          : main.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Tue 16 Aug 2016 11:33:32 AM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

//#include <fstream>
#include <atomic>

#include "CommHandler.h"
#include "DistributedLock.h"
#include "Debug.h"

std::atomic<int> usedRes;
	
void adjustShareFile(int idx, int boundary, int adj) {
//    std::lock_guard<std::mutex> lock(sharedFile_m);
    int busy = usedRes.fetch_add(adj);
    if (adj==0 || (adj>0 && busy <= boundary) || (adj<0 && busy>=boundary)) {
        std::cout << idx << " shared resource available (" << busy << " held)" << std::endl;
    } else {
        std::cout << idx << " ***** resource busy (" << busy << " held)!!! ***** " << std::endl;
        exit(1);
    }
}


void job(int idx, int count=1) {
    DistributedLock* dl = new DistributedLock(idx);
    const string RESOURCENAME("changer") ;
   if (dl->defineResource(RESOURCENAME, count)) {
       std::cout << idx << " created resource " << RESOURCENAME << "(" << count << ")"<< std::endl;
    }else {
       std::cerr << idx << " failed to create resource " << RESOURCENAME << "(" << count << ")"<< std::endl;
       	exit(1);
    	}
    //dl.outEnabled(false);

    if (dl->acquire(RESOURCENAME)) {
        std::cout << idx << " ***** resource acquired ***** " << std::endl;
        adjustShareFile(idx, count, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        std::cout << idx << " releasing shared file" << std::endl;
        adjustShareFile(idx, 0, -1);
        if (dl->release("changer")) {
            std::cout << idx << " ***** resource released ***** " << std::endl;
        }
    } else {
        std::cout << idx << " resource not acquired" << std::endl;
    }

    delete dl;
    std::cout << "job " << idx << " end" << std::endl;

}

int main(int argc, char **argv) {
    usedRes = 0;

    std::vector<std::thread> jobs;

    int max_jobs = 3;
    if (argc > 1) {
        max_jobs = atoi(argv[1]);
    }

    int resourceCount = 1;
    if (argc > 2) {
        resourceCount = atoi(argv[2]);
    }
 
std::cout << "max jobs "  << max_jobs << ";  resource count "  << resourceCount << std::endl;
 

    for(int i=0;i<max_jobs;i++) {
        jobs.push_back(std::thread(job, i+1, resourceCount));
    } 

    for(int i=0;i<max_jobs;i++) {
        jobs[i].join();
        std::cout << "joined job " << i+1 << std::endl;
    }
}

