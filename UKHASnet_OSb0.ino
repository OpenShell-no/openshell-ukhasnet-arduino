//#define ONEWIRE
#define RFM69
#define SERIALDEBUG

//#ifdef ESP8266    // ESP8266 based platform
//#ifdef AVR        // AVR based platform

#ifdef RFM69
#include <UKHASnetRFM69-config.h>
#include <UKHASnetRFM69.h>
byte rfm_txpower = 20;
float rfm_freq_trim = 0.068f;
int16_t lastrssi;
#define RFMRESET_PORT PORTB
#define RFMRESET_DDR DDRB
#define RFMRESET_PIN _BV(1)
#endif
#ifdef ONEWIRE
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

char NODE_NAME[9] = "OSTEST"; // null-terminated string, max 8 bytes, A-z0-9
uint8_t NODE_NAME_LEN = strlen(NODE_NAME);
char HOPS = '9'; // '0'-'9'
uint16_t BROADCAST_INTERVAL = 15;

const byte BUFFERSIZE = 128;
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

byte cpu_div = 1;

void CPU_8MHz() {
    /*
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000000; // Clock divider: 1
  cpu_div = 1;
  sei();
  
  // USART0 - http://wormfood.net/avrbaudcalc.php?postbitrate=9600&postclock=1&u2xmode=1
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x0067; // 9600bps
  */
}

void CPU_4MHz() {
    /*
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000001; // Clock divider: 2
  cpu_div = 2;
  sei();
  
  // USART0
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x0033; // 9600bps
  */
}

void CPU_2MHz() {
    /*
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000010; // Clock divider: 4
  cpu_div = 4;
  sei();
  
  // USART0
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x0019; // 9600bps
  */
}

