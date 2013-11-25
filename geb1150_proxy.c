#include "usbcontrol.h"
#include "netfunc.h"
#include "platform.h"

#include <stdio.h>
#include <string.h>

#define PROXY_PORT 8079

void cleanup(struct usb_device *reader_dev, struct usb_dev_handle *reader)
{
    cleanup_net();
    cleanup_usb(reader_dev, reader);
    //getchar();
}

int handle_command(struct usb_dev_handle *reader, struct reader_command *command)
{
    struct hostent* host=NULL;
    char *t=NULL;
    int err=0, i=0, j=0, s=0, si=0;
    struct in_addr addr;
    struct sockaddr_in connection;
    fd_set rfds;
    struct timeval tv;

    switch (command->cmd) {
    case r_gethostbyname:
        host = gethostbyname(command->sp1);
        if (host == NULL)
            {
                // Send a nak with "h_errno"
                fprintf(stderr,"host not found: h_errno %d\n", h_errno);
                err = send_nak_with_msg(reader, command->n, h_errno);
            }
        else
            {
                // Send an ack with the IP
                //t = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);

                // Send my ip
                //t = (char *)malloc(sizeof(ip));
                //strcpy(t, ip);

                t = "127.0.0.1";
                fprintf(stderr,"host: %s, ip: %s\n", command->sp1, t);
                err = send_ack_with_msg(reader, command->n, strlen(t), t);
            }
        break;
    case r_socket:
        j = 1;
        i = socket(PF_INET, SOCK_STREAM, 0);
        //if (i != -1) si = net_ioctl(i, FIONBIO, &j); // Set to non-blocking mode
        if (i == -1)
            { // Send a nak?
                fprintf(stderr,"unable to open socket: %d (%s)\n", errno, strerror(errno));
                err = send_nak_with_msg(reader, command->n, i);
            }
        else if (si != 0) // Problem setting non-blocking mode
            {
                fprintf(stderr,"unable to set socket to nonblocking mode.  socket: %d, error: %d\n",
                    i, net_errno());
            } else // All OK
            {
                s=i; // Store socket number somewhere

                i = 8192*16; // Send buffer temporary store
                i = setsockopt(s, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i));
                if (i == -1) fprintf(stderr, "Error on setsockopt: SO_SNDBUF\n");
                i = 8192*16; // Receive buffer temporary store
                i = setsockopt(s, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i));
                if (i == -1) fprintf(stderr, "Error on setsockopt: SO_RCVBUF\n");

                /*
                  si = sizeof(i);
                  err =  getsockopt(s, SOL_SOCKET, SO_SNDBUF, &i, &si);
                  if (err == -1 ) fprintf(stderr, "Error on getsockopt: SO_SNDBUF\n");
                  fprintf(stderr,"size of send buffer: %d\n", i);
                  si = sizeof(i);
                  err =  getsockopt(s, SOL_SOCKET, SO_RCVBUF, &i, &si);
                  if (err == -1 ) fprintf(stderr, "Error on getsockopt: SO_RCVBUF\n");
                  fprintf(stderr,"size of receive buffer: %d\n", i);
                */

                t = (char *)malloc(10); memset(t, 0, 10);
                sprintf(t, "%d", s);
                err = send_ack_with_msg(reader, command->n, strlen(t), t);

                fprintf(stderr,"socket: %s\n", t);
                free(t);
            }

        break;
    case r_connect:
        // Convert IP to int.
        //i = inet_addr(command->sp1); memcpy(&addr, &i, sizeof(i));

        connection.sin_family = PF_INET;

        // Connect to localhost only!
        connection.sin_port = htons(PROXY_PORT);
        connection.sin_addr.s_addr = inet_addr("127.0.0.1");

        //connection.sin_port = htons(command->ip2); // Port in network short
        //connection.sin_addr.s_addr = inet_addr(command->sp1); //addr;

        i = connect(command->ip1, (struct sockaddr *)&connection, sizeof(connection));
        if (i != 0) { // Error connecting
            i = net_errno();
            fprintf(stderr,"connect failed: %d\n", i);
            err = send_nak_with_msg(reader, command->n, i);
        }
        else // OK!
            {
                fprintf(stderr,"connect: OK!\n");
                err = send_ack(reader, command->n, 0);
            }

        break;
    case r_send:
        i = send(command->ip1, command->sp1, command->ip2, 0);
        fprintf(stderr,"send: length sent: %d, data length was: %d\n", i, command->ip2);
        if (i == -1) // Problem sending
            {
                i = net_errno();
                fprintf(stderr,"send error: %d (%s)\n", i, strerror(i));
                err = send_nak_with_msg(reader, command->n, i);
            } else { // No problems
            fprintf(stderr,"send: socket: %d, sent length: %d, data length: %d\n",
                command->ip1, i, command->ip2);


            t = (char *)malloc(10); memset(t, 0, 10);
            sprintf(t, "%d", i);
            err = send_ack_with_msg(reader, command->n, strlen(t), t);
            free(t);
        }
        break;
    case r_recv:
        t = (char *)malloc(command->ip2+1); memset(t, 0, command->ip2+1);
        fprintf(stderr,"recv: attempting on socket %d, requested length %d\n", command->ip1, command->ip2);

        FD_ZERO(&rfds);
        FD_SET(command->ip1,&rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        /*	
            si = select(command->ip1+1, &rfds, NULL, NULL, &tv);

            if (si == -1) // Problem with select
            {
            fprintf(stderr, "socket %d: select returns -1\n", command->ip1);
            err = send_nak_with_msg(reader, command->n, 0);
            } else if (!FD_ISSET(command->ip1, &rfds)) { // Nothing available yet
            fprintf(stderr, "socket %d: select returns 0\n", command->ip1);
            err = send_nak_with_msg(reader, command->n, 0);
            } else */
        if ((i = recv(command->ip1, t, command->ip2, 0)) == -1) {
            fprintf(stderr,"recv error: %d (%s)\n", i, strerror(i));
            err = send_nak_with_msg(reader, command->n, 0);
        } else if (i == 0) {
            fprintf(stderr,"socket %d: recv length zero\n");
            err = send_nak_with_msg(reader, command->n, -57);

            /*
              if (i == -1 || i == 0) // Problem receiving, or received nothing
              {
              fprintf(stderr,"recv error: %d (%s)\n", i, strerror(i));
              if (i == -1)
              err = send_nak_with_msg(reader, command->n, 0); // i insead of 0?
              else
              err = send_nak_with_msg(reader, command->n, -57); // No data

              //i = net_errno();
              */
        }
        else { // No problems
            fprintf(stderr,"recv: socket: %d, requested length: %d, data length: %d\n",
                command->ip1, command->ip2, i);

            err = send_ack_with_msg(reader, command->n, i, t);
        }
        free(t);
        break;
    case r_close:
        i = net_close(command->ip1);
        if (i == -1) { // Send a nak?
            i = net_errno();
            fprintf(stderr,"unable to close socket: %d (%s)\n", i, strerror(i));
            err = send_nak_with_msg(reader, command->n, i);
        }
        else // All OK
            {
                //t = (char *)malloc(10); memset(t, 0, 10);
                //sprintf(t, "%d", i);
                err = send_ack(reader, command->n, 0);

                fprintf(stderr,"close: %d\n", i);
                //free(t);
            }
        break;
    default:
        break;
    }

    return err;
}


