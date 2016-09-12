#ifndef peripherals_gps_h
#define peripherals_gps_h

typedef enum gps_lock_t { GPS_LOCK_UNKNOWN, GPS_LOCK_NO, GPS_LOCK_2D, GPS_LOCK_3D } gps_lock_t;
extern gps_lock_t gps_lock;


extern unsigned long timer_lastgps;
extern bool timer_lastgps_enabled;
extern bool do_sendgpsstatus;


void handleGPSString();

#endif
