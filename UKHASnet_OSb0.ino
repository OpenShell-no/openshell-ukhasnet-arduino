#define DEF_ONEWIRE
#define DEF_RFM69
#define SERIALDEBUG
#define DEF_DHT

//#ifdef ESP8266    // ESP8266 based platform
//#ifdef AVR        // AVR based platform

#ifdef DEF_RFM69
const bool HAS_RFM69 = true;
#include <UKHASnetRFM69-config.h>
#include <UKHASnetRFM69.h>
byte rfm_txpower = 20;
float rfm_freq_trim = 0.068f;
int16_t lastrssi = 0;
#define RFMRESET_PORT PORTB
#define RFMRESET_DDR DDRB
#define RFMRESET_PIN _BV(1)
#else
const bool HAS_RFM69 = false;
#endif


#ifdef DEF_ONEWIRE
const bool HAS_ONEWIRE = true;
#include <OneWire.h>
#include <DallasTemperature.h>
#else
const bool HAS_ONEWIRE = false;
#endif

#ifdef DEF_DHT
const bool HAS_DHT = true;
#include <DHT.h>
//DHT dht;
#else
const bool HAS_DHT = false;
#endif

#if defined(AVR)
#define DEF_CPUTEMP
#define DEF_CPUVCC
const bool HAS_CPUTEMP = true;
const bool HAS_CPUVCC = true;
#elif defined(ESP8266)
const bool HAS_CPUTEMP = false;
const bool HAS_CPUVCC = false;
#endif

char NODE_NAME[9] = "OSTEST"; // null-terminated string, max 8 bytes, A-z0-9
uint8_t NODE_NAME_LEN = strlen(NODE_NAME);
char HOPS = '9'; // '0'-'9'
uint16_t BROADCAST_INTERVAL = 5;

float LATITUDE  = NAN;
float LONGITUDE = NAN;
float ALTITUDE  = NAN;
/* COMPILETIME SETTINGS
HAS_UKHASNET
HAS_RFM69
HAS_NRF24
HAS_ONEWIRE
HAS_DHT
HAS_ADC
HAS_VBATSENSE
HAS_SERIAL
HAS_GPS
HAS_BMP085
HAS_BME280
HAS_CPUTEMP
*/


/* EEPROM SETTINGS
ukhasnet_enabled        bool
ukhasnet_interval       int

onewire_enabled         bool
onewire_pin             int

dht_enabled             bool
dht_pin                 int

rfm69_enabled           bool
rfm69_chipselect_pin    int
rfm69_reset_pin         int
rfm69_interrupt_pin     int
rfm69_frequency         float
rfm69_frequency_offset  float

nrf24_enabled           bool
nrf24_chipselect_pin    int
nrf24_channel           int

vbatsense_enabled       bool
vbatsense_pin           int
vbatsense_multiplier    float
vbatsense_offset        float

photoresistor_enabled   bool
photoresistor_pin       int

gps_enabled             bool
location_latitude       float
location_longitude      float
location_altitude       float

cputemp_enabled         bool
*/

const byte BUFFERSIZE = 128;
const byte PAYLOADSIZE = 64;

double vsense_offset = 0.74d; // Seems like it depends on current usage. Jumps to 0.76V
double vsense_mult = 15.227d;


// const uint32_t baudrate = 9600;
// The code in this sketch assumes clock speed of 8MHz (eg. the internal oscilator)
#ifdef DEF_ONEWIRE
const int OWPIN = 9; // 1-wire bus connected on pin 9
const int DSRES = 12; // 12-bit temperature resolution

OneWire onewire(OWPIN);
DallasTemperature dstemp(&onewire);
DeviceAddress dsaddr;
#endif


typedef enum packet_source_t { SOURCE_UNKNOWN, SOURCE_SELF, SOURCE_SERIAL, SOURCE_WIFI, SOURCE_LAN, SOURCE_RFM, SOURCE_NRF24 } packet_source_t;
packet_source_t packet_source;


typedef enum gps_lock_t { GPS_LOCK_UNKNOWN, GPS_LOCK_NO, GPS_LOCK_2D, GPS_LOCK_3D } gps_lock_t;
gps_lock_t gps_lock = GPS_LOCK_UNKNOWN;


/* ------------------------------------------------------------------------- */

