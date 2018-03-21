#!/bin/bash

LOG=$1
if [ -z "$LOG" ]; then
    echo Usage: $0 logfile
    exit 1
fi

gnuplot -p -e 'set title "Linearity of LSM303C";
    set pointsize 0.1;
    plot "'$LOG'" u 2:6 t "LSM303C" w points' &

gnuplot -p -e 'set title "Angle over time";
    plot "'$LOG'" u 0:2 t "EMS22A" w lines,
        "'$LOG'" u 0:6 t "LSM303C" w lines' &

gnuplot -p -e 'set title "Angle";
    set pointsize 0.5;
    plot "'$LOG'" u 0:2 t "EMS22A" w lines,
        "'$LOG'" u 0:6 t "LSM303C" w points,
        "'$LOG'" u 0:10 t "LSM303LDHC" w points,
        "'$LOG'" u 0:14 t "MLX90393" w points' &
gnuplot -p -e 'set title "X Axis";
    set pointsize 0.5;
    plot "'$LOG'" u 0:3 t "EMS22A" w lines,
        "'$LOG'" u 0:7 t "LSM303C" w points,
        "'$LOG'" u 0:11 t "LSM303LDHC" w points,
        "'$LOG'" u 0:15 t "MLX90393" w points' &
gnuplot -p -e 'set title "Y Axis";
    set pointsize 0.5;
    plot "'$LOG'" u 0:4 t "EMS22A" w lines,
        "'$LOG'" u 0:8 t "LSM303C" w points,
        "'$LOG'" u 0:12 t "LSM303LDHC" w points,
        "'$LOG'" u 0:16 t "MLX90393" w points' &
