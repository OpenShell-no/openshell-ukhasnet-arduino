#include "rfm69.h"
#include "../utilities/timer.h"
#include "../utilities/uart.h"
#include "../utilities/util.h"

#include <ukhasnet-rfm69-config.h>
#include <ukhasnet-rfm69.h>

#include "../config.h"
#define SERIAL0
// TODO: fixme

int16_t lastrssi = 0;

#ifdef ESP8266
  int rfm69_reset_pin = 15;
  int rfm69_chipselect_pin = 0;
#else
  int rfm69_reset_pin = 9;      // ATMEGA328PB PB1=9
  int rfm69_chipselect_pin = 8; // ATmega328PB PB0=8
#endif


long _freq;
uint8_t freqbuf[3];

void rfm69_reset() {
  /*
    pinMode(rfm69_reset_pin, OUTPUT); // FIXME: reimplement
    digitalWrite(rfm69_reset_pin, HIGH);
    delay(100);
    digitalWrite(rfm69_reset_pin, LOW);
*/
  //  spi_set_chipselect(rfm69_chipselect_pin);

    while(rf69_init() != RFM_OK) {
        delay(100);
    }
}

void rfm69_set_frequency(float freqMHz) {
    _freq = (long)(((freqMHz + rfm_freq_trim) * 1000000) / 61.04f); // 32MHz / 2^19 = 61.04 Hz
    freqbuf[0] = (_freq >> 16) & 0xff;
    freqbuf[1] = (_freq >> 8) & 0xff;
    freqbuf[2] = _freq & 0xff;

#ifdef SERIAL0
    serial0_print("Setting frequency to: ");
    serial0_print(tostring(freqMHz));
    serial0_print("MHz = 0x");
    serial0_println(tostring(_freq, HEX));
    serial0_flush();

    _rf69_burst_write(RFM69_REG_07_FRF_MSB, freqbuf, 3);

    // FIXME: frequency read back out does not include MSB
    _rf69_burst_read(RFM69_REG_07_FRF_MSB, freqbuf, 3);
    _freq = (freqbuf[0] << 16) | (freqbuf[1] << 8) | freqbuf[2];

    serial0_print("Frequency was set to: ");
    serial0_print(tostring((float)_freq / 1000000 * 61.04f));
    serial0_print("MHz = 0x");
    serial0_println(tostring(_freq, HEX));
    serial0_flush();
#endif
}



void send_rfm69() {
    if (powersave) {
        rfm_txpower = 10;
    } else {
        rfm_txpower = 20;
    }

    rf69_send(databuf, dataptr, rfm_txpower);

    if (powersave) {
        rf69_set_mode(RFM69_MODE_SLEEP);
    } else {
        rf69_set_mode(RFM69_MODE_RX);
    }
}