unsigned long timer_lastgps = 0;
bool timer_lastgps_enabled = false;


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
    for (uint8_t i=0; i<16;i++) {
        if (_floatbuf[i] == 0) {
            break;
        }
        if (_floatbuf[i] >= 'A' and _floatbuf[i] <= 'Z') {
            _floatbuf[i] += 32; // str.tolower()
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

#ifdef DEF_RFM69
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
  
#ifdef HAVE_HWSERIAL0
    Serial.begin(9600);
    Serial.print(F("\nUKHASnet: Oddstr13's atmega328 battery node "));
    Serial.println(NODE_NAME);
    Serial.flush();
#endif

#ifdef DEF_ONEWIRE
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

#ifdef DEF_RFM69
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

#ifdef DEF_RFM69
    sendOwn();
#endif
    
}

double voltage = 0;

double readVCC() {
#ifdef DEF_CPUVCC
    voltage = getVCCVoltage();
#endif
    
#ifdef DEF_RFM69
    if (voltage < 2.5) {
        rfm_txpower = 10;
    } else {
        rfm_txpower = 20;
    }
#endif
    return voltage;
}

void bumpSequence() {
    sequence++;
    sequence %= 26;
    if (!sequence) { // 'a' should only be sendt on boot.
        sequence++;
    }
}

void sendOwn() {
    resetData();
    
    addByte(HOPS);
    addByte(sequence+97);
    
    addByte('V');
    addFloat(readVCC());
    //addByte(',');
    //addFloat(getBatteryVoltage());
    
    if (HAS_CPUTEMP or HAS_ONEWIRE or HAS_DHT) { // and *_enabled
    addByte('T');
#ifdef DEF_CPUTEMP
    addFloat(getChipTemp());
#endif
#ifdef DEF_ONEWIRE
    if (voltage > 2.75) {
        double temp = getDSTemp();
        if (temp != 85) {
            addByte(',');
            addFloat(getDSTemp(), 3);
        }
    }
#endif
    }


#ifdef DEF_RFM69
    if (lastrssi != 0) {
        addByte('R');
        addFloat((float)lastrssi);
    }
#endif    
    
    switch (gps_lock) {
        case GPS_LOCK_2D:
            addByte('L');
            addFloat(LATITUDE, 5);
            addByte(',');
            addFloat(LONGITUDE, 5);
            break;
        case GPS_LOCK_3D:
            addByte('L');
            addFloat(LATITUDE, 5);
            addByte(',');
            addFloat(LONGITUDE, 5);
            addByte(',');
            addFloat(ALTITUDE, 0);
            break;
    }
    
    switch (sequence) {
        case 1: // Location
            break;
        case 2: // Mode
            addString("Z0");
            break;
        /*case 3: // Comment
            addString(":no sleep");
            break;*/
    }
    
    addByte('[');
    addString(NODE_NAME);
    addByte(']');
    
    
    send();
    bumpSequence();
}

void sendPositionStatus() {
    resetData();
    
    addByte(HOPS);
    addByte(sequence+97);
    
    switch (gps_lock) {
        case GPS_LOCK_UNKNOWN:
            addString(":GPS Disconnected");
            break;
        case GPS_LOCK_NO:
            addString(":No GPS lock");
            break;
        case GPS_LOCK_2D:
            addByte('L');
            addFloat(LATITUDE, 5);
            addByte(',');
            addFloat(LONGITUDE, 5);
            addString(":2D GPS Lock");
            break;
        case GPS_LOCK_3D:
            addByte('L');
            addFloat(LATITUDE, 5);
            addByte(',');
            addFloat(LONGITUDE, 5);
            addByte(',');
            addFloat(ALTITUDE, 1);
            addString(":3D GPS Lock");
            break;
    }
    
    addByte('[');
    addString(NODE_NAME);
    addByte(']');
    
    send();
    bumpSequence();
}

uint8_t path_start, path_end;
bool has_repeated;

//rfm_status_t rf69_receive(rfm_reg_t* buf, rfm_reg_t* len, int16_t* lastrssi,
//        bool* rfm_packet_waiting);


// typedef enum packet_source_t { SOURCE_UNKNOWN, SOURCE_SELF, SOURCE_SERIAL, SOURCE_WIFI, SOURCE_LAN, SOURCE_RFM, SOURCE_NRF24 } packet_source_t;

bool packet_received;
// packet_source_t packet_source;

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
        delay(random(0, 600));
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
    while (a[i] != '\0' and b[i] != '\0') {
        if (a[i] != b[i]) {
            return false;
        }
        i++;
    }
    return true;
}

uint8_t s_sub(char* source, char* target, uint8_t start, uint8_t end) {
    uint8_t i;
    for (i=0; i<end-start; i++) {
        target[i] = source[start+i];
    }
    target[++i] = '\0';
    return end - start;
}

uint8_t s_sub(char* source, char* target, uint8_t end) {
    return s_sub(source, target, 0, end);
}

float parse_float(char* buf, uint8_t len) {
    //Serial.println(buf);
    
    float _f_mult = 0.1;
    bool _neg = buf[0] == '-';
    
    for (uint8_t i=0; i<len; i++) {
        if (buf[i] == '.') {
            break;
        }
        if (buf[i] >= '0' and buf[i] <= '9') {
            _f_mult *= 10;
        }
    }
    
    float res = 0;
    
    for (uint8_t i=0; i<len; i++) {
        if (buf[i] >= '0' and buf[i] <= '9') {
            res += (buf[i] - 48) * _f_mult;
            _f_mult /= 10;
        }
    }
    
    if (_neg) {
        res *= -1;
    }
    
    //Serial.println(res);
    return res;
}

float parse_float(char* buf) {
    return parse_float(buf, strlen(buf));
}

