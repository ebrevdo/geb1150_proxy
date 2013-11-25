#include "platform.h"

#ifdef __WINDOWS__
#else
#include <unistd.h>
#endif

void msleep(int microsec) {
#ifdef __WINDOWS__
	Sleep(microsec*10);
#else
	usleep((unsigned long) microsec);
#endif
}
