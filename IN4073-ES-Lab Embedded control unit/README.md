
# Embedded Systems Laboratory software pack - Group 12

This is the Embedded Systems Laboratory repository of Thursday's group 12. 
Tha main code of the drone can be found in "in4073.c" file. The communication protocol is implemented in 
comm.c from the drone side and in protocol.c from the PC side. Also two different PC programs are provided,
there is a basic terminal program in pc_terminal folder and there is an application with simple GUI in pc_gui.
Using the Makefile in in4073 folder can compile both sides, upload the code to the drone and run the terminal 
program by "make upload-run" or the GUI by "make upload-gui-run".

## Philips attempt at building and uploading in docker

```
docker build --platform=linux/amd64 -t in4073 . && docker run --platform=linux/amd64 -it -v /dev/tty.usbserial-DN02EI31:/dev/ttyUSB0 -v `pwd`:/opt/in4073 --rm in4073 /bin/bash
```

## Requirements (Ubuntu 20.04)
* build-essential (gcc and make)
* python2 (for the upload script)
* pip2 (to install pyserial)
* pyserial (`pip2 install pyserial`)
* git (optional)

## Installing pip2 on Ubuntu 20.04
Ubuntu wanted to make it more difficult to install pip2, so it's not downloadable using the package manager.
To install pip2, run:
```
$ wget https://bootstrap.pypa.io/pip/2.7/get-pip.py
$ sudo python2.7 get-pip.py 
```

Next install pyserial by running:
```
$ pip2 install pyserial
```

## Uploading without sudo
To access `/dev/tty` devices on ubuntu you need to be part of the `dialout` group,
you can add yourself to the group by running:
```
$ sudo adduser YOUR_USER_NAME dialout
```
You need to reboot for the change to take effect.

## Building the FCB code and run the PC terminal
Once you have installed the required packages, you can use the Makefile to
compile and upload the code to the FCB. This is done from within the `in4073`
directory.
```
$ cd in4073
$ make upload-run
```

## Building the FCB code and run the GUI
In order to use the GUI you need SDL2 and OpenGL3 on your device. I had OpenGL 
already installed so the installation was not tested, but the following commands 
should work.
```
$ sudo apt-get update
$ sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
```
On Linux SDL2 can be installed as the following.
```
$ sudo apt-get install libsdl2-dev
```
Once you have installed SDL2 and OpenGL3, you can use the Makefile to
compile and upload the code to the FCB. This is done from within the `in4073`
directory.
```
$ cd in4073
$ make upload-gui-run
```