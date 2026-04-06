#!/bin/bash

if [[ $1 != 100 && $1 != 50 ]]; then
    echo "Command line argument is not equal to 100 or 50."
    echo "Supply 100 to build the test for 100 iterations of 10 MB per test."
    echo "Supply 50 to build the test for 50 iterations of 100 MB per test."
    exit 1
fi

if [[ $# -lt 2 ]]; then
    echo "Second command line argument should be the IP of the server to which the Unikernel should connect. For example 192.168.178.221."
    exit 1
fi

git clone https://github.com/unikraft/app-helloworld && cd app-helloworld

mkdir workdir && git clone -b RELEASE-0.16.3 https://github.com/unikraft/unikraft.git workdir/unikraft

git clone https://github.com/jobpaardekooper/unikraft-rpi.git workdir/unikraft/plat/raspi

cd workdir/unikraft/plat && echo '$(eval $(call import_lib,$(UK_PLAT_BASE)/raspi))' >> Makefile.uk && cd ../../..

cd workdir
git clone https://github.com/unikraft/lib-lwip.git libs/lwip && cd libs/lwip && git checkout e20459c47a6b5ab16967c15b220686c5be50d4d7 && cd ../..
git clone https://github.com/unikraft/lib-musl libs/musl && cd libs/musl && git checkout fd1abc9257b40a74c69b3a40467a453bf8892439 && cd ../..
cd ..

mkdir rootfs
touch rootfs/test.txt

cd rootfs && find -depth -print | tac | bsdcpio -o --format newc > ../initrd.cpio && cd ..

cp ../helloworld-config .config
echo 'CONFIG_UK_APP="'$(pwd)'"' >> .config
echo 'CONFIG_UK_BASE="'$(pwd)/workdir/unikraft'"' >> .config
echo 'CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD_PATH="'$(pwd)/initrd.cpio'"' >> .config

sed -i '4s/.*/LIBS := $(UK_LIBS)\/musl:$(UK_LIBS)\/lwip/' Makefile

cp ../../download-client/main.c ./main.c

if [[ $1 -eq 50 ]]; then
    sed -i '14s/.*/#define TIMES_TO_SEND_BUFFER 100/' main.c
    sed -i '15s/.*/#define REPEAT_TEST 50/' main.c
fi

sed -i "57s/.*/servaddr.sin_addr.s_addr = inet_addr(\"$2\");/" main.c

make all

cp ./workdir/build/kernel8.img ../kernel8.img
