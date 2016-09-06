#include "firmware_version.h"
#include "config.h"
//#ifdef ESP8266    // ESP8266 based platform
//#ifdef AVR        // AVR based platform

#include "utilities/util.h"
#include "utilities/buffer.h"

#if defined (AVR)
  #include "boards/avr.h"
#endif

/*
  TODO:40 Implement configuration protocol.
  DOING:0 Refractor code into multiple files.
*/

#ifdef USE_GPS
const bool HAS_GPS = true;
#include "peripherals/gps.h"
#else
const bool HAS_GPS = false;
#endif


#ifdef USE_RFM69
const bool HAS_RFM69 = true;
#include "peripherals/rfm69.h"
#else
const bool HAS_RFM69 = false;
#endif



#ifdef USE_ONEWIRE
const bool HAS_ONEWIRE = true;
#include "peripherals/onewire.h"
#else
const bool HAS_ONEWIRE = false;
#endif

#ifdef USE_DHT
const bool HAS_DHT = true;
#include "peripherals/dht.h"
#else
const bool HAS_DHT = false;
#endif

#if defined(AVR)
#define USE_CPUTEMP
#define USE_CPUVCC
const bool HAS_CPUTEMP = true;
const bool HAS_CPUVCC = true;
#elif defined(ESP8266)
const bool HAS_CPUTEMP = false;
const bool HAS_CPUVCC = false;
#define HAVE_HWSERIAL0
#endif


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
unsigned long ukhasnet_rxcount = 0;
unsigned long ukhasnet_repeatcount = 0;



void debug() {
  #ifdef SERIALDEBUG
    Serial.print(F("Free RAM: "));
    Serial.println(freeRam());
  #endif
}

double voltage = 0; // Last measured voltage.


typedef enum packet_source_t { SOURCE_UNKNOWN, SOURCE_SELF, SOURCE_SERIAL, SOURCE_WIFI, SOURCE_LAN, SOURCE_RFM, SOURCE_NRF24 } packet_source_t;
packet_source_t packet_source;




/* ------------------------------------------------------------------------- */


void send() {
#ifdef HAVE_HWSERIAL0
    for (int i=0;i<dataptr;i++) {
        Serial.write(databuf[i]);
    }
    Serial.write("\r\n");
    Serial.flush();
#endif
#ifdef USE_RFM69
    send_rfm69();
#endif
}


/* ------------------------------------------------------------------------- */



unsigned long packet_count = 0;
byte sequence = 0;

void setup() {

#ifdef HAVE_HWSERIAL0
    Serial.begin(115200);
    Serial.println();

    Serial.print(F("OpenShell UKHASnet Arduino firmware v"));
    Serial.print(firmware_version);
    Serial.print(' ');
    Serial.println(firmware_date);

    Serial.print(F("Revision: "));
    Serial.print(firmware_commit);
    Serial.print('@');
    Serial.println(firmware_branch);

    Serial.print(F("Node: "));
    Serial.println(NODE_NAME);

    debug();

    Serial.flush();
#endif

#ifdef USE_ONEWIRE
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

#ifdef USE_RFM69
    rfm69_reset();

    for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
        Serial.print("Setting ");
        Serial.print(CONFIG[i][0], HEX);
        Serial.print(" = ");
        Serial.println(CONFIG[i][1], HEX);
    }
    //rf69_SetLnaMode(RF_TESTLNA_SENSITIVE); // NotImplemented

#ifdef HAVE_HWSERIAL0
    Serial.println(F("Radio started."));

    dump_rfm69_registers();

    Serial.flush();
    rfm69_set_frequency(869.5f);
#endif
#endif
    sendOwn();
}


double readVCC() {
#ifdef USE_CPUVCC
    voltage = getVCCVoltage();
    if (powersave and (voltage - 0.001d > powersave_treshold)) {
        powersave = false;
    } else if ((not powersave) and (voltage + 0.001d < powersave_treshold)) {
        powersave = true;
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
    debug();

    resetData();

    addByte(HOPS);
    addByte(sequence+97);

    addByte('V');
    addFloat(readVCC());
    if (vbat_enabled or vpanel_enabled) {
        addByte(',');
    }
    if (vbat_enabled) {
        addFloat(readADCVoltage(vbat_pin, vbat_mult, vbat_offset));
    }
    if (vpanel_enabled) {
        addByte(',');
        addFloat(readADCVoltage(vpanel_pin, vpanel_mult, vpanel_offset));

    }
    //addFloat(getBatteryVoltage());

    if (HAS_CPUTEMP or HAS_ONEWIRE or HAS_DHT) { // and *_enabled
    addByte('T');
#ifdef USE_CPUTEMP
    addFloat(getChipTemp());
#endif
#ifdef USE_ONEWIRE
    if (voltage > 2.75) {
        double temp = getDSTemp();
        if (temp != 85) {
            addByte(',');
            addFloat(getDSTemp(), 3);
        }
    }
#endif
    }
    /* Need addString(str(long))
    addByte('C');
    addLong(ukhasnet_rxcount);
    addByte(',');
    addLong(ukhasnet_repeatcount);
    */

    if (powersave) {
        addString("Z1");
    } else {
        addString("Z0");
#ifdef USE_RFM69
        if (lastrssi != 0) {
            addByte('R');
            addFloat((float)lastrssi);
        }
#endif
    }

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

    debug();
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
    ukhasnet_rxcount++;
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
        ukhasnet_repeatcount++;
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
#ifdef USE_RFM69
    if (not powersave) {
        rf69_receive(databuf, &dataptr, &lastrssi, &packet_received);
        if (packet_received) {
            packet_source = SOURCE_RFM;
            Serial.println(packet_received);
            handlePacket();
        }
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

unsigned long timer_sendown = 0; //millis();
unsigned long timer_checkvoltage = 0;
void loop() {
    if (getTimeSince(timer_checkvoltage >= 100)) {
        timer_checkvoltage = millis();
        readVCC();
    }

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

    if (do_sendgpsstatus) {
      sendPositionStatus();
    }
}

/* ------------------------------------------------------------------------- */
