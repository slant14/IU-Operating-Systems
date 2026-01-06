#!/bin/bash

inode=$(ls -i ex1 | cut -d' ' -f1)
num_blocks=$(stat -c %b ex1)
block_size=$(stat -c %B ex1)
total_size=$(stat -c %s ex1)
permissions=$(stat -c %A ex1)

echo "Inode: $inode"
echo "Number of Blocks: $num_blocks"
echo "Size of Each Block: $block_size bytes"
echo "Total Size: $total_size bytes"
echo "Permissions: $permissions"

cp ex1 ex2
stat ex2
stat -c "%h - %n" /etc/* | grep ^3

