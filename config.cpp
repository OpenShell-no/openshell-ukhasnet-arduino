#include "config.h"

typedef uint8_t byte;
typedef uint16_t word;


/* TODO Config options */
char node_name[9] = "OSTEST"; // null-terminated string, max 8 bytes, A-z0-9
uint8_t node_name_len = strlen(node_name);
char hops = '9'; // '0'-'9'

uint16_t broadcast_interval = 120;

uint8_t rfm_txpower = 20;
float rfm_freq = 869.5f;
float rfm_freq_trim = 0.068f;

float latitude  = NAN;
float longitude = NAN;
float altitude  = NAN;

double vsense_offset = 0.74; // Seems like it depends on current usage. Jumps to 0.76V
double vsense_mult = 15.227;

bool vbat_enabled = false;
int    vbat_pin    = 0; // Analog pin A0
double vbat_offset = 0.023;
double vbat_mult   = 11.0;

bool vpanel_enabled = false;
int    vpanel_pin    = 1; // Analog pin A1
double vpanel_offset = 0.330;
double vpanel_mult   = 11.1;

bool   powersave = true; // Allways assume powersave on boot.
double powersave_treshold = 3.0; // Treshold voltage in volts.

bme280_config_t bme280_cfg = {
  true,
  {true, 5},
  {true, 5},
  {true, 5}
};
