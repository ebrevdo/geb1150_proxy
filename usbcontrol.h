#ifndef _USBCONTROL
#define _USBCONTROL

#include <usb.h>

#define USB_TIMEOUT 10 // 10ms

// Data types
enum reader_command_t { r_NULL, r_gethostbyname, r_socket, r_connect, r_send, r_recv, r_close };

// TODO, is this the best way to deal with reader requests?
struct reader_command
{
	enum reader_command_t cmd; // Command
	int n;      // Flow control number
	char *sp1;  // String parameter 1
	int  ip1;   // Integer parameter 1
	int  ip2;   // Integer parameter 2
};

// Functions
struct usb_device *device_init();
int init_reader(struct usb_device **reader_dev,struct usb_dev_handle **reader);
void cleanup_usb(struct usb_device *reader_dev,struct usb_dev_handle *reader);
int wait_command(struct usb_dev_handle *reader, struct reader_command *command);
void cleanup_command(struct reader_command *command);
int send_ack_with_msg(struct usb_dev_handle *reader, int n, int l, char *msg);
int send_ack(struct usb_dev_handle *reader, int n, int l);
int send_nak_with_msg(struct usb_dev_handle *reader, int n, int e);

#endif // _USBCONTROL
