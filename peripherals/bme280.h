#include <stdint.h>

#ifndef PERIPHERALS_BME280_H
#define PERIPHERALS_BME280_H

void bme280_init();

void bme280_sample();

double bme280_temperature();
double bme280_pressure();
double bme280_humidity();


/* ------------------------------------------------------------------------- */
void bme280_cs_assert();
void bme280_cs_deassert();
uint8_t bme280_read_register(uint8_t reg);
void bme280_write_register(uint8_t reg, uint8_t val);

void bme280_read_calibration();
double bme280_compensate_T_double(int32_t adc_T);
double bme280_compensate_P_double(int32_t adc_P);
double bme280_compensate_H_double(int32_t adc_H);

#endif
