#!/bin/bash

make
./CoroutineServer & #���뵽��ִ̨�У�����������Server����

cd example/server

make
./Server & #���뵽��ִ̨��

cd ../client

make
./Client