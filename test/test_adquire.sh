#!/bin/bash

echo "Testing multiple adquire functionality"

MAX=100
CUR=1
DIST_LOCK=./dist_lock
RES1="du"
RES2=( "test.child1" "test.child2" "test.child3" "test.child4" "test.child5" "test.child6" )
JOB="test/delay_job.sh 10"
RETRY=3

idx=0

function try_adquire() {
    re=$1
    $DIST_LOCK -n $RETRY -d -r $RES1:3 -r $re -- $JOB $re 
}

while [ $CUR -le $MAX ]; 
do
    if [ $idx -eq ${#RES2[@]} ];then
        idx=0
        echo "***** loop in the list ******"
    fi
    re=${RES2[$idx]}
    echo "trying to adquire $re"

    try_adquire $re || echo "$re busy"

    let idx=$idx+1
    let CUR=$CUR+1
done

wait
