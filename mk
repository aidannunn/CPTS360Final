#! /bin/bash
rm a.out 2> /dev/null
cp mydisk.ori mydisk
cp disk2.ori disk2
cp disk3.ori disk3
#./mkdisk

gcc *.c

