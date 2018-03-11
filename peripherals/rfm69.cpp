#include <avr/interrupt.h>

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
volatile uint8_t rfm_interrupt_flags = 0;

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
    serial0_println(F("DBG:rfm69_reset"));
  //  spi_set_chipselect(rfm69_chipselect_pin);
    // FIXME: make configurable
    DDRB |= _BV(1);
    PORTB |= _BV(1);
    delay(10); // Datasheet specifies minimum 100 microseconds
    PORTB &= ~(_BV(1));
    delay(20); // Datasheet specifies minimum 10 milliseconds after power-on, and 5 milliseconds after manual reset.

    while(rf69_init() != RFM_OK) {
        delay(10);
    }
    rfm69_enable_interrupts();
}

void rfm69_set_frequency(float freqMHz) {
    serial0_println(F("DBG:rfm69_set_frequency"));
    _freq = (long)(((freqMHz + rfm69_cfg.frequency_trim) * 1000000) / 61.04f); // 32MHz / 2^19 = 61.04 Hz
    freqbuf[0] = (_freq >> 16) & 0xff;
    freqbuf[1] = (_freq >> 8) & 0xff;
    freqbuf[2] = _freq & 0xff;

#ifdef SERIAL0
    serial0_print(F("Setting frequency to: "));
    serial0_print(tostring(freqMHz));
    serial0_print(F("MHz = 0x"));
    serial0_println(tostring(_freq, HEX));
    serial0_flush();
#endif

    _rf69_burst_write(RFM69_REG_07_FRF_MSB, freqbuf, 3);

    // FIXME: frequency read back out does not include MSB
    _rf69_burst_read(RFM69_REG_07_FRF_MSB, freqbuf, 3);
    _freq = (freqbuf[0] << 16) | (freqbuf[1] << 8) | freqbuf[2];

#ifdef SERIAL0
    serial0_print(F("Frequency was set to: "));
    serial0_print(tostring((float)_freq / 1000000 * 61.04f));
    serial0_print(F("MHz = 0x"));
    serial0_println(tostring(_freq, HEX));
    serial0_flush();
#endif
}

void rfm69_enable_interrupts() {
    _rf69_write(RFM69_REG_25_DIO_MAPPING1, RF_DIOMAPPING1_DIO0_01 | RF_DIOMAPPING1_DIO1_01);

    rfm_interrupt_flags = 0;
    PCICR |= _BV(PCIE2);
    PCMSK2 |= _BV(PCINT20) | _BV(PCINT21);
}

volatile uint8_t _oldport = 0;
volatile uint8_t _portnow = 0;
ISR(PCINT2_vect) {
    _portnow = (PIND & 0b00110000); // PD4=DIO0, PD5=DIO1
    rfm_interrupt_flags |= (_oldport ^ _portnow);
    _oldport = _portnow;
}

void send_rfm69() {
    serial0_println(F("DBG:send_rfm69"));

    if (powersave) {
        rf69_send(databuf, dataptr, rfm69_cfg.txpower_low);
    } else {
        rf69_send(databuf, dataptr, rfm69_cfg.txpower);
    }

    if (powersave | (!rfm69_cfg.listen)) {
        rf69_set_mode(RFM69_MODE_SLEEP);
    } else {
        rf69_set_mode(RFM69_MODE_RX);
    }
}

