USBdetach
=========

Userland tool to detach linux kernel driver from USB device.

Imagine this situation :

*  You have 4 FTDI FT232R based USB serial devices connected under GNU/Linux 
   (`/dev/ttyUSB0` to `/dev/ttyUSB3`)
*  You want to access one of these, using libUSB, and want to be able 
   to claim the device
*  You don't want to unload kernel module(s) and don't wanna lost all 
   other `/dev/ttyUSBx`

So what ?

You must find which USD device `/dev/ttyUSBx` is, with which VendorID/ProductID,
which interface number and which devine number and use this to detach the
driver from this specific device. Informations can be gathered with udev, 
and driver detached with libUSB.

That's what USBdetach do.

You can use the /dev node (verbose mode) :

     ./usbdetach -v -p /dev/ttyUSB1 
     Using /dev/ttyUSB1 (UID=0 GID=20 perm=21660)  188:1
     Device node: '/dev/ttyUSB1'
     USB device fully identified
     Let's try to find device 0403:6010, interface 1, device number 67
     Complete Matching device found
       0403:6010
       iManufacturer: FTDI
       iProduct: Dual RS232-HS
       iSerialNumber: <none>
       on bus number 2 with address 67
     kernel driver (or something) is active on interface 1... trying to detach
     Kernel driver detached.
     Check with claiming interface 1
     All interface 1 are belong to me
     Release interface 1...
     Closing device...
     Freeing devices list
     LibUSB exit...

Or you can use Vid/Pid informations (verbose mode) :

     ./usbdetach -v -d 0403:6010 -i 1 -f
     Warning ! Device identification incomplete (7):
       Device number is missing (process without with -f)
     but you ask me to proceed anynway (-f). I may detach a wrong device.
     Let's try to find device 0403:6010, interface 1, without device number
     Matching device found (without devnum)
       0403:6010
       iManufacturer: FTDI
       iProduct: Dual RS232-HS
       iSerialNumber: <none>
       on bus number 2 with address 69
     kernel driver (or something) is active on interface 1... trying to detach
     Kernel driver detached.
     Check with claiming interface 1
     All interface 1 are belong to me
     Release interface 1...
     Closing device...
     Freeing devices list
     LibUSB exit...

It's better to use `-p` than just Vid/Pid informations because you can have
multiple USB devices with the same Vid/Pid. Interface number (`-i`) can be
0 by default. If device number or interface is missing, you wil need to 
force command with `-f`.


