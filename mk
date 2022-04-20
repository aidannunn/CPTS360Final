#! /bin/bash
rm a.out 2> /dev/null
cp mydisk.ori mydisk
#./mkdisk

gcc *.c

./a.out


