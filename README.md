udev-usbdetach
==============

Userland tool to detach linux kernel driver from USB device from an UDEV RUN+ rule.

Imagine this situation :

*  You have 4 FTDI FT232R based USB serial devices connected under GNU/Linux 
   (`/dev/ttyUSB0` to `/dev/ttyUSB3`)
*  You want to access this with an userland FTDI driver, using libUSB, and want to be able 
   to claim the device (with urJTAG for exemple)
*  You don't want to unload kernel module(s) and don't wanna lost all 
   other `/dev/ttyUSBx`

So what ?

You can add an UDEV rule to automatically detach all interfaces when inserted.

Here is a sample UDEV rule :

    SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", GOTO="usb_serial_start"
    GOTO="usb_serial_end"
    LABEL="usb_serial_start"
    ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6011", MODE="0666", RUN+="/usr/local/bin/udev-usbdetach"
    LABEL="usb_serial_end"

You can use -l to specify a log file, and -v to enable all debug:

    SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", GOTO="usb_serial_start"
    GOTO="usb_serial_end"
    LABEL="usb_serial_start"
    ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6011", MODE="0666", RUN+="/usr/local/bin/udev-usbdetach -v -l /var/log/udev-usbdetach.log"
    LABEL="usb_serial_end"

You can specify which interfaces you need to detach :

    udev-usbdetach 0 3 5

Will detach interfaces 0, 3, and 5 only. If nothing is specified, all interfaces will be detached.

You can force attaching instead of detaching interfaces, fo example if all interfaces has be dtached and you wish to re-attach interface 2 you can use the "-a" parameter:

    udev-usbdetach -p /sys/devices/.... -a 2

Requirements :
 - libusb-dev or libusbx-devel
 - libudev-dev or systemd-devel
 - linux, udev & gcc

Roadmap :
 - Add configure script

Warnings :
 - Only tested under Ubuntu 14.04 LTS/Fedora 19 with FTDI Quad RS-232 Device
