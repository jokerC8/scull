#!/bin/bash

mkdev() {
	make clean
	make
	devid=`cat /proc/devices | grep $1 | awk '{print $1}'`
	#if scull exit,rmmod it and insmod it again
	[ $devid ] && sudo rmmod $1
	sudo insmod $1.ko
	devid=`cat /proc/devices | grep $1 | awk '{print $1}'`
	[ -e /dev/$1 ] && sudo rm -rf /dev/$1
	sudo mknod /dev/$1 c $devid 0
}
mkdev "scull"
