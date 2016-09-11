// TODO: This should probably be avr.h
#ifndef boards_avr_h
#define boards_avr_h

double getChipTemp();
double getVCCVoltage();
double readADCVoltage(uint8_t adc, double multiplier, double offset);

void print();
void write(char &buf, uint16_t count);

#endif
