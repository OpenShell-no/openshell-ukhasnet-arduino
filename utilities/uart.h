#include <string.h>
#include <avr/pgmspace.h>

#ifndef UTILITIES_UART_H
#define UTILITIES_UART_H

/* ------------------------------------------------------------------------- */
// From Arduino's WString.h
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
/* ------------------------------------------------------------------------- */


//Helper functions
size_t serial0_write(const char buf[]);
size_t serial0_write(const char buf[], size_t count);

size_t serial0_print(char c);
size_t serial0_print(const char buf[]);
size_t serial0_print(const __FlashStringHelper *ifsh);

size_t serial0_println();
size_t serial0_println(char c);
size_t serial0_println(const char buf[]);
size_t serial0_println(const __FlashStringHelper *ifsh);



// Hardware implementation
void serial0_init(unsigned long baud=9600) __attribute__((weak));
size_t serial0_write(char data) __attribute__((weak));
void serial0_flush() __attribute__((weak));
char serial0_read() __attribute__((weak));
bool serial0_available() __attribute__((weak));

#endif
