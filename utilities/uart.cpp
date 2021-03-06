#include <stdint.h>
#include <string.h>
#include "uart.h"

size_t serial0_write(const char buf[], size_t count) {
  for (int i=0; i<count; i++) {
    serial0_write(buf[i]);
  }
  return count;
}

size_t serial0_write(const char buf[]) {
  return serial0_write(buf, strlen(buf));
}

size_t serial0_print(char c) {
  return serial0_write(c);
}

size_t serial0_print(const char buf[]) {
  return serial0_write(buf);
}

size_t serial0_print(const __FlashStringHelper *ifsh) {
  // from Arduino Print.cpp
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  size_t n = 0;
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) {
      break;
    }
    if (serial0_write(c)) {
      n++;
    } else {
      break;
    }
  }
  return n;
}

size_t serial0_println() {
  return serial0_write("\r\n");
}

size_t serial0_println(char c) {
  return serial0_write(c) + serial0_println();
}

size_t serial0_println(const char buf[]) {
  return serial0_write(buf) + serial0_println();
}

size_t serial0_println(const __FlashStringHelper *ifsh) {
  return serial0_print(ifsh) + serial0_println();
}

//size_t serial0_print(unsigned int)
