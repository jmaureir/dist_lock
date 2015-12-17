#!/bin/bash

echo "Testing for 200 clients over valgrind"

echo 0 > shared.txt
./test_base_1 200
