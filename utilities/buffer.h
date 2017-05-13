#include <stdint.h>
#include "uart.h"

#ifndef BUFFER_H
#define BUFFER_H

const uint8_t BUFFERSIZE = 128;
extern uint8_t databuf[BUFFERSIZE];
extern uint8_t dataptr;


/* ------------------------------------------------------------------------- */
// TODO:60 make all functions respect BUFFERSIZE.

void resetData();

void addString(const char *value);
void addString(const __FlashStringHelper *ifsh);
void addFloat(double value, uint8_t precission = 2, bool strip=true);
void addCharArray(const char *value, uint8_t len);
void addLong(unsigned long value);
void addWord(uint16_t value);
void addByte(uint8_t value);

/* ------------------------------------------------------------------------- */

uint8_t c_find(uint8_t start, char sep, uint8_t count);
uint8_t c_find(uint8_t start, char sep);
uint8_t c_find(char sep, uint8_t count);
uint8_t c_find(char sep);

bool s_cmp(char* a, char* b, uint8_t count);
bool s_cmp(char* a, char* b);

uint8_t s_sub(char* source, char* target, uint8_t start, uint8_t end);
uint8_t s_sub(char* source, char* target, uint8_t end);

float parse_float(char* buf, uint8_t len);
float parse_float(char* buf);

#endif
