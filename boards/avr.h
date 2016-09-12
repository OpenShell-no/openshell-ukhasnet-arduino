// TODO: This should probably be avr.h
#ifndef BOARDS_AVR_H
#define BOARDS_AVR_H

double getChipTemp();
double getVCCVoltage();
double readADCVoltage(uint8_t adc, double multiplier, double offset);

void print();
void write(char &buf, uint16_t count);

#endif
