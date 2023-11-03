#! /bin/bash

make test${1} > my_res.txt
make rtest${1} > ref_res.txt

colordiff -c my_res.txt ref_res.txt