void CPU_1MHz() {
    /*
  cli();
  CLKPR = 0b10000000;
  CLKPR = 0b00000011; // Clock divider: 8
  cpu_div = 8;
  sei();
  
  // USART0
  UCSR0A = UCSR0A | 0b00000010; // Enable double speed
  UBRR0 = 0x000C; // 9600bps
  */
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

byte databuf[BUFFERSIZE];
byte dataptr = 0;
unsigned long packet_count = 0;
byte sequence = 0;

/* ------------------------------------------------------------------------- */
// TODO: make all functions respect BUFFERSIZE.

void resetData() {
  dataptr = 0;
}

// Add \0 terminated string, excluding \0
void addString(char *value) {
    int i = 0;
    while (value[i]) {
        if (dataptr < BUFFERSIZE) {
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
    if (dataptr < BUFFERSIZE) {
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

#ifdef RFM69
void rfm69_reset() {
    RFMRESET_DDR  |= RFMRESET_PIN;
    RFMRESET_PORT |= RFMRESET_PIN;
    delay(100);
    RFMRESET_PORT &= ~(RFMRESET_PIN);
    
    while(rf69_init() != RFM_OK) {
        delay(100);
    }
}

uint8_t freqbuf[3];
long _freq;
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
    
    _rf69_burst_read(RFM69_REG_07_FRF_MSB, freqbuf, 3);
    _freq = (freqbuf[0] << 16) | (freqbuf[1] << 8) | freqbuf[2];
    
    Serial.print(F("Frequency was set to: "));
    Serial.print((float)_freq / 1000000 * 61.04f, DEC);
    Serial.print(F("MHz = 0x"));
    Serial.println(_freq, HEX);
    Serial.flush();
#endif
}
#endif

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

#ifdef RFM69
    rfm69_reset();
    
    for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
        Serial.print("Setting ");
        Serial.print(CONFIG[i][0], HEX);
        Serial.print(" = ");
        Serial.println(CONFIG[i][1], HEX);
    }
    
    rf69_set_mode(RFM69_MODE_RX);
    //rf69_SetLnaMode(RF_TESTLNA_SENSITIVE); // NotImplemented
    
#ifdef HAVE_HWSERIAL0
    Serial.println(F("Radio started."));
    
    dump_rfm69_registers();
    
    Serial.flush();
    rfm69_set_frequency(869.5f);
#endif
#endif

#ifdef HAVE_HWSERIAL0
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

double readVCC() {
    voltage = getVCCVoltage();
    
#ifdef RFM69
    if (voltage < 2.5) {
        rfm_txpower = 10;
    } else {
        rfm_txpower = 20;
    }
#endif
    return voltage;
}

void sendOwn() {
    resetData();
    
    addByte(HOPS);
    addByte(sequence+97);
    
    addByte('V');
    addFloat(readVCC());
    //addByte(',');
    //addFloat(getBatteryVoltage());
    
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


#ifdef RFM69
    addByte('R');
    addFloat((lastrssi / 2.0f) * -1);
#endif    
    
    switch (sequence) {
        case 1: // Location
            break;
        case 2: // Mode
            addString("Z0");
            break;
        case 3: // Comment
            addString(":no sleep");
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
}

uint8_t path_start, path_end;
bool has_repeated;

//rfm_status_t rf69_receive(rfm_reg_t* buf, rfm_reg_t* len, int16_t* lastrssi,
//        bool* rfm_packet_waiting);


typedef enum packet_source_t { SOURCE_UNKNOWN, SOURCE_SELF, SOURCE_SERIAL, SOURCE_WIFI, SOURCE_LAN, SOURCE_RFM, SOURCE_NRF24 } packet_source_t;

bool packet_received;
packet_source_t packet_source;

void handleUKHASNETPacket() {
    Serial.println("handleUKHASNETPacket");
    path_start = 0;
    path_end = 0;
    has_repeated = false;
    for (uint8_t i=0; i<dataptr; i++) {
        if (databuf[i] == '[' || databuf[i] == ',' || databuf[i] == ']') {
            if (path_start && (i - path_start == NODE_NAME_LEN) && !has_repeated) {
                has_repeated = true;
                for (uint8_t j=0; j<NODE_NAME_LEN; j++) {
                    if (databuf[path_start+j] != NODE_NAME[j]) {
                        has_repeated = false;
                    }
                }
            }
            path_start = i + 1;
        }
        if (databuf[i] == ']') {
            path_end = i;
        }
    }
    if (!has_repeated and --databuf[0] >= '0') {
        dataptr = path_end;
        addByte(',');
        addCharArray(NODE_NAME, NODE_NAME_LEN);
        addByte(']');
        send();
    }
}

uint8_t c_find(uint8_t start, char sep, uint8_t count) {
    uint8_t pos = start;
    for (uint8_t i=0; i < count; i++) {
        while (databuf[pos++] != sep) {}
    }
    return pos;
}

uint8_t c_find(uint8_t start, char sep) {
    return c_find(start, sep, 1);
}

uint8_t c_find(char sep, uint8_t count) {
    return c_find(0, sep, count);
}

uint8_t c_find(char sep) {
    return c_find(0, sep, 1);
}

bool s_cmp(char* a, char* b, uint8_t count) {
    for (uint8_t i=0;i<count;i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

bool s_cmp(char* a, char* b) {
    uint8_t i=0;
    while (a[i] != NULL or b[i] != NULL) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

uint8_t s_sub(char* source, char* target, uint8_t start, uint8_t end) {
    uint8_t i;
    for (i=start; i<end; i++) {
        target[i] = source[i];
    }
    target[++i] = NULL;
    return end - start;
}

uint8_t s_sub(char* source, char* target, uint8_t end) {
    return s_sub(source, target, 0, end);
}

uint8_t _gpspos;
uint8_t _gpsbuf[17];
void handleGPSString() {
    if (s_cmp((char*)databuf, "$GPGSA")) {
        _gpspos = c_find(',', 2);
    }
}

void handlePacket() {
    Serial.print("handlePacket ");
    Serial.write(databuf[0]);
    Serial.write(databuf[1]);
    //Serial.write(databuf[dataptr]);
    Serial.println();
    if (databuf[0] >= '0' and
        databuf[0] <= '9' and
        databuf[1] >= 'a' and
        databuf[1] <= 'z') {
        handleUKHASNETPacket();
    } else if (databuf[0] == '$' and databuf[1] == 'G' and databuf[2] == 'P') {
        if (packet_source == SOURCE_SERIAL) {
            handleGPSString();
        }
    }
}

void handleRX() {
#ifdef RFM69
    rf69_receive(databuf, &dataptr, &lastrssi, &packet_received);
    if (packet_received) {
        packet_source = SOURCE_RFM;
        Serial.println(packet_received);
        handlePacket();
    }
#endif
#ifdef SERIALDEBUG
    if (Serial.available()) {
        dataptr = Serial.readBytesUntil('+n', databuf, BUFFERSIZE);
        packet_source = SOURCE_SERIAL;
        handlePacket();
    }
#endif
}

/* ------------------------------------------------------------------------- */
const unsigned long MAXULONG = 0xffffffff;

unsigned long now;
unsigned long getTimeSince(unsigned long ___start) {
    unsigned long interval;
    now = millis();
    if (___start > now) {
        interval = MAXULONG - ___start + now;
    } else {
        interval = now - ___start;
    }
    return interval;
}
/* ------------------------------------------------------------------------- */

unsigned long timer_sendown = 0; //millis();

void loop() {
    handleRX();
    
    if (getTimeSince(timer_sendown) >= (BROADCAST_INTERVAL * 1000)) {
        timer_sendown = millis();
        sendOwn();
    }
    
    
//  sleep(1000);
//    sleep(60000);
}

void send() {
#ifdef HAVE_HWSERIAL0
    for (int i=0;i<dataptr;i++) {
        Serial.write(databuf[i]);
    }
    Serial.write("\r\n");
    Serial.flush();
#endif
#ifdef RFM69
    send_rfm69();
#endif
}

#ifdef RFM69
void send_rfm69() {
    rf69_send(databuf, dataptr, rfm_txpower);
}

#ifdef HAVE_HWSERIAL0
void dump_rfm69_registers() {
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

}

#endif
#endif
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
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)  || defined (__AVR_ATmega328PB__) 
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
  return (wADC - 244.31d) / 1.22d;
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
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328PB__)
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
