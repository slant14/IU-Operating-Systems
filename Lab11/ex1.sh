#!/bin/bash

fallocate -l 50MiB lofs.img 
sudo losetup -f lofs.img
sudo mkfs.ext4 lofs.img 
mkdir ./lofsdisk
sudo mount lofs.img ./lofsdisk

sudo chmod -R a+rwx lofsdisk
echo "Test" > ./lofsdisk/file1
echo "Anothertest" > ./lofsdisk/file2

lofs_directory="lofsdisk"

get_libs() {
    binary_path="/bin/$1"
    echo $(ldd "$binary_path" | awk '{print $3}')
}

add_to_lofs() {
    command_name="$1"
    libs=$(get_libs $command_name)
    mkdir $lofs_directory/lib 2> /dev/null
    mkdir $lofs_directory/bin 2> /dev/null
    cp $libs "$lofs_directory/lib"
    cp "/bin/$command_name" "$lofs_directory"/bin
}

add_to_lofs "bash"
add_to_lofs "cat"
add_to_lofs "echo"
add_to_lofs "ls"

gcc --static ex1.c -o ex1
cp ex1 $lofs_directory/ex1
echo "In chrooted directory:" >> ex1.txt
sudo chroot lofsdisk ./ex1  >> ex1.txt
echo -e "\nIn default directory:" >> ex1.txt
./ex1 >> ex1.txt
 
