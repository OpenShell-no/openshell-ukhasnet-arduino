#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "buffer.h"

uint8_t databuf[BUFFERSIZE];
uint8_t dataptr = 0;

char _floatbuf[16] = "";

/* ------------------------------------------------------------------------- */
// TODO:50 make all functions respect BUFFERSIZE.

void resetData() {
  dataptr = 0;
}

// Add \0 terminated string, excluding \0
void addString(const char *value) {
    int i = 0;
    while (value[i]) {
        if (dataptr < BUFFERSIZE) {
            databuf[dataptr++] = value[i++];
        }
    }
}

void addString(const __FlashStringHelper *ifsh) {
  // from Arduino Print.cpp
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);

  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) {
      break;
    }
    if (dataptr < BUFFERSIZE) {
      databuf[dataptr++] = c;
    } else {
      break;
    }
  }
}


void addFloat(double value, uint8_t precission, bool strip) {
    dtostrf(value, 1, precission, _floatbuf);

    if (precission and strip) {
        uint8_t e = 0;
        for (uint8_t i=0;i<16;i++) {
            if (!_floatbuf[i]) {
                e = i-1;
                break;
            }
        }
        for (uint8_t i=e; i; i--) {
            if (_floatbuf[i] == '0') {
                _floatbuf[i] = 0;
            } else if (_floatbuf[i] == '.') {
                _floatbuf[i] = 0;
                break;
            } else {
                break;
            }
        }
    }
    for (uint8_t i=0; i<16;i++) {
        if (_floatbuf[i] == 0) {
            break;
        }
        if (_floatbuf[i] >= 'A' and _floatbuf[i] <= 'Z') {
            _floatbuf[i] += 32; // str.tolower()
        }
    }
    addString(_floatbuf);
}

void addCharArray(char *value, uint8_t len) {
  /* Add char array to the output data buffer (databuf) */
  for (uint8_t i=0; i<len; i++) {
    if (dataptr < BUFFERSIZE) {
      databuf[dataptr++] = value[i];
    }
  }
}

void addLong(unsigned long value) { // long, unsigned long
  databuf[dataptr++] = value >> 24 & 0xff;
  databuf[dataptr++] = value >> 16 & 0xff;
  databuf[dataptr++] = value >> 8 & 0xff;
  databuf[dataptr++] = value & 0xff;
}

void addWord(uint16_t value) { // word, int, unsigned int
  databuf[dataptr++] = value >> 8 & 0xff;
  databuf[dataptr++] = value & 0xff;
}

void addByte(uint8_t value) { // byte, char, unsigned char
  databuf[dataptr++] = value;
}

/* ------------------------------------------------------------------------- */

uint8_t c_find(uint8_t start, char sep, uint8_t count) {
    uint8_t pos = start;
    for (uint8_t i=0; i < count; i++) {
        while (databuf[pos++] != sep) {}
    }
    return pos;
}

uint8_t c_find(uint8_t start, char sep) {
    return c_find(start, sep, 1);
}

uint8_t c_find(char sep, uint8_t count) {
    return c_find(0, sep, count);
}

uint8_t c_find(char sep) {
    return c_find(0, sep, 1);
}

bool s_cmp(char* a, char* b, uint8_t count) {
    for (uint8_t i=0;i<count;i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

bool s_cmp(char* a, char* b) {
    uint8_t i=0;
    while (a[i] != '\0' and b[i] != '\0') {
        if (a[i] != b[i]) {
            return false;
        }
        i++;
    }
    return true;
}

uint8_t s_sub(char* source, char* target, uint8_t start, uint8_t end) {
    uint8_t i;
    for (i=0; i<end-start; i++) {
        target[i] = source[start+i];
    }
    target[++i] = '\0';
    return end - start;
}

uint8_t s_sub(char* source, char* target, uint8_t end) {
    return s_sub(source, target, 0, end);
}

float parse_float(char* buf, uint8_t len) {
    //Serial.println(buf);

    float _f_mult = 0.1;
    bool _neg = buf[0] == '-';

    for (uint8_t i=0; i<len; i++) {
        if (buf[i] == '.') {
            break;
        }
        if (buf[i] >= '0' and buf[i] <= '9') {
            _f_mult *= 10;
        }
    }

    float res = 0;

    for (uint8_t i=0; i<len; i++) {
        if (buf[i] >= '0' and buf[i] <= '9') {
            res += (buf[i] - 48) * _f_mult;
            _f_mult /= 10;
        }
    }

    if (_neg) {
        res *= -1;
    }

    //Serial.println(res);
    return res;
}

float parse_float(char* buf) {
    return parse_float(buf, strlen(buf));
}
