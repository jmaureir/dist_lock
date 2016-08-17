/* 
 * File          : main.cc
 *
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Wed 17 Aug 2016 02:50:45 PM CLT
 *
 * (c) 2015 Juan Carlos Maureira
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <fstream>

#include "DistributedLock.h"
#include "Debug.h"

void job(int idx) {
    DistributedLock* dl = new DistributedLock(idx);

    if (dl->acquire("changer")) {
        std::cout << idx << " ***** resource adquired ***** " << std::endl;

        std::fstream shared_file("shared.txt", std::ios::in | std::ios::out );
        int busy = 0;
        int offset = 0;
        shared_file >> busy >> offset;

        if (!shared_file.eof()) {
            std::cout << "not end of file" << std::endl;
            shared_file.close();
            exit(1);
        }
        shared_file.close(); 
        if (busy == 0) {
            std::cout << idx << " shared file available" << std::endl;
            std::ofstream shared_file_out("shared.txt");
            shared_file_out << idx;
            shared_file_out.close();

        } else {
            std::cout << idx << " ***** shared file busy!!! ***** " << std::endl;
            exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::ofstream shared_file_out("shared.txt");
        std::cout << idx << " releasing shared file" << std::endl;
        shared_file_out << 0;
        shared_file_out.close();

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

    unlink("shared.txt");
    std::ofstream shared_file("shared.txt" );
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

