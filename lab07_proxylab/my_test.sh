#! /bin/bash

cd tiny
./tiny 15213 &
./tiny 15215 &
cd ..
./proxy 15214 &
curl -v --proxy http://localhost:15214 http://localhost:15213/home.html
curl -v --proxy http://localhost:15214 http://localhost:15213/home.html
curl -v --proxy http://localhost:15214 http://localhost:15213/tiny.c
curl -v --proxy http://localhost:15214 http://localhost:15215/tiny.c
pkill tiny
pkill proxy