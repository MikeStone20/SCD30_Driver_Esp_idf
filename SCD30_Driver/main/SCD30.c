#include "SCD30.h"

static int i2c_master_num = 0;
static int timeout = 60000;
static uint8_t checksums[256];

void set_i2c_master_num(uint _i2c_master_num){
	i2c_master_num = _i2c_master_num;
}

void set_timeout(int _timeout){
	timeout = _timeout;
}

static void generate_checksums(){
	uint16_t poly = 0x131;
	for(int i = 0; i < 256; ++i){
		uint8_t currByte = i;
		for(int bit = 0; bit < 8; ++bit){
			if(currByte & 0x80){
				currByte <<= 1;
				currByte ^= poly;
			}else{
				currByte <<= 1;
			}
		}
		checksums[i] = currByte;

	}
}

static uint8_t get_checksums(uint16_t data){

	if (checksums[1] == 0){
		generate_checksums();
	}

	uint8_t msb = data >> 8;
	uint8_t lsb = data & 0x00FF;

	uint8_t crc = checksums[0x00FF ^ msb];
	crc = checksums[crc ^ lsb];

	return crc;
}

static bool validate_checksum(uint8_t msb, uint8_t lsb, uint8_t crc){
	uint16_t data = ((uint16_t)msb << 8) | lsb;
	return get_checksums(data) == crc;
}

static esp_err_t scd30_write(uint16_t command, uint16_t data){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t buffer[5];
	buffer[0] = command >> 8;
	buffer[1] = command & 0x00FF;
	buffer[2] = data >> 8;
	buffer[3] = data & 0x00FF;
	buffer[4] = get_checksums(data);
	esp_err_t status = i2c_master_write_to_device(i2c_master_num, scd30, buffer, sizeof(buffer), timeout / portTICK_PERIOD_MS);
	return status;
}

static int16_t scd30_read(uint16_t command){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t bytes[2];
	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;
	esp_err_t status = i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	uint8_t response[3];
	status = i2c_master_read_from_device(i2c_master_num, scd30, response, sizeof(response), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	if(validate_checksum(response[0], response[1], response[2])){
		return ((uint16_t)response[0] << 8) | response[1];
	}
	return -1;
}

int8_t get_status(void){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t bytes[2];
	uint8_t response[3];
	uint16_t command = GET_STATUS;

	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;

	esp_err_t status = i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);
	status = i2c_master_read_from_device(i2c_master_num, scd30, response, sizeof(response), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	return validate_checksum(response[0], response[1], response[2]) ? response[1] : -1;
}

esp_err_t start_measurement(uint16_t comp){
	if(comp != 0 && (comp < 700 || comp > 1400)){
		return ESP_ERR_INVALID_ARG;
	}
	return scd30_write(START_MEASUREMENT, comp);
}

esp_err_t stop_measurement(void){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t bytes[2];
	uint16_t command = STOP_MEASUREMENT;
	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;
	return i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
}

esp_err_t set_measurement(uint16_t interval){
	if(interval < 2 || interval > 1800){
		return ESP_ERR_INVALID_ARG;
	}
	return scd30_write(SET_MEASUREMENT, interval);
}

void read_measuremeants(float* data){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	if(!get_status()){
		data[0] = -1;
		data[1] = -1;
		data[2] = -1;
		return;
	}

	uint16_t command = READ_MEASUREMENT;
	uint8_t bytes[2];
	uint8_t response[18];

	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;

	esp_err_t status = i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);
	status = i2c_master_read_from_device(i2c_master_num, scd30, response, sizeof(response), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	uint8_t buffer[4];

	for(int i = 0; i < 17; i += 6){
		buffer[0] = response[i];
		buffer[1] = response[i + 1];
		buffer[2] = response[i + 3];
		buffer[3] = response[i + 4];

		if(validate_checksum(buffer[0], buffer[1], response[i + 2]) && validate_checksum(buffer[2], buffer[3], response[i + 5])){
			unsigned int temp = ((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]); 
			data[i / 6] = *(float *)&temp;
		}else{
			data[i / 6] = -1;
		}
	}
}

esp_err_t toggle_asc(uint16_t flag){
	if(flag > 1){
		return ESP_ERR_INVALID_ARG;
	}
	return scd30_write(TOGGLE_ASC, flag);
}

int8_t get_asc_status(void){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t bytes[2];
	uint16_t command = TOGGLE_ASC;
	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;
	esp_err_t status = i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	uint8_t response[3];
	status = i2c_master_read_from_device(i2c_master_num, scd30, response, sizeof(response), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	if(validate_checksum(response[0], response[1], response[2])){
		return response[1];
	}

	return -1;
}

esp_err_t set_frc(uint16_t comp){
	return scd30_write(SET_FORCED_RECALIBRATION, comp);
} 

int16_t get_frc(void){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t bytes[2];
	uint16_t command = SET_FORCED_RECALIBRATION;
	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;
	esp_err_t status = i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	uint8_t response[3];
	status = i2c_master_read_from_device(i2c_master_num, scd30, response, sizeof(response), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	if(validate_checksum(response[0], response[1], response[2])){
		return ((uint16_t)response[0] << 8) | response[1];
	}

	return -1;
}

esp_err_t set_temp_offset(uint16_t offset){
	return scd30_write(SET_TEMP_OFFSET, offset);
}

int16_t get_temp_offset(void){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t bytes[2];
	uint16_t command = SET_TEMP_OFFSET;
	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;
	esp_err_t status = i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	uint8_t response[3];
	status = i2c_master_read_from_device(i2c_master_num, scd30, response, sizeof(response), timeout / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(status);

	if(validate_checksum(response[0], response[1], response[2])){
		return ((uint16_t)response[0] << 8) | response[1];
	}
	return -1;
}

esp_err_t set_altitude_compsensation(uint16_t comp){
	return scd30_write(SET_ALT_COMP, comp);
}

int16_t get_altitude_compensation(void){
	return scd30_read(SET_ALT_COMP);
}

void read_firmware(char *buffer){
	int16_t response = scd30_read(READ_FIRMWARE_V);
	if(response < 0){
		return;
	}

	uint8_t major_version = response >> 8;
	uint8_t minor_version = response & 0x00FF;
	sprintf(buffer, "%d.%d", major_version, minor_version);

}

esp_err_t soft_reset(void){
	uint8_t scd30 = SCD30_SENSOR_ADDR;
	uint8_t bytes[2];
	uint16_t command = SOFT_RESET;
	bytes[0] = command >> 8;
	bytes[1] = command & 0x00FF;
	return i2c_master_write_to_device(i2c_master_num, scd30, bytes, sizeof(bytes), timeout / portTICK_PERIOD_MS);
}