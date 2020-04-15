#! /bin/bash

make clean && make

./trojan-cc1 identity-cc.c -o cc-attacked
./cc-attacked login.c -o login
