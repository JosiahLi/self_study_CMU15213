#! /bin/bash

make
make test${1} > liangji.txt
echo -------------------------------------------------------------- >> liangji.txt
make rtest${1} >> liangji.txt