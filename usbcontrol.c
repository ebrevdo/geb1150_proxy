#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usbcontrol.h"

#define GEB_VENDOR_ID	0x0993	// Gemstar eBook Group, Ltd
#define GEB_PRODUCT_ID	0x0002	// GEB 1150?
#define GEB_INTERFACE	0		// Main interface
#define GEB_EA_IN	    0x82	// bEndpointAddress for reading from the eBook
#define GEB_EA_OUT		0x02	// bEndpointAddress for writing to the eBook

int init_reader(struct usb_device **reader_dev,struct usb_dev_handle **reader)
{
    int err=0;
    char msgin[128]=""; memset(msgin, 0, 128);

    // Try to find the device
    *reader_dev = device_init();
    if (*reader_dev == NULL) return -1;

    // Try to open the device
    *reader = usb_open(*reader_dev);
    if (reader == NULL) return -1;

    //#ifdef __WINDOWS__
    // Set the configuration value to 1 so that we can use it
    err = usb_set_configuration(*reader, 1);
    if (err<0) return -2;
    //#else
    //#endif

    // Attempt to claim an interface
    err = usb_claim_interface(*reader, GEB_INTERFACE);
    if (err<0) return -3;

    // Initialize communications.  Tell the eBook we're here.
    //  * Start the flow control with integer n=0
    //  * We're about to send the version control number,
    //    it will be 3 bytes
    err = usb_bulk_write(*reader, GEB_EA_OUT,
        "start,0,3,",
        10,USB_TIMEOUT);
    if (err<0) return -4;

    // Send the communications version number
    err = usb_bulk_write(*reader, GEB_EA_OUT,
        "1.0", // Version number 1.0
        3,USB_TIMEOUT);
    if (err<0) return -4;

    // Verify that both were received OK
    err = usb_bulk_read(*reader, GEB_EA_IN, msgin, sizeof(msgin), USB_TIMEOUT);
    if (err<0) return -4;
    else
        {
            msgin[err] = 0; // End of line
            fprintf(stderr,"received %d bytes: %s\n", err, msgin);
        }

    err = usb_bulk_read(*reader, GEB_EA_IN, msgin, sizeof(msgin), USB_TIMEOUT);
    if (err<0) return -4;
    else
        {
            msgin[err] = 0; // End of line
            fprintf(stderr,"received %d bytes: %s\n", err, msgin);
        }

    return 0;
}

void cleanup_usb (struct usb_device *reader_dev,struct usb_dev_handle *reader)
{
    /*
      if (reader != NULL)
      {
      // Attempt to release the interface
      usb_release_interface(reader, GEB_INTERFACE);

      // Set the configuration parameter back to normal
      //#ifdef __WINDOWS__
      usb_set_configuration(reader, 0);
      //#else
      //#endif
      }
    */

    if (reader_dev != NULL)
        {
            // Attempt to close the device
            usb_close(reader);
        }
}

struct usb_device *device_init()
{
    struct usb_bus *usb_bus;
    struct usb_device *dev;
    usb_find_busses();
    usb_find_devices();

    // for (usb_bus = usb_busses; ...
    for (usb_bus = usb_get_busses(); usb_bus; usb_bus = usb_bus->next) {
        for (dev = usb_bus->devices; dev; dev = dev->next)
            {
                //fprintf(stderr,"%d,%d,%x\n", dev->descriptor.idVendor, dev->descriptor.idProduct,dev);

                if ((dev->descriptor.idVendor == GEB_VENDOR_ID) &&
                    (dev->descriptor.idProduct == GEB_PRODUCT_ID))
                    {
                        return dev;
                    }
            }
    }
    return NULL;
}

int parse_int(char *msg, int *offset)
{
    char *t;
    int i;
    i = strtol(&msg[*offset], &t, 10);
    *offset = *offset + (t-&msg[*offset]); // Add on the number of characters we read
    if (msg[*offset]==',')
        *offset = *offset + 1;
    return i;
}

void cleanup_command(struct reader_command *command)
{
    if (command == NULL)
        return;

    memset(command, 0, sizeof(struct reader_command));
    command->cmd = r_NULL;
    command->ip1 = 0;
    command->ip2 = 0;
    command->n = 0;
    if (command->sp1 != NULL) {
        free(command->sp1);
        command->sp1 = NULL;
    }
}

