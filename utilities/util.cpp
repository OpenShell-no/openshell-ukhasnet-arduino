#include <stdint.h>
#include "util.h"
#include "timer.h"

unsigned long now;

int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

unsigned long getTimeSince(unsigned long ___start) {
    unsigned long interval;
    now = millis();
    if (___start > now) {
        interval = MAXULONG - ___start + now;
    } else {
        interval = now - ___start;
    }
    return interval;
}

/* ------------------------------------------------------------------------- */

char _formatbuf[8 * sizeof(uint64_t) + 2] = "";

const char* tostring(uint64_t n, base_t base) {
  /* From Arduino Print.cpp */
  char *str = &_formatbuf[sizeof(_formatbuf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) {base = DEC;}

  do {
    char c = n % base;
    n /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(n);

  return str;
}

const char* tostring(int64_t n, base_t base) {
  /* From Arduino Print.cpp */
  char *str = &_formatbuf[sizeof(_formatbuf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) {base = DEC;}
  bool negative = n < 0;
  if (negative) {
    n = -n;
  }

  do {
    char c = n % base;
    n /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(n);

  if (negative) {
    *--str = '-';
  }

  return str;
}

const char* tostring(uint8_t n, base_t base) {
  return tostring((uint64_t)n, base);
}

const char* tostring(uint16_t n, base_t base) {
  return tostring((uint64_t)n, base);
}

const char* tostring(uint32_t n, base_t base) {
  return tostring((uint64_t)n, base);
}

const char* tostring(int8_t n, base_t base) {
  return tostring((int64_t)n, base);
}

const char* tostring(int16_t n, base_t base) {
  return tostring((int64_t)n, base);
}

const char* tostring(int32_t n, base_t base) {
  return tostring((int64_t)n, base);
}

/* ------------------------------------------------------------------------- */
