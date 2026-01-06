#!/bin/bash

bash gen.sh 10 ex1.txt
ln ex1.txt ex11.txt
ln ex1.txt ex12.txt
cat ex1.txt
echo ------
cat ex11.txt
echo -----
cat ex12.txt


echo > output.txt
ls -i  ex1.txt >> output.txt
ls -i ex11.txt >> output.txt
ls -i ex12.txt >> output.txt

du ex1.txt


ln ex1.txt ex13.txt
mv ex13.txt /tmp

find . -inum $(ls -i ex1.txt | awk '{print $1}')
sudo find / -inum $(ls -i ex1.txt | awk '{print $1}') 2> /dev/null


stat ex1.txt


sudo find / -inum $(ls -i ex1.txt | awk '{print $1}') -exec rm {} \;
