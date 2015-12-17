#!/bin/bash

echo "Testing for 1000 clients"

echo 0 > shared.txt
./test_base_1 1000
