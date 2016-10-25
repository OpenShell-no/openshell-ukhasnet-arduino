
#include <stdint.h>
#include <avr/io.h>

#include "../config.h"

#ifndef peripherals_onewire_h
#define peripherals_onewire_h

/********************************************\
 * Currently only DS18B20 is supported.
 * DS1820 and DS18S20 are NOT supported.
\********************************************/

#include <OneWire.h>

extern OneWire onewire_obj;

void onewire_init();
void onewire_sample(bool wait=false);
void onewire_sample_wait();
double onewire_temperature(const uint8_t addr[8]);

#endif