uint8_t _gpspos;
float _gpsfloat;
char _gpsbuf[17];
gps_lock_t _gps_oldstatus = GPS_LOCK_UNKNOWN;

void handleGPSString() {
    if (s_cmp((char*)databuf, "$GPGSA")) {
        timer_lastgps = millis();
        timer_lastgps_enabled = true;
        _gpspos = c_find(',', 2);
        s_sub((char*)databuf, _gpsbuf, _gpspos, c_find(_gpspos, ',')-1);
        //Serial.println(_gpsbuf);
        switch (_gpsbuf[0]) {
            case '1':
                gps_lock = GPS_LOCK_NO;
                break;
            case '2':
                gps_lock = GPS_LOCK_2D;
                break;
            case '3':
                gps_lock = GPS_LOCK_3D;
                break;
            default:
                gps_lock = GPS_LOCK_UNKNOWN;
        }
    // $GPGGA,123710.00,6240.76823,N,01001.78175,E,1,06,1.16,613.9,M,40.5,M,,*52
    } else if (s_cmp((char*)databuf, "$GPGGA")) {
        
        timer_lastgps = millis();
        timer_lastgps_enabled = true;
        if (gps_lock == GPS_LOCK_2D or gps_lock == GPS_LOCK_3D) {
            _gpspos = c_find(',', 2);
            LATITUDE = (databuf[_gpspos] - 48) * 10;
            LATITUDE += databuf[_gpspos + 1] - 48;
            s_sub((char*)databuf, _gpsbuf, _gpspos+2, c_find(_gpspos, ',')-1);
            _gpsfloat = parse_float(_gpsbuf);
            LATITUDE += _gpsfloat / 60; // TODO: Handle N/S
            
            _gpspos = c_find(',', 3);
            s_sub((char*)databuf, _gpsbuf, _gpspos, c_find(_gpspos, ',')-1);
            
            if (_gpsbuf[0] == 'S') {
                LATITUDE *= -1;
            }
            Serial.print("Latitude: ");
            Serial.println(LATITUDE);
            
            
            
            _gpspos = c_find(',', 4);
            LONGITUDE = parse_float((char*)&databuf[_gpspos], 3);
            //LONGITUDE = (databuf[_gpspos + 1] - 48) * 10;
            //LONGITUDE += databuf[_gpspos + 2] - 48;
            s_sub((char*)databuf, _gpsbuf, _gpspos+3, c_find(_gpspos, ',')-1);
            _gpsfloat = parse_float(_gpsbuf);
            LONGITUDE += _gpsfloat / 60; // TODO: Handle N/S
            
            _gpspos = c_find(',', 5);
            s_sub((char*)databuf, _gpsbuf, _gpspos, c_find(_gpspos, ',')-1);
            
            if (_gpsbuf[0] == 'W') {
                LONGITUDE *= -1;
            }
            Serial.print("Longitude: ");
            Serial.println(LONGITUDE);
        }
        
        if (gps_lock == GPS_LOCK_3D) {
            _gpspos = c_find(',', 9);
            ALTITUDE = parse_float((char*)&databuf[_gpspos], c_find(_gpspos, ',')-1-_gpspos);
            Serial.print("Altitude: ");
            Serial.println(ALTITUDE);
        }
        
        if (_gps_oldstatus != gps_lock) {
            sendPositionStatus();
        }
        _gps_oldstatus = gps_lock;
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
    } else if (s_cmp((char*)databuf, "$GP")) {
        if (packet_source == SOURCE_SERIAL) {
            handleGPSString();
        }
    }
}

void handleRX() {
#ifdef DEF_RFM69
    rf69_receive(databuf, &dataptr, &lastrssi, &packet_received);
    if (packet_received) {
        packet_source = SOURCE_RFM;
        Serial.println(packet_received);
        handlePacket();
    }
#endif
#ifdef SERIALDEBUG
    if (Serial.available()) {
        dataptr = Serial.readBytesUntil('\n', databuf, BUFFERSIZE);
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
    
    if (timer_lastgps_enabled and getTimeSince(timer_lastgps) >= 15000) {
        gps_lock = GPS_LOCK_UNKNOWN;
        sendPositionStatus();
        timer_lastgps_enabled = false;
    }
    
    if (getTimeSince(timer_sendown) >= (BROADCAST_INTERVAL * 1000)) {
        timer_sendown = millis();
        sendOwn();
    }
}

void send() {
#ifdef HAVE_HWSERIAL0
    for (int i=0;i<dataptr;i++) {
        Serial.write(databuf[i]);
    }
    Serial.write("\r\n");
    Serial.flush();
#endif
#ifdef DEF_RFM69
    send_rfm69();
#endif
}

#ifdef DEF_RFM69
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

#ifdef DEF_CPUTEMP
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
#endif

#ifdef DEF_CPUVCC
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
#endif

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

#ifdef DEF_ONEWIRE
double getDSTemp() {
    dstemp.requestTemperatures();
    
    double temp = dstemp.getTempC(dsaddr);
    
    return temp;
}
#endif
