#!/bin/bash

touch ex5.txt


chmod a-w ex5.txt
chmod u=rwx,o=rwx ex5.txt
chmod g=u ex5.txt

