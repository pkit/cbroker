#!/bin/bash

NUM="$1"
if [ -z "$NUM" ]; then
	NUM="1"
fi
for i in `seq 1 $NUM`; do
	./client ~/h > /tmp/client-"$i" &
done
wait
for i in `seq 1 $NUM`; do
	tail -n1 /tmp/client-"$i"
done

