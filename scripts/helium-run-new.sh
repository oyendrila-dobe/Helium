#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: run.sh /path/to/folder/containing/benchmarks"
    exit 1;
fi

for bench in $1/*; do
    if [ -d $bench ]; then
        echo "-- Processing " $bench " .."
        simple_name=${bench##*/}
        echo "  -- random testing ..."
        helium $bench --test-generation-method=random --random-test-number=100 > $simple_name.random.output.txt
    fi
done


for bench in $1/*; do
    if [ -d $bench ]; then
        echo "-- Processing " $bench " .."
        simple_name=${bench##*/}
        echo "  -- pairwise testing ..."
        helium $bench --test-generation-method=pairwise --pairwise-test-number=1000 > $simple_name.pairwise.output.txt
    fi
done
