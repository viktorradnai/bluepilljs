#!/bin/bash

LOG=$1
if [ -z "$LOG" ]; then
    echo Usage: $0 logfile
    exit 1
fi

gnuplot -p -e 'set title "Degrees"; plot "'$LOG'" u 0:2 t "EMS22A" w lines, "'$LOG'" u 0:6 t "LSM303LDHC" w points, "'$LOG'" u 0:10 t "MLX90393" w points' &
gnuplot -p -e 'set title "X Axis"; plot "'$LOG'" u 0:3 t "EMS22A" w lines, "'$LOG'" u 0:7 t "LSM303LDHC" w points, "'$LOG'" u 0:11 t "MLX90393" w points' &
gnuplot -p -e 'set title "Y Axis"; plot "'$LOG'" u 0:4 t "EMS22A" w lines, "'$LOG'" u 0:8 t "LSM303LDHC" w points, "'$LOG'" u 0:12 t "MLX90393" w points' &