void dump_rfm69_registers() {
#ifdef SERIAL0
    rfm_reg_t result;

    _rf69_read(RFM69_REG_01_OPMODE, &result);
    serial0_print(F("REG_01_OPMODE: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_02_DATA_MODUL, &result);
    serial0_print(F("REG_02_DATA_MODUL: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_03_BITRATE_MSB, &result);
    serial0_print(F("REG_03_BITRATE_MSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_04_BITRATE_LSB, &result);
    serial0_print(F("REG_04_BITRATE_LSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_05_FDEV_MSB, &result);
    serial0_print(F("REG_05_FDEV_MSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_06_FDEV_LSB, &result);
    serial0_print(F("REG_06_FDEV_LSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_07_FRF_MSB, &result);
    serial0_print(F("REG_07_FRF_MSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_08_FRF_MID, &result);
    serial0_print(F("REG_08_FRF_MID: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_09_FRF_LSB, &result);
    serial0_print(F("REG_09_FRF_LSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0A_OSC1, &result);
    serial0_print(F("REG_0A_OSC1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0B_AFC_CTRL, &result);
    serial0_print(F("REG_0B_AFC_CTRL: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0D_LISTEN1, &result);
    serial0_print(F("REG_0D_LISTEN1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0E_LISTEN2, &result);
    serial0_print(F("REG_0E_LISTEN2: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_0F_LISTEN3, &result);
    serial0_print(F("REG_0F_LISTEN3: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_10_VERSION, &result);
    serial0_print(F("REG_10_VERSION: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_11_PA_LEVEL, &result);
    serial0_print(F("REG_11_PA_LEVEL: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_12_PA_RAMP, &result);
    serial0_print(F("REG_12_PA_RAMP: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_13_OCP, &result);
    serial0_print(F("REG_13_OCP: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_18_LNA, &result);
    serial0_print(F("REG_18_LNA: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_19_RX_BW, &result);
    serial0_print(F("REG_19_RX_BW: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1A_AFC_BW, &result);
    serial0_print(F("REG_1A_AFC_BW: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1B_OOK_PEAK, &result);
    serial0_print(F("REG_1B_OOK_PEAK: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1C_OOK_AVG, &result);
    serial0_print(F("REG_1C_OOK_AVG: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1D_OOF_FIX, &result);
    serial0_print(F("REG_1D_OOF_FIX: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1E_AFC_FEI, &result);
    serial0_print(F("REG_1E_AFC_FEI: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_1F_AFC_MSB, &result);
    serial0_print(F("REG_1F_AFC_MSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_20_AFC_LSB, &result);
    serial0_print(F("REG_20_AFC_LSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_21_FEI_MSB, &result);
    serial0_print(F("REG_21_FEI_MSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_22_FEI_LSB, &result);
    serial0_print(F("REG_22_FEI_LSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_23_RSSI_CONFIG, &result);
    serial0_print(F("REG_23_RSSI_CONFIG: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_24_RSSI_VALUE, &result);
    serial0_print(F("REG_24_RSSI_VALUE: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_25_DIO_MAPPING1, &result);
    serial0_print(F("REG_25_DIO_MAPPING1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_26_DIO_MAPPING2, &result);
    serial0_print(F("REG_26_DIO_MAPPING2: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_27_IRQ_FLAGS1, &result);
    serial0_print(F("REG_27_IRQ_FLAGS1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_28_IRQ_FLAGS2, &result);
    serial0_print(F("REG_28_IRQ_FLAGS2: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_29_RSSI_THRESHOLD, &result);
    serial0_print(F("REG_29_RSSI_THRESHOLD: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2A_RX_TIMEOUT1, &result);
    serial0_print(F("REG_2A_RX_TIMEOUT1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2B_RX_TIMEOUT2, &result);
    serial0_print(F("REG_2B_RX_TIMEOUT2: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2C_PREAMBLE_MSB, &result);
    serial0_print(F("REG_2C_PREAMBLE_MSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2D_PREAMBLE_LSB, &result);
    serial0_print(F("REG_2D_PREAMBLE_LSB: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2E_SYNC_CONFIG, &result);
    serial0_print(F("REG_2E_SYNC_CONFIG: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_2F_SYNCVALUE1, &result);
    serial0_print(F("REG_2F_SYNCVALUE1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_30_SYNCVALUE2, &result);
    serial0_print(F("REG_30_SYNCVALUE2: 0x"));
    serial0_println(tostring(result, HEX));

    /* Sync values 1-8 go here */
    _rf69_read(RFM69_REG_37_PACKET_CONFIG1, &result);
    serial0_print(F("REG_37_PACKET_CONFIG1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_38_PAYLOAD_LENGTH, &result);
    serial0_print(F("REG_38_PAYLOAD_LENGTH: 0x"));
    serial0_println(tostring(result, HEX));

    /* Node address, broadcast address go here */
    _rf69_read(RFM69_REG_3B_AUTOMODES, &result);
    serial0_print(F("REG_3B_AUTOMODES: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_3C_FIFO_THRESHOLD, &result);
    serial0_print(F("REG_3C_FIFO_THRESHOLD: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_3D_PACKET_CONFIG2, &result);
    serial0_print(F("REG_3D_PACKET_CONFIG2: 0x"));
    serial0_println(tostring(result, HEX));

    /* AES Key 1-16 go here */
    _rf69_read(RFM69_REG_4E_TEMP1, &result);
    serial0_print(F("REG_4E_TEMP1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_4F_TEMP2, &result);
    serial0_print(F("REG_4F_TEMP2: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_58_TEST_LNA, &result);
    serial0_print(F("REG_58_TEST_LNA: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_5A_TEST_PA1, &result);
    serial0_print(F("REG_5A_TEST_PA1: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_5C_TEST_PA2, &result);
    serial0_print(F("REG_5C_TEST_PA2: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_6F_TEST_DAGC, &result);
    serial0_print(F("REG_6F_TEST_DAGC: 0x"));
    serial0_println(tostring(result, HEX));

    _rf69_read(RFM69_REG_71_TEST_AFC, &result);
    serial0_print(F("REG_71_TEST_AFC: 0x"));
    serial0_println(tostring(result, HEX));

    #endif
}
