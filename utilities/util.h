#include <stdint.h>
#ifndef utilities_util_h
#define utilities_util_h

int freeRam();


const unsigned long MAXULONG = 0xffffffff;
extern unsigned long now;
unsigned long getTimeSince(unsigned long ___start);

#endif
