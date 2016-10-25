#include "onewire.h"


#include "../utilities/uart.h"
#include "../utilities/util.h"

OneWire onewire_obj(onewire_cfg.pin);

uint8_t addr[8];
void onewire_init() {
  serial0_println(F("DBG:onewire_init"));

  onewire_obj.target_search(0x28);
  serial0_println(tostring(onewire_obj.reset()));

  uint8_t i=0;
  while (onewire_obj.search(addr)) {
    serial0_print(F("DBG:onewire addr: "));

    for (uint8_t j=0;j<8;j++) {
      serial0_print(tostring(addr[j], HEX));
      serial0_write(' ');
      onewire_cfg.ds[i].address[j] = addr[j];
    }

    // if device is DS18B20 and crc ok
    if (addr[0] == 0x28 and OneWire::crc8(addr, 7) == addr[7]) {
      serial0_println(F(" OK"));
      onewire_cfg.ds[i].present = true;
    } else {
      serial0_println(F(" ERR"));
      // Search for next device instead, to conserve onewire_config items
      continue;
    }

    if (++i > ONEWIRE_CONFIG_MAX_DEVICES) {
      // Unable to store more devices
      break;
    }
  }

  for (;i<ONEWIRE_CONFIG_MAX_DEVICES;i++) {
    // device indexes not in use
    onewire_cfg.ds[i].present = false;
  }
}

unsigned long ow_sample_start=0;
void onewire_sample(bool wait) {
  serial0_print(F("DBG:onewire_sample("));
  serial0_print(tostring(wait));
  serial0_println(')');

  serial0_println(tostring(onewire_obj.reset()));
  onewire_obj.write(0xCC); // Skip ROM
  onewire_obj.write(0x44); // Convert temperature
  ow_sample_start = millis();
  if (wait) {
    onewire_sample_wait();
  }
}

void onewire_sample_wait() {
  serial0_println(F("DBG:onewire_sample_wait"));
  while (getTimeSince(ow_sample_start) < ONEWIRE_SAMPLE_TIME) {
    delay(10);
  }
}

uint8_t _ow_buf[9];
double onewire_temperature(const uint8_t addr[8]) {
  serial0_println(F("DBG:onewire_temperature"));
  onewire_obj.reset();
  onewire_obj.select(addr);
  onewire_obj.write(0xBE);         // Read Scratchpad

  for (uint8_t i = 0; i < 9; i++) {           // we need 9 bytes
    _ow_buf[i] = onewire_obj.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (_ow_buf[1] << 8) | _ow_buf[0];
  /*if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (_ow_buf[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - _ow_buf[6];
    }
  } else {
  */
    uint8_t cfg = (_ow_buf[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  //}*/
  return (double)raw / 16.0;
}
