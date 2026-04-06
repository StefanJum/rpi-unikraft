#!/bin/bash

git clone https://github.com/unikraft/app-nginx && cd app-nginx

mkdir workdir && git clone -b RELEASE-0.16.3 https://github.com/unikraft/unikraft.git workdir/unikraft

git clone https://github.com/jobpaardekooper/unikraft-rpi.git workdir/unikraft/plat/raspi

cd workdir/unikraft/plat && echo '$(eval $(call import_lib,$(UK_PLAT_BASE)/raspi))' >> Makefile.uk && cd ../../..

cd workdir
git clone https://github.com/unikraft/lib-lwip.git libs/lwip && cd libs/lwip && git checkout e20459c47a6b5ab16967c15b220686c5be50d4d7 && cd ../..
git clone https://github.com/unikraft/lib-musl libs/musl && cd libs/musl && git checkout fd1abc9257b40a74c69b3a40467a453bf8892439 && cd ../..
git clone https://github.com/unikraft/lib-nginx libs/nginx && cd libs/nginx && git checkout 36a030031ce705934dd1ed6e69fdcd113a287bfe && cd ../..
cd ..

cp -r ../../test-files/ rootfs/nginx/html/test-files

cd rootfs && find -depth -print | tac | bsdcpio -o --format newc > ../initrd.cpio && cd ..

cp ../nginx-config .config
echo 'CONFIG_UK_APP="'$(pwd)'"' >> .config
echo 'CONFIG_UK_BASE="'$(pwd)/workdir/unikraft'"' >> .config
echo 'CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD_PATH="'$(pwd)/initrd.cpio'"' >> .config

make all

cp ./workdir/build/kernel8.img ../kernel8.img
