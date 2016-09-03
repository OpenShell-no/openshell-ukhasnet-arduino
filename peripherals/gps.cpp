
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
