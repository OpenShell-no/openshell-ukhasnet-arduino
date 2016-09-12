#include "config.h"

typedef uint8_t byte;
typedef uint16_t word;


/* TODO Config options */
char NODE_NAME[9] = "OSTEST"; // null-terminated string, max 8 bytes, A-z0-9
uint8_t NODE_NAME_LEN = strlen(NODE_NAME);
char HOPS = '9'; // '0'-'9'
uint16_t BROADCAST_INTERVAL = 120;

float LATITUDE  = NAN;
float LONGITUDE = NAN;
float ALTITUDE  = NAN;

double vsense_offset = 0.74; // Seems like it depends on current usage. Jumps to 0.76V
double vsense_mult = 15.227;

bool vbat_enabled = true;
int    vbat_pin    = 0; // Analog pin A0
double vbat_offset = 0.023;
double vbat_mult   = 11.0;

bool vpanel_enabled = true;
int    vpanel_pin    = 1; // Analog pin A1
double vpanel_offset = 0.330;
double vpanel_mult   = 11.1;

bool   powersave = true; // Allways assume powersave on boot.
double powersave_treshold = 3.0; // Treshold voltage in volts.
