#include "firmware_version.h"
#include "config.h"
//#ifdef ESP8266    // ESP8266 based platform
//#ifdef AVR        // AVR based platform

#include <util/delay.h>
#include <stdlib.h>

// IO
#include <avr/io.h>
// Watchdog
#include <avr/wdt.h>

#include "utilities/util.h"
#include "utilities/buffer.h"
#include "utilities/timer.h"
#include "utilities/uart.h"
#include "utilities/lowpower.h"

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


#ifdef USE_BME280
const bool HAS_BME280 = true;
#include "peripherals/bme280.h"
#else
const bool HAS_BME280 = false;
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

uint8_t reset_reason = 0;

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
  #ifdef SERIAL0
    serial0_print(F("Free RAM: "));
    serial0_println(tostring(freeRam()));
  #endif
}

double voltage = 0; // Last measured voltage.


typedef enum packet_source_t { SOURCE_UNKNOWN, SOURCE_SELF, SOURCE_SERIAL, SOURCE_WIFI, SOURCE_LAN, SOURCE_RFM, SOURCE_NRF24 } packet_source_t;
packet_source_t packet_source;




/* ------------------------------------------------------------------------- */


void send() {
  serial0_println(F("DBG:send"));
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
    serial0_println(F("DBG:sendPositionStatus"));
    resetData();

    addByte(hops);
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
            addFloat(latitude, 5);
            addByte(',');
            addFloat(longitude, 5);
            addString(":2D GPS Lock");
            break;
        case GPS_LOCK_3D:
            addByte('L');
            addFloat(latitude, 5);
            addByte(',');
            addFloat(longitude, 5);
            addByte(',');
            addFloat(altitude, 1);
            addString(":3D GPS Lock");
            break;
    }

    addByte('[');
    addString(node_name);
    addByte(']');

    send();
    bumpSequence();
}

void sendInitial() {
  serial0_println(F("DBG:sendInitial"));
  //
  debug();

  resetData();

  addByte(hops);
  addByte(sequence+97);

  addByte('V');
  addFloat(readVCC());

  if (powersave | (!rfm69_cfg.listen)) {
      addString("Z1");
  } else {
      addString("Z0");
  }

  addString(F(":reset="));
  addString(reset_tostring());

  addByte('[');
  addString(node_name);
  addByte(']');

  send();
  delay(300);
  send();
  bumpSequence();
}

