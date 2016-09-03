
typedef enum gps_lock_t { GPS_LOCK_UNKNOWN, GPS_LOCK_NO, GPS_LOCK_2D, GPS_LOCK_3D } gps_lock_t;
gps_lock_t gps_lock = GPS_LOCK_UNKNOWN;


unsigned long timer_lastgps = 0;
bool timer_lastgps_enabled = false;

uint8_t _gpspos;
float _gpsfloat;
char _gpsbuf[17];
gps_lock_t _gps_oldstatus = GPS_LOCK_UNKNOWN;

void handleGPSString();