int wait_command(struct usb_dev_handle *reader, struct reader_command *command)
{
    int len=0; int offset=0; int l=0;
    char msgin[8192]="";
    len = usb_bulk_read(reader, GEB_EA_IN, msgin, sizeof(msgin), USB_TIMEOUT);
    if (len <= 0) return len; // Either an error or nothing
    msgin[len]=0; // End of message, set to zero for parse_int()
    // Ideally, we would memset the rest of the buffer to zero

    fprintf(stderr,"command in: %s\n",msgin);
    // Parse the command.  Possible commands:
    //   gethostbyname, socket, connect, send, recv, close
    if (strncmp(msgin,"recv,",5) == 0)
        {
            command->cmd = r_recv;
            offset = 5;
            command->n = parse_int(msgin, &offset);
            l = parse_int(msgin, &offset); // Length of remainder of data
            command->ip1 = parse_int(msgin, &offset); // Socket number to receive on
            command->ip2 = parse_int(msgin, &offset); // How many bytes to receive

            fprintf(stderr,"recv: socket: %d, data len: %d\n", command->ip1, command->ip2);
        }
    else if (strncmp(msgin,"send,",5) == 0)
        {
            command->cmd = r_send;
            offset = 5;
            command->n = parse_int(msgin, &offset);
            l = parse_int(msgin, &offset); // Length of remainder of data
            command->ip1 = parse_int(msgin, &offset); // Socket number to send on
            command->ip2 = parse_int(msgin, &offset); // Length of data to send
            command->sp1 = (char *)malloc(command->ip2+1); memset(command->sp1, 0, command->ip2+1);
            memcpy(command->sp1, &msgin[offset], command->ip2); // Store the data to send

            fprintf(stderr,"send: socket: %d, data len: %d\n", command->ip1, l);
        }
    else if (strncmp(msgin,"gethostbyname,",14)==0)
        {
            offset = 14;
            command->cmd = r_gethostbyname;
            command->n = parse_int(msgin, &offset);
            l = parse_int(msgin, &offset); // Length of remainder of string
            command->sp1 = (char *)malloc(l+1); memset(command->sp1, 0, l+1);
            memcpy(command->sp1, &msgin[offset], l); // Store the hostname

            fprintf(stderr,"gethostbyname: flow number: %d, length: %d, host: %s\n",
                command->n, l, command->sp1);
        }
    else if (strncmp(msgin,"socket,",7) == 0)
        {
            // TODO, add socket to list of sockets for later automated cleanup
            command->cmd = r_socket;
            offset = 7;
            command->n = parse_int(msgin, &offset);
            command->ip1 = parse_int(msgin, &offset); // Should be '0'

            fprintf(stderr,"socket: flow number: %d, param: %d\n", command->n, command->ip1);
        }
    else if (strncmp(msgin,"connect,",8) == 0)
        {
            command->cmd = r_connect;
            offset = 8;
            command->n = parse_int(msgin, &offset);
            l = parse_int(msgin, &offset); // Length of remainder of command
            command->ip1 = parse_int(msgin, &offset); // Socket to connect on
            l = ((char *)memchr(&msgin[offset], ',', l) - &msgin[offset]); // Length of IP
            command->sp1 = (char *)malloc(l+1);  memset(command->sp1, 0, l+1);
            strncpy(command->sp1, &msgin[offset], l); // Store the IP
            offset = offset + l + 1; // Skip the IP & separator
            command->ip2 = parse_int(msgin, &offset); // Store the port

            fprintf(stderr,"connect: flow number: %d, socket: %d, hostname: %s, port: %d\n",
                command->n, command->ip1, command->sp1, command->ip2);
        }
    else if (strncmp(msgin,"close,",6) == 0)
        {
            command->cmd = r_close;
            offset = 6;
            command->n = parse_int(msgin, &offset);
            l = parse_int(msgin, &offset); // Length of remainder of data
            command->ip1 = parse_int(msgin, &offset); // Socket to close

            fprintf(stderr,"close: flow number: %d, socket: %d\n", command->n, command->ip1);
        }
    return len;
}

int send_ack_with_msg(struct usb_dev_handle *reader, int n, int l, char *msg)
{
    int err=0;
    char msgf[64] = ""; memset(msgf,0,64);
    sprintf(msgf, "ack,%d,%d,", n, l);
    err = usb_bulk_write(reader, GEB_EA_OUT, msgf, strlen(msgf), USB_TIMEOUT);
    if (err < 0) return err;

    fprintf(stderr, "ack sent: %s\n", msgf);

    err = usb_bulk_write(reader, GEB_EA_OUT, msg, l, USB_TIMEOUT);
    if (err < 0) return err;

    fprintf(stderr, "ack comment sent (length %d, sent %d)\n", l, err);

    return err;
}

int send_nak_with_msg(struct usb_dev_handle *reader, int n, int e)
{
    int err=0;
    char msgf[64] = "";
    char estr[8] = "";
    memset(msgf,0,64);
    memset(estr,0,8);

    sprintf(estr, "%d", e);
    sprintf(msgf, "nak,%d,%d,", n, strlen(estr));
    err = usb_bulk_write(reader, GEB_EA_OUT, msgf, strlen(msgf), USB_TIMEOUT);
    if (err < 0) return err;

    fprintf(stderr, "nak sent: %s\n", msgf);

    err = usb_bulk_write(reader, GEB_EA_OUT, estr, strlen(estr), USB_TIMEOUT);
    if (err < 0) return err;

    fprintf(stderr, "nak comment sent: %s\n", estr);

    return err;
}

int send_ack(struct usb_dev_handle *reader, int n, int l)
{
    int err=0;
    char msgf[64] = ""; memset(msgf,0,64);

    sprintf(msgf, "ack,%d,%d,", n, l);
    err = usb_bulk_write(reader, GEB_EA_OUT, msgf, strlen(msgf), USB_TIMEOUT);
    if (err < 0) return err;

    fprintf(stderr, "ack sent: %s\n", msgf);

    return err;
}
