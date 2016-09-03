#include <stdint.h>

#include <UKHASnetRFM69-config.h>
#include <UKHASnetRFM69.h>

#include "../utilities/buffer.h"

byte rfm_txpower = 20;
float rfm_freq_trim = 0.068f;
int16_t lastrssi = 0;

#ifdef ESP8266
  int rfm69_reset_pin = 15;
  int rfm69_chipselect_pin = 0;
#else
  int rfm69_reset_pin = 9;      // ATMEGA328PB PB1=9
  int rfm69_chipselect_pin = 8; // ATmega328PB PB0=8
#endif


void rfm69_reset();

uint8_t freqbuf[3];
long _freq;
void rfm69_set_frequency(float freqMHz);

void send_rfm69();
void dump_rfm69_registers();
