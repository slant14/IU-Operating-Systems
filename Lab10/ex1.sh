#!/bin/bash

path="/tmp/week10"

# Create directory
mkdir $path 2> /dev/null

# Clear content of the directory
rm -rf $path/*
gcc ex1.c -o ex1
gcc monitor.c -o monitor
gnome-terminal --working-directory="$PWD" -- bash -c "./monitor $path; exec bash"
sleep 0.2
./ex1 $path

