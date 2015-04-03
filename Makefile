TARGET  := udev-usbdetach
WARN    := -Wall 
CFLAGS  := -O2 ${WARN} `pkg-config libusb-1.0 libudev --cflags`
LDFLAGS := `pkg-config libusb-1.0 libudev --libs`
CC      := gcc


all: ${TARGET}

${TARGET}.o: ${TARGET}.c
${TARGET}: ${TARGET}.o
	${CC} ${TARGET}.o ${LDFLAGS} -o ${TARGET}

clean:
	rm -rf ${TARGET}.o ${TARGET}
