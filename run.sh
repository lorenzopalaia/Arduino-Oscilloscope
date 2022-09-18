#!/bin/bash

cd avr_src
make analogRead.hex
make clean
cd ../client_src
gcc -o client client.c
./client
cd ..
gnuplot -persist -e 'plot for[i = 0:7] "./outputs/out_ch".i.".txt" title "ch".i w l'