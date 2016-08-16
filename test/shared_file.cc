/* 
 * File          : shared_file.cc
 *
 * Author        : Juan Carlos Maureira
 * Created       : Wed 09 Dec 2015 03:12:59 PM CLT
 * Last Modified : Fri 12 Aug 2016 10:49:07 AM GYT
 *
 * (c) 2015 Juan Carlos Maureira
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <fstream>

int main(int argc, char **argv) {

    unsigned long int delay = 1000;

    if (argc==2) {
        delay = atoi(argv[1]);
    }


    std::fstream shared_file("shared.txt", std::ios::in | std::ios::out );
    int busy = 0;
    shared_file >> busy;
    
    if (busy == 0) {
        std::cout << "shared file available" << std::endl;
        shared_file.seekg (0, std::ios::beg);
        shared_file << "1" << std::endl;
    } else {
        std::cout << " ***** shared file busy!!! ***** " << std::endl;
        shared_file.close();
        exit(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    std::cout << "releasing shared file" << std::endl;
    shared_file.seekg (0, std::ios::beg);
    shared_file << "0" << std::endl;
    shared_file.close();

    return 0;
}

