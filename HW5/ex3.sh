#!/bin/bash

gcc -o ex3 ex3.c -pthread

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

n=10000000

for m in 1 2 4 10 100
do
    echo "m=$m:" >> ex3_res.txt
    (time ./ex3 $n $m > /dev/null) 2>> ex3_res.txt
    echo "" >> ex3_res.txt
done

echo "done"