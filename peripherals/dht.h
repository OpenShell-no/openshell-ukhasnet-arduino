#ifndef peripherals_dht_h
#define peripherals_dht_h

#include <stdint.h>

//DHT dht;

extern bool dht_lastresult;

bool dht_sample();

double dht_temperature();
double dht_humidity();

#endif
