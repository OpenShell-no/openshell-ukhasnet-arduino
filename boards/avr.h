// TODO: This should probably be avr.h

double getChipTemp();
double getVCCVoltage();
double readADCVoltage(uint8_t adc, double multiplier, double offset);

void print();
void write(char &buf, uint16_t count);
