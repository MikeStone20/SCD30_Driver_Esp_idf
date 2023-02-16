#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "esp_log.h"
#include "driver/i2c.h"

// SCD30 Address Bus

const uint8_t SCD30_SENSOR_ADDR = 0x61;

// SCD30 Commands

#define START_MEASUREMENT  0X0010
#define STOP_MEASUREMENT 0X0104
#define SET_MEASUREMENT 0X4600
#define GET_STATUS 0x0202
#define READ_MEASUREMENT 0x0300
#define TOGGLE_ASC 0x5306
#define SET_FORCED_RECALIBRATION 0x5204
#define SET_TEMP_OFFSET 0x5403
#define SET_ALT_COMP 0x5102
#define READ_FIRMWARE_V 0xD100
#define SOFT_RESET 0xD304

/*
   Gets SCD30 measurement read status

   Returns 0 on read not ready, 1 on read ready, <0 on non ESP_OK status.
*/
int8_t get_status(void);

/*
   Begin sensor measurement with ambient pressure compensation.

   comp: how much ambient pressure to compensate for. Range 0 U [700, 1400].

   Returns esp_err_t code for the result of write.
*/
esp_err_t start_measurement(uint16_t comp);

/*
   Stop sensor measurement.

   Returns esp_err_t code for the result of write.
*/
esp_err_t stop_measurement(void);

/*
   Set measurement interval.

   interval: recording interval in seconds. Default is 2 seconds. Range is
   [2, 1800]

   Returns esp_err_t code for the result of write.
*/
esp_err_t set_measurement(uint16_t interval);

/*
   Read recorded temperature, humidity, and Co2 measurements. 
   -1 for measuremeants indicate that an error occurred while reading from

   data: heap allocated buffer to store measurements in.
*/
void read_measuremeants(float* data);

/*
   Enable/Disable Automatic Self Calibration.

   flag: A flag to enable/disable asc. 1 enables, 0 disables.

*/
esp_err_t toggle_asc(uint16_t flag);

/*
   GetAutomatic Self Calibration.

   Returns asc flags status, or -1 on fail.
*/
int16_t get_asc_status(void);

/*
   Calibrate SCD30 to compensate for reference C02 in system.

   comp: Amount of Co2 in ppm to compensate for.

   Returns esp_err_t code for result of write.
*/
esp_err_t set_frc(uint16_t comp);

/*
   Get current Co2 refernce compensation.

   Returns amount of Co2 in ppm compensated for.
*/
int16_t get_frc(void);

/*
   Offset SCD30 temperature reading.

   offset: Amount of temperature in Celsius to offset.

   Returns esp_err_t code for result of write.
*/
esp_err_t set_temp_offset(uint16_t offset);

/*
   Get current temperature offset amount.

   Returns amount of temperature in Celsius offset by.
*/
int16_t get_temp_offset(void);

/*
   Calibrate SCD30 to compensate for altitude.

   comp: Amount in meters above sea level.

   Returns esp_err_t code for result of write.
*/
esp_err_t set_altitude_compsensation(uint16_t comp);

/*
   Get current altitude above sea level compensated for.

   comp: Amount in meters above sea level compensated for.

   Returns esp_err_t code for result of write.
*/
int16_t get_altitude_compensation(void);

/*
   Get current hardware firmware

   comp: Heap allocated char* to store firmware version.

*/
void read_firmware(char *buffer);

/*
   Reset SCD30 into the same state as initial power on.

   Returns esp_err_t code for result of write.
*/
esp_err_t soft_reset(void);