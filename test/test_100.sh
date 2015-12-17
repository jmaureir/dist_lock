#!/bin/bash

echo "Testing for 100 clients"

echo 0 > shared.txt
./test_base_1 100