void sendOwn() {
    serial0_println(F("DBG:sendOwn"));
    debug();

    resetData();

    addByte(hops);
    addByte(sequence+97);

    switch (sequence) {
        case 1: // Location
            if (latitude != NAN or longitude != NAN or altitude != NAN) {
                addByte('L');
                if (latitude != NAN) {
                    addFloat(latitude, 5);
                }
                addByte(',');
                if (longitude != NAN) {
                    addFloat(longitude, 5);
                }
                if (altitude != NAN) {
                    addByte(',');
                    addFloat(altitude, 0);
                }
            }
            break;
        case 2: // Version
            addByte(':');
            addString(F("version="));
            addCharArray(firmware_commit, 7);
            if (firmware_commit_dirty) {
                addByte('+');
            }
            break;
        /*case 3: // Comment
            addString(":no sleep");
            break;*/
        default:
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
    #ifdef USE_ONEWIRE
      if (onewire_cfg.enabled) {
        onewire_sample();
      }
    #endif

    #ifdef USE_BME280
      if (bme280_cfg.enabled) {
        bme280_sample();
      }
    #endif
    #ifdef USE_DHT
      if (dht_cfg.enabled) {
        dht_sample();
      }
    #endif

    if (HAS_CPUTEMP | HAS_BME280) { // and *_enabled | HAS_DHT | HAS_ONEWIRE
      addByte('T');
      #ifdef USE_CPUTEMP
        addFloat(getChipTemp());
      #endif
      #ifdef USE_BME280
        if (bme280_cfg.enabled & bme280_cfg.temperature.enabled) {
          addByte(',');
          addFloat(bme280_temperature(), bme280_cfg.temperature.decimals);
        }
      #endif
      #ifdef USE_DHT
        if (dht_cfg.enabled & dht_cfg.temperature.enabled & dht_lastresult) {
          addByte(',');
          addFloat(dht_temperature(), dht_cfg.temperature.decimals);
        }
      #endif

      #ifdef USE_ONEWIRE
        if (onewire_cfg.enabled) {
          onewire_sample_wait();
          for (uint8_t i=0; i < ONEWIRE_CONFIG_MAX_DEVICES; i++) {
            if (onewire_cfg.ds[i].enabled & onewire_cfg.ds[i].present) {
              addByte(',');
              addFloat(onewire_temperature(onewire_cfg.ds[i].address), onewire_cfg.decimals);
            }
          }
        }
      #endif
    }
    #ifdef USE_BME280
      if (bme280_cfg.enabled & bme280_cfg.pressure.enabled) {
        addByte('P');
        addFloat(bme280_pressure(), bme280_cfg.pressure.decimals);
      }
    #endif
    #if defined(USE_BME280) | defined(USE_DHT)
      #ifdef USE_BME280
        if (bme280_cfg.enabled & bme280_cfg.humidity.enabled) {
          addByte('H');
          addFloat(bme280_humidity(), bme280_cfg.humidity.decimals);
        }
      #endif
      #ifdef USE_DHT
        if (dht_cfg.enabled & dht_cfg.humidity.enabled & dht_lastresult) {
          addByte('H');
          addFloat(dht_humidity(), dht_cfg.humidity.decimals);
        }
      #endif
    #endif
    /* Need addString(str(long))
    addByte('C');
    addLong(ukhasnet_rxcount);
    addByte(',');
    addLong(ukhasnet_repeatcount);
    */

    if (powersave | (!rfm69_cfg.listen)) {
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
            addFloat(latitude, 5);
            addByte(',');
            addFloat(longitude, 5);
            break;
        case GPS_LOCK_3D:
            addByte('L');
            addFloat(latitude, 5);
            addByte(',');
            addFloat(longitude, 5);
            addByte(',');
            addFloat(altitude, 0);
            break;
    }
    } // end packet sequence switch

    addByte('[');
    addString(node_name);
    addByte(']');


    send();
    bumpSequence();

    debug();
}

/* ------------------------------------------------------------------------- */



void setup() {
  serial0_println(F("DBG:setup"));

#ifdef SERIAL0
    serial0_init(115200);
    serial0_println();

    serial0_print(F("OpenShell UKHASnet Arduino firmware v"));
    serial0_print(firmware_version);
    serial0_print(' ');
    serial0_println(firmware_date);

    serial0_print(F("Revision: "));
    serial0_print(firmware_commit);
    serial0_print('@');
    serial0_println(firmware_branch);

    serial0_print(F("Node: "));
    serial0_println(node_name);

    debug();

    serial0_flush();
#endif

#ifdef USE_ONEWIRE
    onewire_init();
#endif
#ifdef USE_BME280
    bme280_init();
#endif

#if 0
//USE_ONEWIRE
#ifdef SERIAL0
    serial0_println(F("Scanning 1-wire bus..."));
#endif
    dstemp.begin();
    dstemp.setResolution(12);
#ifdef SERIAL0
    serial0_print(F("1-wire devices: "));
    serial0_print(dstemp.getDeviceCount(), DEC);
    serial0_println();
    serial0_print(F("1-wire parasite: "));
    serial0_println(dstemp.isParasitePowerMode());
    serial0_flush();
#endif
    if (!dstemp.getAddress(dsaddr, 0)) {
#ifdef SERIAL0
        serial0_println(F("WARNING: 1-wire: Unable to find temperature device"));
        serial0_flush();
#endif
    }

#endif

  #ifdef USE_RFM69
    rfm69_reset();

    #ifdef SERIAL0
      for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
        serial0_print(F("Setting 0x"));
        serial0_print(tostring(CONFIG[i][0], HEX));
        serial0_print(F(" = 0x"));
        serial0_println(tostring(CONFIG[i][1], HEX));
      }
    #endif
    //rf69_SetLnaMode(RF_TESTLNA_SENSITIVE); // NotImplemented

    #ifdef SERIAL0
      serial0_println(F("Radio started."));

      dump_rfm69_registers();

      serial0_flush();
    #endif
    rfm69_set_frequency(rfm69_cfg.frequency);
  #endif
  wdt_reset();
  sendInitial();
  wdt_reset();
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
    serial0_println(F("handleUKHASNETPacket"));
