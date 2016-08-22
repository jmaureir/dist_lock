#!/bin/bash

echo "Testing query functionality"

MAX=100
CUR=1
DIST_LOCK=./dist_lock
RES1="du"
RES2=( "test.child1" "test.child2" "test.child3" "test.child4" "test.child5" "test.child6" )
JOB="test/delay_job.sh 10"

idx=0

function adquire() {
    re=$1
    $DIST_LOCK -d -r $RES1:3 -r $re -- $JOB $re 
}

while [ $CUR -le $MAX ]; 
do
    if [ $idx -eq ${#RES2[@]} ];then
        idx=0
        echo "***** loop in the list ******"
    fi
    re=${RES2[$idx]}
    echo "testing for $re"

    $DIST_LOCK -Q $re && adquire $re || echo "$re busy"

    let idx=$idx+1
    let CUR=$CUR+1
done

wait
