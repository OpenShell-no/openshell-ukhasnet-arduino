#include "bme280.h"
#include <avr/io.h>
#include <stdint.h>
#include "../utilities/spi.h"
#include "../utilities/uart.h"
#include "../utilities/util.h"
#include "../utilities/timer.h"
#include "../config.h"

union {
  struct {
    uint8_t press_msb;
    uint8_t press_lsb;
    uint8_t press_xlsb;
    uint8_t temp_msb;
    uint8_t temp_lsb;
    uint8_t temp_xlsb;
    uint8_t hum_msb;
    uint8_t hum_lsb;
  } data;
  uint8_t raw[8];
} _readings;

struct {
  uint16_t dig_T1:16;
  int16_t dig_T2:16;
  int16_t dig_T3:16;

  uint16_t dig_P1:16;
  int16_t dig_P2:16;
  int16_t dig_P3:16;
  int16_t dig_P4:16;
  int16_t dig_P5:16;
  int16_t dig_P6:16;
  int16_t dig_P7:16;
  int16_t dig_P8:16;
  int16_t dig_P9:16;

  uint8_t dig_H1:8;
  int16_t dig_H2:16;
  uint8_t dig_H3:8;
  int16_t dig_H4:12;
  int16_t dig_H5:12;
  int8_t  dig_H6:8;
} bme280_calibration;

void bme280_init() {
  bme280_write_register(0xE0, 0xB6); // Reset
  serial0_print(F("BMP280 ID: "));
  serial0_println(tostring(bme280_read_register(0xD0), HEX));
  bme280_read_calibration();
}

void bme280_read_calibration() {
  bme280_calibration.dig_T1 = ((uint16_t)bme280_read_register(0x89) << 8) | bme280_read_register(0x88);
  bme280_calibration.dig_T2 = ((int16_t)bme280_read_register(0x8B) << 8) | bme280_read_register(0x8A);
  bme280_calibration.dig_T3 = ((int16_t)bme280_read_register(0x8D) << 8) | bme280_read_register(0x8C);

  bme280_calibration.dig_P1 = ((uint16_t)bme280_read_register(0x8F) << 8) | bme280_read_register(0x8E);
  bme280_calibration.dig_P2 = ((int16_t)bme280_read_register(0x91) << 8) | bme280_read_register(0x90);
  bme280_calibration.dig_P3 = ((int16_t)bme280_read_register(0x93) << 8) | bme280_read_register(0x92);
  bme280_calibration.dig_P4 = ((int16_t)bme280_read_register(0x95) << 8) | bme280_read_register(0x94);
  bme280_calibration.dig_P5 = ((int16_t)bme280_read_register(0x97) << 8) | bme280_read_register(0x96);
  bme280_calibration.dig_P6 = ((int16_t)bme280_read_register(0x99) << 8) | bme280_read_register(0x98);
  bme280_calibration.dig_P7 = ((int16_t)bme280_read_register(0x9B) << 8) | bme280_read_register(0x9A);
  bme280_calibration.dig_P8 = ((int16_t)bme280_read_register(0x9D) << 8) | bme280_read_register(0x9C);
  bme280_calibration.dig_P9 = ((int16_t)bme280_read_register(0x9F) << 8) | bme280_read_register(0x9E);

  bme280_calibration.dig_H1 = bme280_read_register(0xA1);
  bme280_calibration.dig_H2 = ((int16_t)bme280_read_register(0xE2) << 8) | bme280_read_register(0xE1);
  bme280_calibration.dig_H3 = bme280_read_register(0xE3);
  bme280_calibration.dig_H4 = ((int16_t)bme280_read_register(0xE4) << 4) | (bme280_read_register(0xE5) & 0x0F);
  bme280_calibration.dig_H5 = ((int16_t)bme280_read_register(0xE6) << 4) | ((bme280_read_register(0xE5) >> 4) & 0x0F);
  bme280_calibration.dig_H6 = (int8_t)bme280_read_register(0xE7);
}

double _temperature=0;
double _pressure=0;
double _humidity=0;

unsigned long __timeoutstart;

