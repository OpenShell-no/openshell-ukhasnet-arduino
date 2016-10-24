#include "onewire.h"


OneWire onewire_obj(10);

uint8_t addr[8];
void onewire_init() {
  onewire_obj.reset_search();

  uint8_t i=0;
  while (ds.search(addr)) {
    for (uint8_t j=0;j<8;j++) {
      onewire_config.ds[i].address[j] = addr[j];
    }

    // FIXME: search for next device instead, to conserve onewire_config items
    // if device is DS18B20 and crc ok
    if (addr[0] == 0x28 and OneWire::crc8(addr, 7) == addr[7]) {
      onewire_config.ds[i].present = true;
    } else {
      onewire_config.ds[i].present = false;
    }

    if (++i < ONEWIRE_CONFIG_MAX_DEVICES) {
      break;
    }
  }

  for (;i<ONEWIRE_CONFIG_MAX_DEVICES;i++) {
    onewire_config.ds[i].present = false;
  }
}

onewire_sample() {
  ds.reset();
  ds.write(0xCC); // Skip ROM
  ds.write(0x44); // Convert temperature
}
