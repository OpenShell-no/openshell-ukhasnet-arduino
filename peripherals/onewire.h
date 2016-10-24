
#include <stdint.h>
#include <avr/io.h>

#include "../config.h"

#ifndef peripherals_onewire_h
#define peripherals_onewire_h

#include <OneWire.h>

extern OneWire onewire_obj;

void onewire_init();
void onewire_sample();

#endif
