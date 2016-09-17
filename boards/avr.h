// TODO: This should probably be avr.h
#ifndef BOARDS_AVR_H
#define BOARDS_AVR_H

double getChipTemp();
double getVCCVoltage();
double readADCVoltage(uint8_t adc, double multiplier, double offset);

#define SERIAL0

#endif
