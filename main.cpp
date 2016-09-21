#include "firmware_version.h"
#include "config.h"
//#ifdef ESP8266    // ESP8266 based platform
//#ifdef AVR        // AVR based platform

#include <util/delay.h>
#include <stdlib.h>

#include <avr/io.h>

#include "utilities/util.h"
#include "utilities/buffer.h"
#include "utilities/timer.h"
#include "utilities/uart.h"

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
#include <ukhasnet-rfm69-config.h>
#include <ukhasnet-rfm69.h>
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

unsigned long packet_count = 0;
uint8_t sequence = 0;

void debug() {
  #ifdef SERIALDEBUG
    serial0_print(F("Free RAM: "));
    serial0_println(tostring(freeRam()));
  #endif
}

double voltage = 0; // Last measured voltage.


typedef enum packet_source_t { SOURCE_UNKNOWN, SOURCE_SELF, SOURCE_SERIAL, SOURCE_WIFI, SOURCE_LAN, SOURCE_RFM, SOURCE_NRF24 } packet_source_t;
packet_source_t packet_source;




/* ------------------------------------------------------------------------- */


void send() {
#ifdef SERIAL0
    for (int i=0;i<dataptr;i++) {
        serial0_write(databuf[i]);
    }
    serial0_write("\r\n");
    serial0_flush();
#endif
#ifdef USE_RFM69
    send_rfm69();
#endif
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

/* ------------------------------------------------------------------------- */



void setup() {

#ifdef SERIAL0
    serial0_init(115200);
    serial0_println();

    serial0_print("OpenShell UKHASnet Arduino firmware v");
    serial0_print(firmware_version);
    serial0_print(' ');
    serial0_println(firmware_date);

    serial0_print("Revision: ");
    serial0_print(firmware_commit);
    serial0_print('@');
    serial0_println(firmware_branch);

    serial0_print("Node: ");
    serial0_println(NODE_NAME);

    debug();

    serial0_flush();
#endif

#ifdef USE_ONEWIRE
#ifdef SERIAL0
    serial0_println("Scanning 1-wire bus...");
#endif
    dstemp.begin();
    dstemp.setResolution(12);
#ifdef SERIAL0
    serial0_print("1-wire devices: ");
    serial0_print(dstemp.getDeviceCount(), DEC);
    serial0_println();
    serial0_print("1-wire parasite: ");
    serial0_println(dstemp.isParasitePowerMode());
    serial0_flush();
#endif
    if (!dstemp.getAddress(dsaddr, 0)) {
#ifdef SERIAL0
        serial0_println("WARNING: 1-wire: Unable to find temperature device");
        serial0_flush();
#endif
    }

#endif

#ifdef USE_RFM69
    rfm69_reset();

    for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
    #ifdef SERIAL0
        serial0_print("Setting 0x");
        serial0_print(tostring(CONFIG[i][0], HEX));
        serial0_print(" = 0x");
        serial0_println(tostring(CONFIG[i][1], HEX));
    #endif
    }
    //rf69_SetLnaMode(RF_TESTLNA_SENSITIVE); // NotImplemented

#ifdef SERIAL0
    serial0_println("Radio started.");

    dump_rfm69_registers();

    serial0_flush();
    rfm69_set_frequency(rfm_freq);
#endif
#endif
    sendOwn();
}


uint8_t path_start, path_end;
bool has_repeated;

//rfm_status_t rf69_receive(rfm_reg_t* buf, rfm_reg_t* len, int16_t* lastrssi,
//        bool* rfm_packet_waiting);


// typedef enum packet_source_t { SOURCE_UNKNOWN, SOURCE_SELF, SOURCE_SERIAL, SOURCE_WIFI, SOURCE_LAN, SOURCE_RFM, SOURCE_NRF24 } packet_source_t;

bool packet_received;
// packet_source_t packet_source;

void handleUKHASNETPacket() {
#ifdef SERIAL0
    serial0_println("handleUKHASNETPacket");
#endif
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
        delay(rand() % 601);
        send();
        ukhasnet_repeatcount++;
    }
}


void handlePacket() {
#ifdef SERIAL0
    serial0_print("handlePacket ");
    serial0_write(databuf[0]);
    serial0_write(databuf[1]);
    //serial0_write(databuf[dataptr]);
    serial0_println();
#endif
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
            #ifdef SERIAL0
            serial0_println(packet_received);
            #endif
            handlePacket();
        }
    }
#endif
#ifdef SERIALDEBUG
    if (serial0_available()) {
        dataptr = serial0_readBytesUntil('\n', databuf, BUFFERSIZE);
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

int main() {
  DDRD |= 1<<2; // pinMode(PB2, OUTPUT);
  PORTD |= 1<<2; // digitalWrite(PB2, ON);

  sei(); // Enable interrupts

  start_timer();
  delay(1000);

  setup();
  while (true) {
    PORTD ^= 1<<2;
    loop();
  }
  return 0;
}
