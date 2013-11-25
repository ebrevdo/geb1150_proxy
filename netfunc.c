#include "netfunc.h"

int init_net()
{
#ifdef __WINDOWS__
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
	    // Tell the user that we could not find a usable
	    // WinSock DLL.
		return err;
	}
#endif // __WINDOWS__

	return 0;
}

void cleanup_net()
{
#ifdef __WINDOWS__
	WSACleanup();
#endif __WINDOWS__
}

int net_errno()
{
#ifdef __WINDOWS__
	return WSAGetLastError();
#else
	return errno;
#endif
}

int net_close(int s)
{
#ifdef __WINDOWS__
	return closesocket(s);
#else
	return close(s);
#endif
}

int net_ioctl(int s, long cmd, unsigned long* argp)
{
#ifdef __WINDOWS__
	return ioctlsocket(s, cmd, argp);
#else
	return ioctl(s, cmd, argp);
#endif
}
