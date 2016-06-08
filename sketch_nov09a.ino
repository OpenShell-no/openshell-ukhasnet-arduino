#define ONEWIRE

#include <UKHASnetRFM69-config.h>
#include <UKHASnetRFM69.h>
#ifdef ONEWIRE
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

char NODE_NAME[9] = "OSb0"; // null-terminated string, max 8 bytes, A-z0-9
char HOPS = '9'; // '0'-'9'
byte txpower = 20;

const byte PAYLOADSIZE = 64;

double vsense_offset = 0.74d; // Seems like it depends on current usage. Jumps to 0.76V
double vsense_mult = 15.227d;


// const uint32_t baudrate = 9600;
// The code in this sketch assumes clock speed of 8MHz (eg. the internal oscilator)
#ifdef ONEWIRE
const int OWPIN = 9; // 1-wire bus connected on pin 9
const int DSRES = 12; // 12-bit temperature resolution

OneWire onewire(OWPIN);
DallasTemperature dstemp(&onewire);
DeviceAddress dsaddr;
#endif



/* ------------------------------------------------------------------------- */

byte cpu_div = 8;

void CPU_8MHz() {
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000000; // Clock divider: 1
  cpu_div = 1;
  sei();
  
  // USART0 - http://wormfood.net/avrbaudcalc.php?postbitrate=9600&postclock=1&u2xmode=1
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x0067; // 9600bps
}

void CPU_4MHz() {
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000001; // Clock divider: 2
  cpu_div = 2;
  sei();
  
  // USART0
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x0033; // 9600bps
}

void CPU_2MHz() {
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000010; // Clock divider: 4
  cpu_div = 4;
  sei();
  
  // USART0
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x0019; // 9600bps
}

void CPU_1MHz() {
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000011; // Clock divider: 8
  cpu_div = 8;
  sei();
  
  // USART0
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x000C; // 9600bps
}

void setCPUDIV(byte div) {
    switch (div) {
        case 1:
            CPU_8MHz();
            break;
        case 2:
            CPU_4MHz();
            break;
        case 4:
            CPU_2MHz();
            break;
        case 8:
            CPU_1MHz();
            break;
    }
}

void sleep(unsigned long d) {
    delay(d / cpu_div);
}

byte databuf[PAYLOADSIZE];
byte dataptr = 0;
unsigned long packet_count = 0;
byte sequence = 0;

/* ------------------------------------------------------------------------- */

void resetData() {
  dataptr = 0;
}

// Add \0 terminated string, excluding \0
void addString(char *value) {
    int i = 0;
    while (value[i]) {
        if (dataptr < PAYLOADSIZE) {
            databuf[dataptr++] = value[i++];
        }
    }
}

char _floatbuf[16];
void addFloat(double value, byte precission = 2, bool strip=true) { 
    dtostrf(value, 1, precission, _floatbuf);
    
    if (precission and strip) {
        byte e;
        for (byte i=0;i<16;i++) {
            if (!_floatbuf[i]) {
                e = i-1;
                break;
            }
        }
        for (byte i=e; i; i--) {
            if (_floatbuf[i] == '0') {
                _floatbuf[i] = 0;
            } else if (_floatbuf[i] == '.') {
                _floatbuf[i] = 0;
                break;
            } else {
                break;
            }
        }
    }
    addString(_floatbuf);
}

void addCharArray(char *value, byte len) {
  /* Add char array to the output data buffer (databuf) */
  for (byte i=0; i<len; i++) {
    if (dataptr < PAYLOADSIZE) {
      databuf[dataptr++] = value[i];
    }
  }
}

void addLong(unsigned long value) { // long, unsigned long
  databuf[dataptr++] = value >> 24 & 0xff;
  databuf[dataptr++] = value >> 16 & 0xff;
  databuf[dataptr++] = value >> 8 & 0xff;
  databuf[dataptr++] = value & 0xff;
}

void addWord(word value) { // word, int, unsigned int
  databuf[dataptr++] = value >> 8 & 0xff;
  databuf[dataptr++] = value & 0xff;
}

void addByte(byte value) { // byte, char, unsigned char
  databuf[dataptr++] = value;
}

/* ------------------------------------------------------------------------- */

void setup() {
    CPU_8MHz();
  
#ifdef HAVE_HWSERIAL0
    Serial.begin(9600);
    Serial.print(F("\nUKHASnet: Oddstr13's atmega328 battery node "));
    Serial.println(NODE_NAME);
    Serial.flush();
#endif

#ifdef ONEWIRE
#ifdef HAVE_HWSERIAL0
    Serial.println(F("Scanning 1-wire bus..."));
#endif
    dstemp.begin();
    dstemp.setResolution(12);
#ifdef HAVE_HWSERIAL0
    Serial.print(F("1-wire devices: "));
    Serial.print(dstemp.getDeviceCount(), DEC);
    Serial.println();
    Serial.print(F("1-wire parasite: ")); 
    Serial.println(dstemp.isParasitePowerMode());
    Serial.flush();
#endif
    if (!dstemp.getAddress(dsaddr, 0)) {
#ifdef HAVE_HWSERIAL0
        Serial.println("WARNING: 1-wire: Unable to find temperature device");
        Serial.flush();
#endif
    }
    
#endif

    while(rf69_init() != RFM_OK) {
        delay(100);
    }
#ifdef HAVE_HWSERIAL0
    Serial.println(F("Radio started."));
    Serial.flush();

    //  UBRR0 = 0x0033;
    CPU_1MHz();
    Serial.println(F("Serial test at 1MHz clock speed."));
    Serial.flush();
    CPU_2MHz();
    Serial.println(F("Serial test at 2MHz clock speed."));
    Serial.flush();
    CPU_4MHz();
    Serial.println(F("Serial test at 4MHz clock speed."));
    Serial.flush();
    CPU_8MHz();
    Serial.println(F("Serial test at 8MHz clock speed."));
    Serial.flush();
#endif
    CPU_1MHz();
}

