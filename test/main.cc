/* 
 * File          : main.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Wed 17 Aug 2016 10:39:21 AM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <fstream>

#include "CommHandler.h"
#include "DistributedLock.h"
#include "Debug.h"

void job(int idx) {
    DistributedLock* dl = new DistributedLock(idx);

    if (dl->acquire("changer")) {
        std::cout << idx << " ***** resource adquired ***** " << std::endl;

        std::fstream shared_file("shared.txt", std::ios::in | std::ios::out );
        int busy = 0;
        shared_file >> busy;
        
        if (busy == 0) {
            std::cout << idx << " shared file available" << std::endl;
            shared_file.seekg (0, std::ios::beg);
            shared_file << idx << std::endl;
        } else {
            std::cout << idx << " ***** shared file busy!!! ***** " << std::endl;
            shared_file.close();
            exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << idx << " releasing shared file" << std::endl;
        shared_file.seekg (0, std::ios::beg);
        shared_file << "0" << std::endl;
        shared_file.close();

        if (dl->release("changer")) {
            std::cout << idx << " ***** resource released ***** " << std::endl;
        }
    } else {
        std::cout << idx << " resource not adquired" << std::endl;
    }

    delete(dl);
    std::cout << "job " << idx << " end" << std::endl;

}

int main(int argc, char **argv) {

    std::fstream shared_file("shared.txt", std::ios::in | std::ios::out );
    int busy = 0;
    shared_file << busy;
    shared_file.close();

    std::vector<std::thread> jobs;

    int max_jobs = 3; 
    if (argc > 1) {
        max_jobs = atoi(argv[1]);
    }
  

    for(int i=0;i<max_jobs;i++) {
        jobs.push_back(std::thread(job,i+1));
    } 

    for(int i=0;i<max_jobs;i++) {
        std::cout << "joining job " << i+1 << std::endl;
        jobs[i].join();
    }
}

