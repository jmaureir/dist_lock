/* 
 * File          : main_multi.cc
 *
 * Test for multiple resource acquisition
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 17 Aug 2016 04:24:56 PM CLT
 * Last Modified : Thu 18 Aug 2016 10:47:57 AM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>

#include "DistributedLock.h"

std::atomic<int> r1_count;
std::atomic<int> r2_count;

int max_r1 = 3;

void job_single(int idx) {
    DistributedLock* dl = new DistributedLock(idx);

    dl->defineResource("R1",max_r1);

    if (dl->acquire()) {
        std::cout << idx << " ***** resource R1 adquired ***** " << std::endl;
        r1_count.fetch_add(1);
        std::cout << idx << " R1:" << r1_count << std::endl;

        if (r1_count > max_r1) {
            std::cout << "r1 max count exceeded." << std::endl;
            exit(1);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (dl->releaseAll()) {
            r1_count.fetch_sub(1);
            std::cout << idx << " ***** resource R1 released ***** " << std::endl;
            std::cout << idx << " R1:" << r1_count << std::endl;
        }
    } else {
        std::cout << idx << " resource R1 not adquired" << std::endl;
    }

    delete(dl);
    std::cout << "job multi " << idx << " end" << std::endl;

}

void job_multi(int idx) {
    DistributedLock* dl = new DistributedLock(idx);

    dl->defineResource("R1",max_r1);
    dl->defineResource("R2");

    if (dl->acquire()) {
        std::cout << idx << " ***** resources R1 & R2 adquired ***** " << std::endl;

        r1_count.fetch_add(1);
        r2_count.fetch_add(1);
        std::cout << idx << " R1:" << r1_count << " R2:" << r2_count << std::endl;

        if (r1_count > max_r1) {
            std::cout << "r1 max count exceeded." << std::endl;
            exit(1);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (dl->releaseAll()) {
            std::cout << idx << " ***** resource R1 & R2 released ***** " << std::endl;
            r1_count.fetch_sub(1);
            r2_count.fetch_sub(1);
            std::cout << idx << " R1:" << r1_count << " R2:" << r2_count << std::endl;
        }
    } else {
        std::cout << idx << " resource not adquired" << std::endl;
    }

    delete(dl);
    std::cout << "job single " << idx << " end" << std::endl;

}

int main(int argc, char **argv) {

    std::cout << "Test for Multiple Resource Acquisition" << std::endl;

    r1_count = 0;
    r2_count = 0;

    std::vector<std::thread> jobs;

    int max_jobs = 3; 

    if (argc > 1) {
        max_jobs = atoi(argv[1]);
    }

    for(int i=0;i<max_jobs;i++) {
        jobs.push_back(std::thread(job_single,i+1));
    } 

    for(int i=max_jobs;i<2*max_jobs;i++) {
        jobs.push_back(std::thread(job_multi,i+1));
    } 

    for(int i=0;i<2*max_jobs;i++) {
        jobs[i].join();
        std::cout << "joined job " << i+1 << std::endl;
    }
}

