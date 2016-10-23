#include <stdint.h>
#ifndef utilities_util_h
#define utilities_util_h

int freeRam();


const unsigned long MAXULONG = 0xffffffff;
extern unsigned long now;
unsigned long getTimeSince(unsigned long ___start);

extern uint8_t reset_reason;
const char* reset_tostring();


/* ------------------------------------------------------------------------- */

typedef enum base_t {NO_CONVERT=0, BIN=2, OCT=8,  DEC=10, HEX=16} base_t;

const char* tostring(uint8_t n, base_t base=DEC);
const char* tostring(uint16_t n, base_t base=DEC);
const char* tostring(uint32_t n, base_t base=DEC);
const char* tostring(uint64_t n, base_t base=DEC);

const char* tostring(int8_t n, base_t base=DEC);
const char* tostring(int16_t n, base_t base=DEC);
const char* tostring(int32_t n, base_t base=DEC);
const char* tostring(int64_t n, base_t base=DEC);

const char* tostring(float n, uint8_t precission = 2, bool strip=true);
const char* tostring(double n, uint8_t precission = 2, bool strip=true);

/* ------------------------------------------------------------------------- */
#endif
