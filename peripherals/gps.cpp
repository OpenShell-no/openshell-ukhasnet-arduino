#include <stdint.h>
#include "../config.h"
#include "../utilities/buffer.h"
#include "../utilities/timer.h"
#include "gps.h"

gps_lock_t gps_lock = GPS_LOCK_UNKNOWN;


unsigned long timer_lastgps = 0;
bool timer_lastgps_enabled = false;
bool do_sendgpsstatus = false;


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
            latitude = (databuf[_gpspos] - 48) * 10;
            latitude += databuf[_gpspos + 1] - 48;
            s_sub((char*)databuf, _gpsbuf, _gpspos+2, c_find(_gpspos, ',')-1);
            _gpsfloat = parse_float(_gpsbuf);
            latitude += _gpsfloat / 60; // TODO: Handle N/S

            _gpspos = c_find(',', 3);
            s_sub((char*)databuf, _gpsbuf, _gpspos, c_find(_gpspos, ',')-1);

            if (_gpsbuf[0] == 'S') {
                latitude *= -1;
            }
            //Serial.print("Latitude: "); // FIXME: Implement printing
            //Serial.println(latitude);



            _gpspos = c_find(',', 4);
            longitude = parse_float((char*)&databuf[_gpspos], 3);
            //longitude = (databuf[_gpspos + 1] - 48) * 10;
            //longitude += databuf[_gpspos + 2] - 48;
            s_sub((char*)databuf, _gpsbuf, _gpspos+3, c_find(_gpspos, ',')-1);
            _gpsfloat = parse_float(_gpsbuf);
            longitude += _gpsfloat / 60; // TODO: Handle N/S

            _gpspos = c_find(',', 5);
            s_sub((char*)databuf, _gpsbuf, _gpspos, c_find(_gpspos, ',')-1);

            if (_gpsbuf[0] == 'W') {
                longitude *= -1;
            }
            //Serial.print("Longitude: "); // FIXME: Implement printing
            //Serial.println(longitude);
        }

        if (gps_lock == GPS_LOCK_3D) {
            _gpspos = c_find(',', 9);
            altitude = parse_float((char*)&databuf[_gpspos], c_find(_gpspos, ',')-1-_gpspos);
            //Serial.print("Altitude: "); // FIXME: Implement printing
            //Serial.println(altitude);
        }

        if (_gps_oldstatus != gps_lock) {
            do_sendgpsstatus = true;
        }
        _gps_oldstatus = gps_lock;
    }
}
