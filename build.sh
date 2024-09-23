#!/bin/bash

make
./CoroutineServer & #放入到后台执行，不阻塞后面Server运行

cd example/server

make
./Server & #放入到后台执行

cd ../client

make
./Client