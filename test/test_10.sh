#!/bin/bash

echo "Testing for 10 clients"

echo 0 > shared.txt
./test_base_1 10
