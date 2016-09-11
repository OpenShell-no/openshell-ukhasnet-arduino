#ifndef peripherals_onewire_h
#define peripherals_onewire_h

#ifdef UNDEFINED__ // TODO: Implement/port OneWire
#include <stdint.h>

#include <OneWire.h>
#include <DallasTemperature.h>

/* TODO:0 Config options */
const int OWPIN = 9; // 1-wire bus connected on pin 9
const int DSRES = 12; // 12-bit temperature resolution

OneWire onewire(OWPIN);
DallasTemperature dstemp(&onewire);
DeviceAddress dsaddr;



double getDSTemp();
#endif
#endif
