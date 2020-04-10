#! /bin/bash

make clean
make

./trojan-cc1 login.c -o login-attacked
echo "ken" | ./login-attacked
