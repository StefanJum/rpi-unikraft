#!/bin/bash

git clone https://github.com/unikraft/app-helloworld && cd app-helloworld

mkdir workdir && git clone -b RELEASE-0.16.3 https://github.com/unikraft/unikraft.git workdir/unikraft

git clone https://github.com/StefanJum/unikraft-plat-rpi workdir/unikraft/plat/raspi

cd workdir/unikraft/plat && echo '$(eval $(call import_lib,$(UK_PLAT_BASE)/raspi))' >> Makefile.uk && cd ../../..

pwd
cp ../helloworld-config def-conf
echo 'CONFIG_UK_APP="'$(pwd)'"' >> .config
echo 'CONFIG_UK_BASE="'$(pwd)/workdir/unikraft'"' >> .config

UK_DEFCONFIG=$(pwd)/def-conf make defconfig
make all

cp ./workdir/build/kernel8.img ../kernel8.img
