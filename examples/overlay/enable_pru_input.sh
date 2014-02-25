#!/bin/bash -e

if ! id | grep -q root; then
    echo "must be run as root"
    exit
fi

cd `dirname $0`
if  cat /sys/devices/bone_capemgr.*/slots |grep PRU || false ; then
    cat /sys/devices/bone_capemgr.*/slots
    exit
else
    dtc -O dtb -o PRU-INPUT-00A0.dtbo -b 0 -@ pru_input_overlay.dts
    mv PRU-INPUT-00A0.dtbo /lib/firmware/
    echo PRU-INPUT:00A0 > /sys/devices/bone_capemgr.*/slots
    cat /sys/devices/bone_capemgr.*/slots
    exit
fi