# Raspberry Pi platform for [Unikraft](https://unikraft.org)

## History

This project was forked from the [`plat-raspi` GitHub repo using branch `sikkiladho-devel`](https://github.com/SikkiLadho/plat-raspi/tree/sikkiladho-devel) by [Mushahid Hussain (SikkiLadho)](https://github.com/SikkiLadho). Which was in turn a fork of [`plat-raspi` GitHub repo using branch `spagani-devel`](https://github.com/unikraft/plat-raspi/tree/spagani-devel) by Santiago Pagani. The code seems to be origionally created for the paper ["Towards Highly Specialized, POSIX -compliant Software Stacks with Unikraft: Work-in-Progress"](https://ieeexplore.ieee.org/document/9244044). These origional implementations were for the Raspberry Pi 3B+ but when developing this further for my thesis I put a focus on the Raspberry Pi 3B. The results of my thesis and the resulting measurements can be found in the `measurements` branch of this repository.

The ethernet driver included is taken from the [USPi project](https://github.com/rsta2/uspi/tree/master) and is licensed under `GPL-3.0-only`.

## Raspberry Pi 3B platform

The code has been updated to work with [Unikraft v0.16.3 Telesto](https://github.com/unikraft/unikraft/releases/tag/RELEASE-0.16.3) and boots the [Hello World program](https://github.com/unikraft/app-helloworld) and [Unikraft Nginx](https://github.com/unikraft/app-nginx). 
 
Configuration options have not currently been setup properly. This means that you have so manually select some specific options using `make menuconfig` that could be automatically selected for you. Aditionally, this also results in the fact that the current platform expects the network driver to always be included even if your program does not need it. This should also be updated in to future to be configurable.

### Running programs

To run the Nginx program clone the follwing repository:
```
git clone https://github.com/unikraft/app-nginx && cd app-nginx
```

Create a `workdir` folder and clone Unikraft into it using the following command:
```
mkdir workdir && git clone -b RELEASE-0.16.3 https://github.com/unikraft/unikraft.git workdir/unikraft
```

Clone this repository into **unikraft/plat/** using following command:
```
git clone https://github.com/jobpaardekooper/unikraft-rpi.git workdir/unikraft/plat/raspi
```

Update **unikraft/plat/Makefile.uk** to register the Raspberry Pi platform we just cloned:
```
cd workdir/unikraft/plat && echo '$(eval $(call import_lib,$(UK_PLAT_BASE)/raspi))' >> Makefile.uk && cd ../../..
```

In your `workdir` directory create a `libs` folder and clone the following libraries into it by using:
```bash
cd workdir
git clone https://github.com/unikraft/lib-lwip.git libs/lwip && cd libs/lwip && git checkout e20459c47a6b5ab16967c15b220686c5be50d4d7 && cd ../..
git clone https://github.com/unikraft/lib-musl libs/musl && cd libs/musl && git checkout fd1abc9257b40a74c69b3a40467a453bf8892439 && cd ../..
git clone https://github.com/unikraft/lib-nginx libs/nginx && cd libs/nginx && git checkout 36a030031ce705934dd1ed6e69fdcd113a287bfe && cd ../..
cd ..
```

Nginx needs some files located in the rootfs folder to run. Run the following command to create `initrd.cpio`:
```bash
cd rootfs && find -depth -print | tac | bsdcpio -o --format newc > ../initrd.cpio && cd ..
```

- run `make menuconfig`
- select/deselect the following options in the make menuconfig:
	- Architecture Selection --> Architecture --> select: Armv8 compatible (64 bits)
	- Architecture Selection --> Target Processor --> select: Cortex-A53
	- Architecture Selection --> deselect: Workaround for Cortex-A73 erratum
	- Platform Configuration --> select: Raspberry Pi 3B
	- Library Configuration --> ukboot --> deselect: Show Unikraft banner (this is to minimize boot time)
	- Build Options --> select: Drop unused functions and data
	- Library Configuration --> select: posix-time: Time syscalls
	- Library Configuration --> select: ukring: Ring buffer interface
	- Library Configuration --> select: musl: A C standard library
	- Library Configuration --> select: lwip - Lightweight TCP/IP stack
	- Library Configuration --> lwip - Lightweight TCP/IP stack --> Netif drivers --> deselect: Loop interface
	- Library Configuration --> lwip - Lightweight TCP/IP stack --> Netif drivers --> select: Force polling mode
	- Library Configuration --> lwip - Lightweight TCP/IP stack --> select: Fail boot without netifs
	- Library Configuration --> lwip - Lightweight TCP/IP stack --> select: DHCP client
	- Library Configuration --> lwip - Lightweight TCP/IP stack --> select: Wait for 1st interface
	- Library Configuration --> vfscore: VFS Core Interface --> Compiled-in filesystem table --> Configuration --> Embedded InitRD (CPIO)
	- Library Configuration --> vfscore: VFS Core Interface --> Path to embedded initrd (you sould see the path to the .cpio file we created in the previous step)
	- Library Configuration --> select: libnginx - a HTTP and reverse proxy, a mail proxy, and a generic TCP/UDP proxy server
	- Library Configuration --> select: libnginx - a HTTP and reverse proxy, a mail proxy, and a generic TCP/UDP proxy server --> Provide main 

`posix-time` is required by the USPi code (`nanosleep`).
`ukring` is used for the tx/rx queues in the `raspi-net` networking device.

Now install one of the standard RPi operating systems onto an SD card using (for example) the [Raspberry Pi imager tool](https://www.raspberrypi.com/software/).

Now build the unikernel:
```
make all
```

Once built, replace the `kernel8.img` on the SD card with the newly built Unikraft `kernel8.img`.

Also update the `config.txt` file on the SD card to add the following:
```
enable_uart=1
core_freq_min=500
arm_64bit=1
boot_delay=0
disable_splash=1
```
