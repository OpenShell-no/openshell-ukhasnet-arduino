#include <math.h>

#include "dht.h"
#include "../utilities/timer.h"
#include "../utilities/uart.h"

const uint8_t dht_pin = 7;

const uint16_t _maxcycles = 1000;
uint32_t expectPulse(bool level) {
  // From https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.cpp
  uint32_t count = 0;
  // On AVR platforms use direct GPIO port access as it's much faster and better
  // for catching pulses that are 10's of microseconds in length:
  uint8_t portState = level ? _BV(dht_pin) : 0;
  while ((PIND & _BV(dht_pin)) == portState) {
    if (count++ >= _maxcycles) {
      return 0; // Exceeded timeout, fail.
    }
  }

  return count;
}

bool dht_lastresult;
uint8_t _dht_data[5];
bool dht_sample() {
  serial0_println(F("DBG:dht_sample"));
  _dht_data[0] = _dht_data[1] = _dht_data[2] = _dht_data[3] = _dht_data[4] = 0;
  //getTimeSince();
  PORTD |= _BV(dht_pin); // +
  DDRD |= _BV(dht_pin); // O
  delay(1);
  PORTD &= ~(_BV(dht_pin)); // -
  delay(25);
  PORTD |= _BV(dht_pin); // +
  DDRD &= ~(_BV(dht_pin)); // I

  while ((PIND >> dht_pin) & 1 == 1) {}

  uint32_t cycles[80];

  cli();
  expectPulse(0);
  expectPulse(1);
  for (uint8_t i=0;i<80;) {
    cycles[i++] = expectPulse(0);
    cycles[i++] = expectPulse(1);
  }
  sei();

  for (int i=0; i<40; ++i) {
    uint32_t lowCycles  = cycles[2*i];
    uint32_t highCycles = cycles[2*i+1];
    if ((lowCycles == 0) || (highCycles == 0)) { // timeout
      dht_lastresult = false;
      return dht_lastresult;
    }
    _dht_data[i/8] <<= 1;
    // Now compare the low and high cycle times to see if the bit is a 0 or 1.
    if (highCycles > lowCycles) {
      // High cycles are greater than 50us low cycle count, must be a 1.
      _dht_data[i/8] |= 1;
    }
    // Else high cycles are less than (or equal to, a weird case) the 50us low
    // cycle count so this must be a zero.  Nothing needs to be changed in the
    // stored data.
  }
  if (_dht_data[4] == ((_dht_data[0] + _dht_data[1] + _dht_data[2] + _dht_data[3]) & 0xFF)) {
    dht_lastresult = true;
    return dht_lastresult;
  } else {
    dht_lastresult = false;
    return dht_lastresult;
  }
}

double dht_temperature() {
  double f = NAN;
  if (dht_lastresult) {
    f = _dht_data[2] & 0x7F;
    f *= 256;
    f += _dht_data[3];
    f *= 0.1;
    if (_dht_data[2] & 0x80) {
      f *= -1;
    }
  }
  return f;
}

double dht_humidity() {
  double f = NAN;
  if (dht_lastresult) {
    f = _dht_data[0];
    f *= 256;
    f += _dht_data[1];
    f *= 0.1;
  }
  return f;
}
