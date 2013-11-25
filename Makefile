STATIC := $(shell libusb-config --libs | tr ' ' '\n' | grep -e '^-L' | cut -dL -f2-)
CFLAGS = -g $(libusb-config --cflags)
LDFLAGS = $(libusb-config --libs)

geb1150_proxy: netfunc.o platform.o usbcontrol.o $(STATIC)/libusb.a

clean:
	rm -rf *.o geb1150_proxy
