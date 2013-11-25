#ifndef _NETFUNC
#define _NETFUNC

#include "platform.h"

#ifdef __WINDOWS__
#include <windows.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h>
#endif

#include <sys/types.h>

int init_net();
void cleanup_net();
int net_errno();
int net_close(int s);
int net_ioctl(int s, long cmd, unsigned long* argp);

#endif // _NETFUNC
