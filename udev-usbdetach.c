/*
   Copyright (c) 2012, Denis Bodor
   Copyright (c) 2015, Neil Armstrong
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met: 

   1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <regex.h>
#include <sys/stat.h>
#include <libudev.h>
#include <libusb.h>

#define OPT "shVvp:l:"
#define DAREGEX "^[0-9a-fA-F]{4}:[0-9a-fA-F]{4}$"

int verbose;

unsigned int udevstufftoint(const char *udevstring, int base) {
  char *endp;
  int ret;
  errno = 0;

  if(udevstring == NULL)
    return(-1);

  ret = (unsigned int)strtol(udevstring, &endp, base);
  if(errno) {
    fprintf(stderr,"udevstufftoint: Unable to parse number Error : %s (%d)\n",
        strerror(errno),errno);
    return(-2);
  }
  if (endp == optarg) {
    fprintf(stderr, "udevstufftoint: No digits were found\n");
    return(-3);
  }
  return(ret);
}

void giveup(libusb_device_handle *devh, libusb_context *ctx) {
    libusb_close(devh);
    libusb_exit(ctx);
    fprintf(stderr,"Fatal error. Giving up ! Sorry.\n");
    exit(EXIT_FAILURE);
}

void badarg() {
    fprintf(stderr,"Bad syntaxe for USB ID. Please use -d idVendor:idProduct\n");
    exit(EXIT_FAILURE);
}

int printdesc (struct libusb_device_handle *handle, uint8_t idx) {
    unsigned char buffer[256];
    int ret = 0;

    // desc asked exist ?
    if(!idx) {
        printf("<none>\n");
        return(1);
    }

    if ((ret = libusb_get_string_descriptor_ascii(handle, idx, buffer, sizeof(buffer))) <= 0)
        return(1);

    if(ret > 1)
        printf("'%s'\n", buffer);

    return(0);
}

void printinfo() {
    printf("dev-usbdetach v0.1\nCopyright (c) 2015 Neil Armstrong <superna9999@gmail.com>\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n");
    printf("Syntaxe :\n");
    printf(" -h -V                     you read this right now\n");
    printf(" -v                        be verbose\n");
    printf(" -p syspath                use sys path instead of udev environment\n");
    printf(" -l logfile                output debug, verbose and error to log file\n");
    printf(" -s                        simulate (don't detach and claim)\n");
}

int main(int argc, char **argv) {
    int optch;

    char *syspath = NULL;

    int donothing = 0;

    struct udev *udev_ctx;
    struct udev_device *udev_dev = NULL;

    libusb_context *ctx = NULL;

    struct libusb_device_handle *devhaccess;

    struct libusb_device **list;
    struct libusb_device *found = NULL;
    struct libusb_device_descriptor fdesc;
   
    struct st_device {
        int vid;
        int pid;
        int busnum;
        int devnum;
    } targetdevice = { 0, 0 };

    ssize_t cnt;
    ssize_t i = 0;

    while ((optch = getopt(argc, argv, OPT)) != -1) {
        switch (optch) {
            case 's':
                donothing=1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'p':
                syspath=optarg;
                break;
            case 'l':
                if(!freopen(optarg, "a+", stdout)) {
                    fprintf(stderr, "Failed to open stdout log file %m\n");
                    exit(EXIT_FAILURE);
                }
                if(!freopen(optarg, "a+", stderr)) {
                    fprintf(stderr, "Failed to open stderr log file %m\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
            case 'V':
                printinfo();
                exit(EXIT_SUCCESS);
                break;
            default:
                fprintf(stderr,"Unknow option\n");
                exit(EXIT_FAILURE);
        }
    }

    udev_ctx = udev_new();
    if (!udev_ctx)
    {
        fprintf(stderr,"Can't init libudev\n");
        exit(EXIT_FAILURE);
    }

    if (syspath)
        udev_dev = udev_device_new_from_syspath(udev_ctx, syspath);
    if (syspath && !udev_dev)
        fprintf(stderr,"Failed to get device from syspath '%s'\n", syspath);
    if (!udev_dev)
    {
        udev_dev = udev_device_new_from_environment(udev_ctx);
        if (!udev_dev)
            fprintf(stderr,"Can't find device from environment variables\n");
    }
    if (!udev_dev)
    {
        fprintf(stderr,"Failed to get udev device\n");
        udev_unref(udev_ctx);
        exit(EXIT_FAILURE);
    }

    if(verbose) {
        printf("udev Device node: '%s' subsystem '%s' devtype '%s'\n", udev_device_get_devnode(udev_dev), udev_device_get_subsystem(udev_dev), udev_device_get_devtype(udev_dev));
    }

    if (!udev_device_get_subsystem(udev_dev) ||
        !udev_device_get_devtype(udev_dev) ||
        strcmp(udev_device_get_subsystem(udev_dev), "usb") != 0 ||
        strcmp(udev_device_get_devtype(udev_dev), "usb_device") != 0)
    {
        fprintf(stderr,"Invalid usb subsystem and device type\n");
        udev_unref(udev_ctx);
        exit(EXIT_FAILURE);
    }

    // Get USB Bus num/dev
    targetdevice.busnum  = udevstufftoint(udev_device_get_sysattr_value(udev_dev, "busnum"), 10);
    targetdevice.devnum  = udevstufftoint(udev_device_get_sysattr_value(udev_dev, "devnum"), 10);
    targetdevice.vid  = udevstufftoint(udev_device_get_sysattr_value(udev_dev, "idVendor"), 16);
    targetdevice.pid  = udevstufftoint(udev_device_get_sysattr_value(udev_dev, "idProduct"), 16);

    // Closing udev
    udev_dev = NULL;
    udev_unref(udev_ctx);

    if (targetdevice.busnum < 0 ||
        targetdevice.devnum < 0 ||
        targetdevice.vid < 0 ||
        targetdevice.pid < 0)
    {
        fprintf(stderr,"Invalid device parameters VID=%d PID=%d Bus %d Dev %d\n",
                targetdevice.busnum, targetdevice.devnum,
                targetdevice.vid, targetdevice.pid);
        udev_unref(udev_ctx);
        exit(EXIT_FAILURE);
    }

    if(verbose) {
        printf("Let's try to find device %04x:%04x Bus %03d Device %03d\n", targetdevice.vid, targetdevice.pid, targetdevice.busnum, targetdevice.devnum);
    }

    /* init */
    if(libusb_init(&ctx) != 0) {
        fprintf(stderr,"Can't init libusb\n");
        fprintf(stderr,"libusb_init Error : %s (%d)\n",
                strerror(errno),errno);
        exit(EXIT_FAILURE);
    }

    libusb_set_debug(ctx, 3);

    // get all connected usb devices
    if((cnt = libusb_get_device_list(ctx, &list)) < 0) {
        fprintf(stderr,"Can't get devices list\n");
        fprintf(stderr,"libusb_get_device_list  Error : %s (%d)\n",
                strerror(errno),errno);
        libusb_exit(ctx);
        exit(EXIT_FAILURE);
    }

    int nbrinterf=0;

    // parse the list
    for (i = 0; i < cnt; i++) {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *config; 
        struct libusb_device *device = list[i];

        /* get device description */
        if (libusb_get_device_descriptor(device,&desc) != 0) {
            fprintf(stderr,"libusb_get_device_descriptor Error : %s (%d)\n",
                    strerror(errno),errno);
            continue;
        }

        if(desc.idVendor == targetdevice.vid && desc.idProduct == targetdevice.pid &&
           libusb_get_device_address(device) == targetdevice.devnum &&
               libusb_get_bus_number(device) == targetdevice.busnum) {
            // nedd config description to check interface number
            if (libusb_get_active_config_descriptor(device, &config) != 0) {
                fprintf(stderr,"libusb_get_device_descriptor Error : %s (%d)\n",
                        strerror(errno),errno);
                continue;
            }
            nbrinterf = config->bNumInterfaces;
            if(nbrinterf < 1) {
                fprintf(stderr, "Less than 1 interface. Too weird !\n");
                continue;
            }

            found = device;
            memcpy (&fdesc, &desc, sizeof(fdesc));
            break;
        }
    }

    // ok, we found the device
    if (found) {
        // get handle
        if(libusb_open(found, &devhaccess) != 0) {
            fprintf(stderr,"Unable to get handler on device\n");
            fprintf(stderr,"libusb_get_device_descriptor Error : %s (%d)\n",
                    strerror(errno),errno);
            libusb_free_device_list(list, 1);
            libusb_exit(ctx);
            exit(EXIT_FAILURE);
        }

        if(verbose) {
            printf("  %04x:%04x\n", fdesc.idVendor, fdesc.idProduct);
            printf("  iManufacturer: ");
            printdesc(devhaccess, fdesc.iManufacturer);
            printf("  iProduct: ");
            printdesc(devhaccess, fdesc.iProduct);
            printf("  iSerialNumber: ");
            printdesc(devhaccess, fdesc.iSerialNumber);
            printf("  on bus number %u with address %u\n", libusb_get_bus_number(found), libusb_get_device_address(found));
        }

        // are we acting for real ?
        if(!donothing) {
            int iface;
            for(iface = 0 ; iface < nbrinterf ; ++iface) 
            {
                if(libusb_kernel_driver_active(devhaccess, iface) == 1) {
                    if(verbose)
                        printf("kernel driver (or something) is active on interface %u... trying to detach\n", iface);
                    if(libusb_detach_kernel_driver(devhaccess, iface)) {
                        fprintf(stderr,"Unable to detach kernel driver from interface %u.\n", iface);
                        giveup(devhaccess,ctx);
                    }
                    if(verbose)
                        printf("Kernel driver detached.\n");
                } else {
                    if(verbose)
                        printf("No kernel attached. Nothing to do. Claiming device for checking.\n");
                }

                // just checking
                if(verbose) printf("Check with claiming interface %u\n",iface);
                if(libusb_claim_interface(devhaccess, iface) != 0) {
                    fprintf(stderr,"libusb_claim_interface  Error : %s (%d)\n",
                            strerror(errno),errno);
                    giveup(devhaccess,ctx);
                }

                if(verbose) printf("All interface %d are belong to me\n", iface);

                if(verbose) printf("Release interface %u...\n",iface);
                if(libusb_release_interface(devhaccess,iface) !=0)
                    fprintf(stderr,"libusb_release_interface  Error : %s (%d)\n",
                            strerror(errno),errno);
            }
        }

        if(verbose) printf("Closing device...\n");
        libusb_close(devhaccess);
    } else {
        fprintf(stderr,"Device not found\n");
    }

    /* For convenience, the libusb_free_device_list() function includes 
     * a parameter to optionally unreference all the devices in the 
     * list before freeing the list itself */
    if(verbose)
        printf("Freeing devices list\n");
    libusb_free_device_list(list, 1);

    if(verbose)
        printf("LibUSB exit...\n");
    libusb_exit(ctx);

    return(EXIT_SUCCESS);
}
