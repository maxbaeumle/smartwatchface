#ifndef common_h
#define common_h

#include "pebble.h"

#define RECONNECT_KEY 0
#define REQUEST_CALENDAR_KEY 1
#define CLOCK_STYLE_KEY 2
#define CALENDAR_RESPONSE_KEY 3
#define REQUEST_BATTERY_KEY 8
#define BATTERY_RESPONSE_KEY 9

#define CLOCK_STYLE_12H 1
#define CLOCK_STYLE_24H 2

typedef struct {
  uint8_t index;
  char title[21];
  bool has_location;
  char location[21];
  bool all_day;
  char start_date[18];
  int32_t alarms[2];
} Event;

typedef struct {
  uint8_t state;
  int8_t level;
} BatteryStatus;

#endif