double voltage = 0;

void loop() {
    voltage = getVCCVoltage();
    
    
    if (voltage < 2.5) {
        txpower = 10;
    } else {
        txpower = 20;
    }
    
    resetData();
    
    addByte(HOPS);
    addByte(sequence+97);
    
    addByte('V');
    addFloat(voltage);
    addByte(',');
    addFloat(getBatteryVoltage());
    
    addByte('T');
    addFloat(getChipTemp());
#ifdef ONEWIRE
    if (voltage > 2.75) {
        double temp = getDSTemp();
        if (temp != 85) {
            addByte(',');
            addFloat(getDSTemp(), 3);
        }
    }
#endif
    
    
    switch (sequence) {
        case 1: // Location
            break;
        case 2: // Mode
            addString("Z1");
            break;
        case 3: // Comment
            addString(":no lp sleep");
            break;
    }
    
    addByte('[');
    addString(NODE_NAME);
    addByte(']');
    
    
    send();
    
    
    sequence++;
    sequence %= 26;
    if (!sequence) { // 'a' should only be sendt on boot.
        sequence++;
    }

//  sleep(1000);
    sleep(60000);
}

void send() {
#ifdef HAVE_HWSERIAL0
    for (int i=0;i<dataptr;i++) {
        Serial.write(databuf[i]);
    }
    Serial.write("\r\n");
    Serial.flush();
#endif
    send_rfm69();
}

void send_rfm69() {
    rf69_send(databuf, dataptr, txpower);
}

/* ------------------------------------------------------------------------- */

double getChipTemp() {
  uint16_t wADC;
  
  // Set the internal reference and mux.
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)
  ADMUX = 0b10001111; // REFS2 = 0; REFS1 = 1; REFS0 = 0; MUX3:0 = 0b1111
                      // Reference = 1.1V, Measure ADC4 (Temperature)
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined (__AVR_ATtiny84__)
  ADMUX = 0b10100010; // REFS1 = 1; REFS0 = 0; MUX5:0 = 0b100010
                      // Reference = 1.1V, Measure ADC8 (Temperature)
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
  ADMUX = 0b11001000; // REFS1 = 1; REFS0 = 1; MUX3:0 = 0b1000
                      // Reference = 1.1V, Measure ADC8 (Temperature)
#else
  #error Unknown CPU type, not implemented.
#endif
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));
  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;
  // The offset of 324.31 could be wrong. It is just an indication.
  return (wADC - 344.31d) / 1.22d;
}

double getVCCVoltage() {
  uint16_t wADC;
  
  // Set the internal reference and mux.
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)
  ADMUX = 0b00001110; // REFS2 = 0; REFS1 = 0; REFS0 = 0; MUX3:0 = 0b1100
                      // Reference = VCC, Measure 1.1V(VBG) bandgap
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined (__AVR_ATtiny84__)
  ADMUX = 0b00100001; // REFS1 = 0; REFS0 = 0; MUX5:0 = 0b100001
                      // Reference = VCC, Measure 1.1V(VBG) bandgap
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
  ADMUX = 0b01001110; // REFS1 = 0; REFS0 = 1; MUX3:0 = 0b1110
                      // Reference = AVCC, Measure 1.1V(VBG) bandgap
#else
  #error Unknown CPU type, not implemented.
#endif
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);             // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  while (bit_is_set(ADCSRA, ADSC)); // Wait for conversion to finish.

  wADC = ADCW;
  return wADC ? (1.1d * 1023) / wADC : -1;
}

double getBatteryVoltage() {
  uint16_t wADC;
  
  // Set the internal reference and mux.
  ADMUX = 0b11000000; // REFS1 = 1; REFS0 = 1; MUX3:0 = 0b0000
                      // Reference = 1.1V, Measure ADC0
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);             // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  while (bit_is_set(ADCSRA, ADSC)); // Wait for conversion to finish.
  wADC = ADCW;
  // wADC / 1024 * 1.1 == ADC0 Voltage
  // wADC / 1024 * 1.1 * (VBAT/ADC0V) == VBAT
  //return wADC ? wADC / 1024.0d * 1.1d: -1;
  //return wADC ? wADC / 1024.0d * 1.1d * (12.76/0.818555): -1;
  return wADC ? (((wADC / 1024.0d) * 1.1d) * vsense_mult) + vsense_offset : -1;
}

#ifdef ONEWIRE
double getDSTemp() {
    byte old_div = cpu_div;
    CPU_8MHz();
    
    dstemp.requestTemperatures();
    
    double temp = dstemp.getTempC(dsaddr);
    
    setCPUDIV(old_div);
    
    return temp;
}
#endif