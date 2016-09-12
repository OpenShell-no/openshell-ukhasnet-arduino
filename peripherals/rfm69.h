#ifndef peripherals_rfm69_h
#define peripherals_rfm69_h

#include <stdint.h>

#include "../utilities/buffer.h"

extern uint8_t rfm_txpower;
extern float rfm_freq_trim;
extern int16_t lastrssi;
extern int rfm69_reset_pin;
extern int rfm69_chipselect_pin;


void rfm69_reset();

extern uint8_t freqbuf[3];

void rfm69_set_frequency(float freqMHz);

void send_rfm69();
void dump_rfm69_registers();

#endif
