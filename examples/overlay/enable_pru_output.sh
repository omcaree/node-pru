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
    dtc -O dtb -o PRU-OUTPUT-00A0.dtbo -b 0 -@ pru_output_overlay.dts
    mv PRU-OUTPUT-00A0.dtbo /lib/firmware/
    echo PRU-OUTPUT:00A0 > /sys/devices/bone_capemgr.*/slots
    cat /sys/devices/bone_capemgr.*/slots
    exit
fi