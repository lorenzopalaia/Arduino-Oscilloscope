#!/bin/bash

cd client_src
./client
cd ..
gnuplot -persist -e 'plot for[i = 0:7] "./outputs/out_ch".i.".txt" w l'