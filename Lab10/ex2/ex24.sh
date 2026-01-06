#!/bin/bash
#!/bin/bash


rm -r ./tmp
ln -s ./tmp tmp1


ls -li 

mkdir ./tmp
ls -li 


bash gen.sh 10 /tmp/ex1.txt
ls -li ./tmp1

ln -s $(pwd)/tmp ./tmp1/tmp2
bash gen.sh 10  ./tmp1/tmp2/ex1.txt

cd tmp1
cd tmp2
cd tmp2
cd tmp2
cd tmp2



cd ..
cd ..
cd ..
cd ..
cd ..
ls -l
rm -r ./tmp
ls -li ./tmp1 

rm ./tmp1


