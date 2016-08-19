#!/bin/bash

echo "Testing query functionality"

MAX=20
CUR=1
DIST_LOCK=./dist_lock
RES="test"
JOB="test/delay_job.sh 2"

while [ $CUR -le $MAX ]; 
do
    $DIST_LOCK -q $RES
    busy=$?
    if [ "$busy" == "0" ]; then
        echo "$RES available. adquiring it"
        $DIST_LOCK -r $RES -- $JOB &
    else
        echo "$RES busy"
    fi
    let CUR=$CUR+1
done