void bme280_sample() {
  serial0_println(F("DBG:bme280_sample"));
  //0xF7 to 0xFE (temperature, pressure and humidity)

  // 4 filter coeff
  bme280_write_register(0xF5, 2<<2);
  /* 16x oversampling, forced mode. */
  // ctrl_hum(osrs_h(5)) 16x oversampling
  bme280_write_register(0xF2, 5);
  // ctrl_meas(osrs_t(5), osrs_p(5), mode(1))
  bme280_write_register(0xF4, (5<<5) | (5<<2) | 0x01);

  __timeoutstart = millis();
  while ((bme280_read_register(0xF3) & (_BV(3) | _BV(1))) == 0) {
      if (getTimeSince(__timeoutstart) >= 800) {
          serial0_println(F("ERR:disabling BME280"));
          bme280_cfg.enabled = false;
          return;
      }
  } // Wait for conversion to start
  while ((bme280_read_register(0xF3) & (_BV(3) | _BV(1))) != 0) {} // Wait for conversion to finish


  bme280_cs_assert();
  spi0_exchange(0xF7 | 0x80); // Select register
  for (uint8_t i=0;i<8;i++) {
    _readings.raw[i] = spi0_exchange();
  }
  bme280_cs_deassert();
  _temperature = bme280_compensate_T_double((((int32_t)_readings.data.temp_msb << 12) | ((int32_t)_readings.data.temp_lsb << 4) | ((_readings.data.temp_xlsb >> 4) & 0x0F)));
  _pressure    = bme280_compensate_P_double((((int32_t)_readings.data.press_msb << 12) | ((int32_t)_readings.data.press_lsb << 4) | ((_readings.data.press_xlsb >> 4) & 0x0F)));
  _humidity = bme280_compensate_H_double(((int32_t)_readings.data.hum_msb << 8) | (_readings.data.hum_lsb));
}

double bme280_temperature() {
  return _temperature;
}

double bme280_pressure() {
  return _pressure;
}

double bme280_humidity() {
  return _humidity;
}

/* ------------------------------------------------------------------------- */
void bme280_cs_assert() {
  DDRB  |= _BV(2);
  spi0_init(8000000);
  PORTB &= ~(_BV(2));
}

void bme280_cs_deassert() {
  PORTB |= _BV(2);
}

uint8_t bme280_read_register(uint8_t reg) {
  bme280_cs_assert();

  spi0_exchange(reg | 0x80); // Select register
  uint8_t res = spi0_exchange(); // Read value

  bme280_cs_deassert();

  return res;
}

void bme280_write_register(uint8_t reg, uint8_t val) {
  bme280_cs_assert();

  spi0_exchange(reg & 0x7F); // Select register
  spi0_exchange(val); // Write value

  bme280_cs_deassert();
}

// Returns temperature in DegC, double precision. Output value of "51.23" equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t t_fine;
double bme280_compensate_T_double(int32_t adc_T) {
    double var1, var2, T;
    var1 = (((double)adc_T)/16384.0 - ((double)bme280_calibration.dig_T1)/1024.0) * ((double)bme280_calibration.dig_T2);
    var2 = ((((double)adc_T)/131072.0 - ((double)bme280_calibration.dig_T1)/8192.0)
      * (((double)adc_T)/131072.0 - ((double) bme280_calibration.dig_T1)/8192.0)) * ((double)bme280_calibration.dig_T3);
    t_fine = (int32_t)(var1 + var2);
    T = (var1 + var2) / 5120.0;
    return T;
}
// Returns pressure in Pa as double. Output value of "96386.2" equals 96386.2 Pa = 963.862 hPa
double bme280_compensate_P_double(int32_t adc_P) {
    double var1, var2, p;
    var1 = ((double)t_fine/2.0) - 64000.0;
    var2 = var1 * var1 * ((double)bme280_calibration.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)bme280_calibration.dig_P5) * 2.0;
    var2 = (var2/4.0)+(((double)bme280_calibration.dig_P4) * 65536.0);
    var1 = (((double)bme280_calibration.dig_P3) * var1 * var1 / 524288.0 + ((double)bme280_calibration.dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0)*((double)bme280_calibration.dig_P1);
    if (var1 == 0.0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)bme280_calibration.dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)bme280_calibration.dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)bme280_calibration.dig_P7)) / 16.0;
    return p;
}
// Returns humidity in %rH as as double. Output value of "46.332" represents 46.332 %rH
double bme280_compensate_H_double(int32_t adc_H) {
    double var_H;
    var_H = (((double)t_fine) - 76800.0);
    var_H = (adc_H - (((double)bme280_calibration.dig_H4) * 64.0 + ((double)bme280_calibration.dig_H5) / 16384.0 * var_H))
      * (((double)bme280_calibration.dig_H2) / 65536.0 * (1.0 + ((double)bme280_calibration.dig_H6) / 67108864.0 * var_H * (1.0 + ((double)bme280_calibration.dig_H3) / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - ((double)bme280_calibration.dig_H1) * var_H / 524288.0);

    if (var_H > 100.0) {
        var_H = 100.0;
    } else if (var_H < 0.0) {
        var_H = 0.0;
    }
    return var_H;
}