void dump_rfm69_registers() {
#ifdef SERIAL0
    rfm_reg_t result;

    _rf69_read(RFM69_REG_01_OPMODE, &result);
    serial0_print("REG_01_OPMODE: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_02_DATA_MODUL, &result);
    serial0_print("REG_02_DATA_MODUL: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_03_BITRATE_MSB, &result);
    serial0_print("REG_03_BITRATE_MSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_04_BITRATE_LSB, &result);
    serial0_print("REG_04_BITRATE_LSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_05_FDEV_MSB, &result);
    serial0_print("REG_05_FDEV_MSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_06_FDEV_LSB, &result);
    serial0_print("REG_06_FDEV_LSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_07_FRF_MSB, &result);
    serial0_print("REG_07_FRF_MSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_08_FRF_MID, &result);
    serial0_print("REG_08_FRF_MID: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_09_FRF_LSB, &result);
    serial0_print("REG_09_FRF_LSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0A_OSC1, &result);
    serial0_print("REG_0A_OSC1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0B_AFC_CTRL, &result);
    serial0_print("REG_0B_AFC_CTRL: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0D_LISTEN1, &result);
    serial0_print("REG_0D_LISTEN1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0E_LISTEN2, &result);
    serial0_print("REG_0E_LISTEN2: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0F_LISTEN3, &result);
    serial0_print("REG_0F_LISTEN3: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_10_VERSION, &result);
    serial0_print("REG_10_VERSION: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_11_PA_LEVEL, &result);
    serial0_print("REG_11_PA_LEVEL: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_12_PA_RAMP, &result);
    serial0_print("REG_12_PA_RAMP: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_13_OCP, &result);
    serial0_print("REG_13_OCP: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_18_LNA, &result);
    serial0_print("REG_18_LNA: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_19_RX_BW, &result);
    serial0_print("REG_19_RX_BW: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1A_AFC_BW, &result);
    serial0_print("REG_1A_AFC_BW: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1B_OOK_PEAK, &result);
    serial0_print("REG_1B_OOK_PEAK: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1C_OOK_AVG, &result);
    serial0_print("REG_1C_OOK_AVG: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1D_OOF_FIX, &result);
    serial0_print("REG_1D_OOF_FIX: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1E_AFC_FEI, &result);
    serial0_print("REG_1E_AFC_FEI: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1F_AFC_MSB, &result);
    serial0_print("REG_1F_AFC_MSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_20_AFC_LSB, &result);
    serial0_print("REG_20_AFC_LSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_21_FEI_MSB, &result);
    serial0_print("REG_21_FEI_MSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_22_FEI_LSB, &result);
    serial0_print("REG_22_FEI_LSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_23_RSSI_CONFIG, &result);
    serial0_print("REG_23_RSSI_CONFIG: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_24_RSSI_VALUE, &result);
    serial0_print("REG_24_RSSI_VALUE: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_25_DIO_MAPPING1, &result);
    serial0_print("REG_25_DIO_MAPPING1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_26_DIO_MAPPING2, &result);
    serial0_print("REG_26_DIO_MAPPING2: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_27_IRQ_FLAGS1, &result);
    serial0_print("REG_27_IRQ_FLAGS1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_28_IRQ_FLAGS2, &result);
    serial0_print("REG_28_IRQ_FLAGS2: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_29_RSSI_THRESHOLD, &result);
    serial0_print("REG_29_RSSI_THRESHOLD: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2A_RX_TIMEOUT1, &result);
    serial0_print("REG_2A_RX_TIMEOUT1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2B_RX_TIMEOUT2, &result);
    serial0_print("REG_2B_RX_TIMEOUT2: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2C_PREAMBLE_MSB, &result);
    serial0_print("REG_2C_PREAMBLE_MSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2D_PREAMBLE_LSB, &result);
    serial0_print("REG_2D_PREAMBLE_LSB: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2E_SYNC_CONFIG, &result);
    serial0_print("REG_2E_SYNC_CONFIG: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2F_SYNCVALUE1, &result);
    serial0_print("REG_2F_SYNCVALUE1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_30_SYNCVALUE2, &result);
    serial0_print("REG_30_SYNCVALUE2: 0x");
    serial0_println(tostring(result, HEX));

    /* Sync values 1-8 go here */
    _rf69_read(RFM69_REG_37_PACKET_CONFIG1, &result);
    serial0_print("REG_37_PACKET_CONFIG1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_38_PAYLOAD_LENGTH, &result);
    serial0_print("REG_38_PAYLOAD_LENGTH: 0x");
    serial0_println(tostring(result, HEX));

    /* Node address, broadcast address go here */
    _rf69_read(RFM69_REG_3B_AUTOMODES, &result);
    serial0_print("REG_3B_AUTOMODES: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_3C_FIFO_THRESHOLD, &result);
    serial0_print("REG_3C_FIFO_THRESHOLD: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_3D_PACKET_CONFIG2, &result);
    serial0_print("REG_3D_PACKET_CONFIG2: 0x");
    serial0_println(tostring(result, HEX));

    /* AES Key 1-16 go here */
    _rf69_read(RFM69_REG_4E_TEMP1, &result);
    serial0_print("REG_4E_TEMP1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_4F_TEMP2, &result);
    serial0_print("REG_4F_TEMP2: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_58_TEST_LNA, &result);
    serial0_print("REG_58_TEST_LNA: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_5A_TEST_PA1, &result);
    serial0_print("REG_5A_TEST_PA1: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_5C_TEST_PA2, &result);
    serial0_print("REG_5C_TEST_PA2: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_6F_TEST_DAGC, &result);
    serial0_print("REG_6F_TEST_DAGC: 0x");
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_71_TEST_AFC, &result);
    serial0_print("REG_71_TEST_AFC: 0x");
    serial0_println(tostring(result, HEX));

    #endif
}
