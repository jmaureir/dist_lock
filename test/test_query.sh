#!/bin/bash

echo "Testing query functionality"

MAX=100
CUR=1
DIST_LOCK=./dist_lock
RES1="du"
RES2=("test.child1" "test.child2" "test.child3" "test.child4" "test.child5" "test.child6" )
JOB="test/delay_job.sh 10"

idx=0

while [ $CUR -le $MAX ]; 
do
    if [ $idx -eq ${#RES2[@]} ];then
        idx=0
    fi

    re=${RES2[$idx]}

    $DIST_LOCK -q $re
    busy=$?
    if [ "$busy" == "0" ]; then
        echo "$re available. adquiring it"
        $DIST_LOCK -r $RES1:5 -r $re -- $JOB $re &
    else
        echo "$re busy"
    fi
    let idx=$idx+1
    let CUR=$CUR+1
done

wait