int main(int argc, char **argv)
{
    struct usb_device *reader_dev = NULL;
    struct usb_dev_handle *reader = NULL;
    struct reader_command command = {0}; // TODO: DEFINE THIS
    int err = 0;

    err = init_net();
    if (err != 0) {
        fprintf(stderr, "Unable to initialize network support\n");
        return 1;
    }

    // Initialize libUSB once
    usb_init();

    for (;1;msleep(100)) { // Keep looping, check to see if reader is plugged in or not
        err = init_reader(&reader_dev, &reader);

        if (err != 0) {
            if (err != -1)
                fprintf(stderr,"err: %d\n",err);

            // Device not found
            if (reader_dev == NULL) {
                //fprintf(stderr, "eBook not found\n");
                cleanup_usb(reader_dev, reader);
                continue;
            }
            // Couldn't open the device
            else if (reader == NULL) {
                fprintf(stderr, "Not able to connect to the eBook\n");
                cleanup_usb(reader_dev, reader);
                continue;
            }
            else if (err == -2) {
                fprintf(stderr, "Could not set the configuration parameter on the eBook\n");
                cleanup_usb(reader_dev, reader);
                continue;
            }
            else if (err == -3) {
                fprintf(stderr, "Could not claim an interface to the eBook\n");
                cleanup_usb(reader_dev, reader);
                continue;
            }
            else {
                fprintf(stderr, "Trouble initializing communications with the eBook\n");
                cleanup_usb(reader_dev, reader);
                continue;
            }
        } // endif on init_reader

        // We've found it and initialized.  Now loop, waiting for a command
        for (;1;msleep(1)) {
            // Clean up previous command
            cleanup_command(&command);

            err = wait_command(reader, &command);
            //fprintf(stderr,"wait command response: %d\n",err);
            if (err < 0) {
                fprintf(stderr, "Communications broken on wait_command\n");
                break; // Had an error communicating, stop.
            }
            if (err == 0) continue; // No message

            err = handle_command(reader, &command);
            if (err < 0) {
                fprintf(stderr, "Communications broken on handle_command\n");
                break; // Had an error communicating, stop.
            }
            if (err == 0) continue; // No message
        } // endfor waiting for a command


        cleanup_usb(reader_dev, reader);
    } // endfor (;true;...)
    /*
    // write start sequence to it
    i = usb_interrupt_write(usb_handle, 0x02, "start,0,3,",10,WAIT_TIMEOUT);
    if (i<0) perror("usb_interrupt_write 1");
    else fprintf(stderr,"bytes written: %d\n", i);

    i = usb_interrupt_write(usb_handle, 0x02, "1.0", 3,WAIT_TIMEOUT);
    if (i<0) perror("usb_interrupt_write 2");
    else fprintf(stderr,"bytes written: %d\n", i);

    // read possible reply on 0x81
    i = usb_interrupt_read(usb_handle, 0x81, strin, 512, WAIT_TIMEOUT);
    if (i<0) perror("usb_interrupt_read 0x81");
    else fprintf(stderr,"read %d chars on 0x81: %s\n",i, strin);

    // read reply
    i = usb_interrupt_read(usb_handle, 0x82, strin, 512, WAIT_TIMEOUT);
    if (i<0) perror("usb_interrupt_read");
    else fprintf(stderr,"read %d chars: %s\n", i, strin);
    */

    cleanup(reader_dev, reader);

    return 0;
}
