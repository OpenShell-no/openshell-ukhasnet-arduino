#include <stdint.h>
#include <Arduino.h>

int freeRam();


const unsigned long MAXULONG = 0xffffffff;
unsigned long now;
unsigned long getTimeSince(unsigned long ___start);