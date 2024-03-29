#!/bin/bash

cd avr_src
make arduino.hex
make clean
cd ../client_src
gcc -o client client.c
./client
cd ..
echo "[!] Running Gnuplot"
gnuplot -persist -e 'set yrange [0:5]; plot for[i = 0:7] "./outputs/out_ch".i.".txt" title "ch".i w l;'