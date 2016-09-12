#include <stdint.h>
#include <avr/io.h>
#include "../config.h"
#include "../utilities/timer.h"

void yield() {} // TODO: yield should probably do something sane

double getChipTemp() {
  uint16_t wADC;

  // Set the internal reference and mux.
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)
  ADMUX = 0b10001111; // REFS2 = 0; REFS1 = 1; REFS0 = 0; MUX3:0 = 0b1111
                      // Reference = 1.1V, Measure ADC4 (Temperature)
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined (__AVR_ATtiny84__)
  ADMUX = 0b10100010; // REFS1 = 1; REFS0 = 0; MUX5:0 = 0b100010
                      // Reference = 1.1V, Measure ADC8 (Temperature)
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)  || defined (__AVR_ATmega328PB__)
  ADMUX = 0b11001000; // REFS1 = 1; REFS0 = 1; MUX3:0 = 0b1000
                      // Reference = 1.1V, Measure ADC8 (Temperature)
#else
  #error Unknown CPU type, not implemented.
#endif
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));
  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;
  // The offset of 324.31 could be wrong. It is just an indication.
  return (wADC - 244.31d) / 1.22d;
}

double getVCCVoltage() {
  uint16_t wADC;

  // Set the internal reference and mux.
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)
  ADMUX = 0b00001110; // REFS2 = 0; REFS1 = 0; REFS0 = 0; MUX3:0 = 0b1100
                      // Reference = VCC, Measure 1.1V(VBG) bandgap
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined (__AVR_ATtiny84__)
  ADMUX = 0b00100001; // REFS1 = 0; REFS0 = 0; MUX5:0 = 0b100001
                      // Reference = VCC, Measure 1.1V(VBG) bandgap
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328PB__)
  ADMUX = 0b01001110; // REFS1 = 0; REFS0 = 1; MUX3:0 = 0b1110
                      // Reference = AVCC, Measure 1.1V(VBG) bandgap
#else
  #error Unknown CPU type, not implemented.
#endif
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);             // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  while (bit_is_set(ADCSRA, ADSC)); // Wait for conversion to finish.

  wADC = ADCW;
  return wADC ? (1.1d * 1023) / wADC : -1;
}

double readADCVoltage(uint8_t adc, double multiplier, double offset) {
    // ATmega328PB Datasheet: 29.9.1 - p321
    // Measures relative to 1.1V Reference, any value above will be 1024
    uint16_t wADC;

    // Set the internal reference and mux.
    ADMUX = 0b11000000; // REFS1 = 1; REFS0 = 1; MUX3:0 = 0b0000
                      // Reference = 1.1V, Measure ADC0
    ADMUX |= adc; // Select ADC

    ADCSRA |= _BV(ADEN);  // enable the ADC
    delay(20);             // wait for voltages to become stable.
    ADCSRA |= _BV(ADSC);  // Start the ADC
    while (bit_is_set(ADCSRA, ADSC)); // Wait for conversion to finish.
    wADC = ADCW;
    // wADC / 1024 * 1.1 == ADC0 Voltage
    // wADC / 1024 * 1.1 * (VBAT/ADC0V) == VBAT
    //return wADC ? wADC / 1024.0d * 1.1d: -1;
    //return wADC ? wADC / 1024.0d * 1.1d * (12.76/0.818555): -1;
    return wADC ? (((wADC / 1024.0d) * 1.1d) * multiplier) + offset : -1;
}
