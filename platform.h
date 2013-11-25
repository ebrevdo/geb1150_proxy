#ifndef _PLATFORM
#define _PLATFORM

#if defined(_MSC_VER) || defined(_WIN32)
#define __WINDOWS__
#include <windows.h>
#endif

void msleep(int microsec);

#endif // _PLATFORM