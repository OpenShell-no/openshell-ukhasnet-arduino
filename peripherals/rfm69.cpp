#include "rfm69.h"
#include "../utilities/timer.h"

#include <ukhasnet-rfm69-config.h>
#include <ukhasnet-rfm69.h>

#include "../config.h"

uint8_t rfm_txpower = 20;
float rfm_freq_trim = 0.068f;
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
    pinMode(rfm69_reset_pin, OUTPUT);
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

#ifdef HAVE_HWSERIAL0
    Serial.print(F("Setting frequency to: "));
    Serial.print(freqMHz, DEC);
    Serial.print(F("MHz = 0x"));
    Serial.println(_freq, HEX);
    Serial.flush();

    _rf69_burst_write(RFM69_REG_07_FRF_MSB, freqbuf, 3);

    // FIXME: frequency read back out does not include MSB
    _rf69_burst_read(RFM69_REG_07_FRF_MSB, freqbuf, 3);
    _freq = (freqbuf[0] << 16) | (freqbuf[1] << 8) | freqbuf[2];

    Serial.print(F("Frequency was set to: "));
    Serial.print((float)_freq / 1000000 * 61.04f, DEC);
    Serial.print(F("MHz = 0x"));
    Serial.println(_freq, HEX);
    Serial.flush();
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
#ifdef HAVE_HWSERIAL0
    rfm_reg_t result;

    _rf69_read(RFM69_REG_01_OPMODE, &result);
    Serial.print(F("REG_01_OPMODE: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_02_DATA_MODUL, &result);
    Serial.print(F("REG_02_DATA_MODUL: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_03_BITRATE_MSB, &result);
    Serial.print(F("REG_03_BITRATE_MSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_04_BITRATE_LSB, &result);
    Serial.print(F("REG_04_BITRATE_LSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_05_FDEV_MSB, &result);
    Serial.print(F("REG_05_FDEV_MSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_06_FDEV_LSB, &result);
    Serial.print(F("REG_06_FDEV_LSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_07_FRF_MSB, &result);
    Serial.print(F("REG_07_FRF_MSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_08_FRF_MID, &result);
    Serial.print(F("REG_08_FRF_MID: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_09_FRF_LSB, &result);
    Serial.print(F("REG_09_FRF_LSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_0A_OSC1, &result);
    Serial.print(F("REG_0A_OSC1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_0B_AFC_CTRL, &result);
    Serial.print(F("REG_0B_AFC_CTRL: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_0D_LISTEN1, &result);
    Serial.print(F("REG_0D_LISTEN1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_0E_LISTEN2, &result);
    Serial.print(F("REG_0E_LISTEN2: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_0F_LISTEN3, &result);
    Serial.print(F("REG_0F_LISTEN3: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_10_VERSION, &result);
    Serial.print(F("REG_10_VERSION: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_11_PA_LEVEL, &result);
    Serial.print(F("REG_11_PA_LEVEL: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_12_PA_RAMP, &result);
    Serial.print(F("REG_12_PA_RAMP: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_13_OCP, &result);
    Serial.print(F("REG_13_OCP: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_18_LNA, &result);
    Serial.print(F("REG_18_LNA: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_19_RX_BW, &result);
    Serial.print(F("REG_19_RX_BW: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_1A_AFC_BW, &result);
    Serial.print(F("REG_1A_AFC_BW: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_1B_OOK_PEAK, &result);
    Serial.print(F("REG_1B_OOK_PEAK: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_1C_OOK_AVG, &result);
    Serial.print(F("REG_1C_OOK_AVG: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_1D_OOF_FIX, &result);
    Serial.print(F("REG_1D_OOF_FIX: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_1E_AFC_FEI, &result);
    Serial.print(F("REG_1E_AFC_FEI: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_1F_AFC_MSB, &result);
    Serial.print(F("REG_1F_AFC_MSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_20_AFC_LSB, &result);
    Serial.print(F("REG_20_AFC_LSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_21_FEI_MSB, &result);
    Serial.print(F("REG_21_FEI_MSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_22_FEI_LSB, &result);
    Serial.print(F("REG_22_FEI_LSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_23_RSSI_CONFIG, &result);
    Serial.print(F("REG_23_RSSI_CONFIG: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_24_RSSI_VALUE, &result);
    Serial.print(F("REG_24_RSSI_VALUE: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_25_DIO_MAPPING1, &result);
    Serial.print(F("REG_25_DIO_MAPPING1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_26_DIO_MAPPING2, &result);
    Serial.print(F("REG_26_DIO_MAPPING2: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_27_IRQ_FLAGS1, &result);
    Serial.print(F("REG_27_IRQ_FLAGS1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_28_IRQ_FLAGS2, &result);
    Serial.print(F("REG_28_IRQ_FLAGS2: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_29_RSSI_THRESHOLD, &result);
    Serial.print(F("REG_29_RSSI_THRESHOLD: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_2A_RX_TIMEOUT1, &result);
    Serial.print(F("REG_2A_RX_TIMEOUT1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_2B_RX_TIMEOUT2, &result);
    Serial.print(F("REG_2B_RX_TIMEOUT2: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_2C_PREAMBLE_MSB, &result);
    Serial.print(F("REG_2C_PREAMBLE_MSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_2D_PREAMBLE_LSB, &result);
    Serial.print(F("REG_2D_PREAMBLE_LSB: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_2E_SYNC_CONFIG, &result);
    Serial.print(F("REG_2E_SYNC_CONFIG: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_2F_SYNCVALUE1, &result);
    Serial.print(F("REG_2F_SYNCVALUE1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_30_SYNCVALUE2, &result);
    Serial.print(F("REG_30_SYNCVALUE2: 0x"));
    Serial.println(result, HEX);

    /* Sync values 1-8 go here */
    _rf69_read(RFM69_REG_37_PACKET_CONFIG1, &result);
    Serial.print(F("REG_37_PACKET_CONFIG1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_38_PAYLOAD_LENGTH, &result);
    Serial.print(F("REG_38_PAYLOAD_LENGTH: 0x"));
    Serial.println(result, HEX);

    /* Node address, broadcast address go here */
    _rf69_read(RFM69_REG_3B_AUTOMODES, &result);
    Serial.print(F("REG_3B_AUTOMODES: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_3C_FIFO_THRESHOLD, &result);
    Serial.print(F("REG_3C_FIFO_THRESHOLD: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_3D_PACKET_CONFIG2, &result);
    Serial.print(F("REG_3D_PACKET_CONFIG2: 0x"));
    Serial.println(result, HEX);

    /* AES Key 1-16 go here */
    _rf69_read(RFM69_REG_4E_TEMP1, &result);
    Serial.print(F("REG_4E_TEMP1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_4F_TEMP2, &result);
    Serial.print(F("REG_4F_TEMP2: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_58_TEST_LNA, &result);
    Serial.print(F("REG_58_TEST_LNA: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_5A_TEST_PA1, &result);
    Serial.print(F("REG_5A_TEST_PA1: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_5C_TEST_PA2, &result);
    Serial.print(F("REG_5C_TEST_PA2: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_6F_TEST_DAGC, &result);
    Serial.print(F("REG_6F_TEST_DAGC: 0x"));
    Serial.println(result, HEX);

    _rf69_read(RFM69_REG_71_TEST_AFC, &result);
    Serial.print(F("REG_71_TEST_AFC: 0x"));
    Serial.println(result, HEX);

    #endif
}
