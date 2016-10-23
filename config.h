#include <stdint.h>
#include <math.h>
#include <string.h>

#ifndef CONFIG_H
#define CONFIG_H

#define USE_RFM69
//#define SERIALDEBUG
// FIXME: USE_GPS is not optional yet, code needs to be fixed.
#define USE_GPS
#define USE_DHT
#define USE_BME280
#define USE_ONEWIRE


/* TODO Config options */
extern char node_name[9]; // null-terminated string, max 8 bytes, A-z0-9
extern uint8_t node_name_len;
extern char hops; // '0'-'9'
extern uint16_t broadcast_interval;

struct rfm69_config_t {
  bool enabled:1;
  bool listen:1;
  uint8_t txpower:5;
  uint8_t txpower_low:5;
  float frequency;
  float frequency_trim;
};
extern rfm69_config_t rfm69_cfg;

extern float latitude;
extern float longitude;
extern float altitude;

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

struct bme280_config_t {
  bool enabled:1;
  struct {
    bool enabled:1;
    uint8_t oversampling:3;
  } temperature, pressure, humidity;
};

extern bme280_config_t bme280_cfg;

struct dht_config_t {
  bool enabled:1;
  struct {
    bool enabled:1;
  } temperature, humidity;
};
extern dht_config_t dht_cfg;


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