#endif
    ukhasnet_rxcount++;
    path_start = 0;
    path_end = 0;
    has_repeated = false;
    for (uint8_t i=0; i<dataptr; i++) {
        if (databuf[i] == '[' || databuf[i] == ',' || databuf[i] == ']') {
            if (path_start && (i - path_start == node_name_len) && !has_repeated) {
                has_repeated = true;
                for (uint8_t j=0; j<node_name_len; j++) {
                    if (databuf[path_start+j] != node_name[j]) {
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
        addCharArray(node_name, node_name_len);
        addByte(']');
        delay(rand() % 601);
        send();
        ukhasnet_repeatcount++;
    }
}


void handlePacket() {

#ifdef SERIAL0
    serial0_print(F("handlePacket "));
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
  //serial0_println(F("handleRX"));
#ifdef USE_RFM69
    if ((!powersave) & rfm69_cfg.listen) {
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
uint16_t interval_left = 0;
void loop() {
    serial0_flush();
    if (interval_left) {
        interval_left = WDTsleep(interval_left);
    } else {
        interval_left = WDTsleep(broadcast_interval);
    }

    wdt_reset();
    #ifdef WDP3
        wdt_enable(WDTO_8S);
    #else
        wdt_enable(WDTO_2S);
    #endif

    serial0_print("Woken up with ");
    serial0_print(tostring(interval_left));
    serial0_println("s left.");
/*
    if (getTimeSince(timer_checkvoltage) >= 100) {
        timer_checkvoltage = millis();
        readVCC();
    }
*/
    handleRX();

/*
    if (timer_lastgps_enabled and getTimeSince(timer_lastgps) >= 15000) {
        gps_lock = GPS_LOCK_UNKNOWN;
        sendPositionStatus();
        timer_lastgps_enabled = false;
    }
*/
    if (!interval_left) {
        wdt_reset();
        delay(20); // ADC was probably off. Wait a little for stabilization.
        sendOwn();
    }

    /*
    if (do_sendgpsstatus) {
      sendPositionStatus();
    }
    */
}

/* ------------------------------------------------------------------------- */

int main() {
    wdt_disable();

    reset_reason = MCUSR;

    #ifdef SERIAL0
        serial0_init(115200);
        serial0_print(F("Reset reasons: "));
        serial0_println(reset_tostring());
        #ifdef WDP3
        serial0_println(F("DBG:Watchdog timeout: 8s"));
        #else
        serial0_println(F("DBG:Watchdog timeout: 2s"));
        #endif
    #endif

    // Heartbeat on PD2, OUTPUT
    DDRD |= _BV(2); // pinMode(PB2, OUTPUT);
    DDRD |= _BV(3);

    PORTD |= _BV(2); // digitalWrite(PB2, ON);

    sei(); // Enable interrupts

    start_timer();

    wdt_reset();
    setup();

    MCUSR &= 0xF0; // Reset reset reason flags.

    serial0_println(F("DBG:Entering main loop."));
    while (true) {
        wdt_reset();
        PORTD ^= _BV(2); // toggle heartbeat LED
        loop();
    }
    return 0;
}
