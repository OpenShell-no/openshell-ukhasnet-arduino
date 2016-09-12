#include <stdint.h>
#include <math.h>
#include <string.h>

#ifndef CONFIG_H
#define CONFIG_H

//#define USE_ONEWIRE
#define USE_RFM69
//#define SERIALDEBUG
// FIXME: USE_GPS is not optional yet, code needs to be fixed.
#define USE_GPS
//#define USE_DHT


/* TODO Config options */
extern char NODE_NAME[9]; // null-terminated string, max 8 bytes, A-z0-9
extern uint8_t NODE_NAME_LEN;
extern char HOPS; // '0'-'9'
extern uint16_t BROADCAST_INTERVAL;

extern float LATITUDE;
extern float LONGITUDE;
extern float ALTITUDE;

const uint8_t PAYLOADSIZE = 64;

extern double vsense_offset;
extern double vsense_mult;

extern bool vbat_enabled;
extern int    vbat_pin;
extern double vbat_offset;
extern double vbat_mult;

extern bool   vpanel_enabled;
extern int    vpanel_pin;
extern double vpanel_offset;
extern double vpanel_mult;

extern bool   powersave;
extern double powersave_treshold;



// TODO:40 Implement configuration protocol.
/* EEPROM SETTINGS
ukhasnet_enabled        bool
ukhasnet_interval       int

onewire_enabled         bool
onewire_pin             int

dht_enabled             bool
dht_pin                 int

rfm69_enabled           bool
rfm69_chipselect_pin    int
rfm69_reset_pin         int
rfm69_interrupt_pin     int
rfm69_frequency         float
rfm69_frequency_offset  float

nrf24_enabled           bool
nrf24_chipselect_pin    int
nrf24_channel           int

vbatsense_enabled       bool
vbatsense_pin           int
vbatsense_multiplier    float
vbatsense_offset        float

photoresistor_enabled   bool
photoresistor_pin       int

gps_enabled             bool
location_latitude       float
location_longitude      float
location_altitude       float

cputemp_enabled         bool
*/

#endif
