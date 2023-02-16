#include "freertos/FreeRTOS.h"
#include "SCD30.c"

#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO 26
#define I2C_MASTER_SCL_IO 27
#define I2C_MASTER_FREQ_HZ 30000

static const char* SYSTEM_TAG = "SYSTEM_TAG";
static const char* SCD30_TAG = "SCD_TAG";

static esp_err_t i2c_master_init(void)
{
	int i2c_master_port = I2C_MASTER_NUM;

	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = I2C_MASTER_SDA_IO,
		.scl_io_num = I2C_MASTER_SCL_IO,
		.sda_pullup_en = GPIO_PULLUP_DISABLE,
		.scl_pullup_en = GPIO_PULLUP_DISABLE,
		.master.clk_speed = I2C_MASTER_FREQ_HZ,
	};

	i2c_param_config(i2c_master_port, &conf);
	return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void app_main(void)
{
	ESP_ERROR_CHECK(i2c_master_init());
	vTaskDelay(2000/ portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(start_measurement(0));
	vTaskDelay(2000/ portTICK_PERIOD_MS);
	// Read Data every 5 seconds
	ESP_ERROR_CHECK(set_measurement(5));
	float *scd30_buffer = (float *)malloc(3 * sizeof(float));
	if(scd30_buffer == NULL){
		ESP_LOGI(SYSTEM_TAG, "Failed to allocate SCD30 data buffer. Exiting");
		exit(1);
	}

	scd30_buffer[0] = -1;
	scd30_buffer[1] = -1;
	scd30_buffer[2] = -1;
	//Gives SCD30 chane to turn on to avoid time out error
	vTaskDelay(2000/ portTICK_PERIOD_MS);

	// Temp offset set to 0
	ESP_ERROR_CHECK(set_temp_offset(0));
	vTaskDelay(2000/ portTICK_PERIOD_MS);

	// Altitude comp set to 0
	ESP_ERROR_CHECK(set_altitude_compsensation(0));
	vTaskDelay(2000/ portTICK_PERIOD_MS);

	// Reading config settings
	int8_t asc_status = get_asc_status();
	vTaskDelay(2000/ portTICK_PERIOD_MS);
	int16_t frc = get_frc();
	vTaskDelay(2000/ portTICK_PERIOD_MS);
	int16_t temp_offset = get_temp_offset();
	vTaskDelay(2000/ portTICK_PERIOD_MS);
	int16_t alt_comp = get_altitude_compensation();
	vTaskDelay(2000/ portTICK_PERIOD_MS);

	while(1){
		int8_t status = get_status();
		vTaskDelay(2000/ portTICK_PERIOD_MS);

		if(status == 1){
			read_measuremeants(scd30_buffer);
			ESP_LOGI(SCD30_TAG, "Current SCD30 Readings:\nCo2:%f temp:%f RH:%f\n", scd30_buffer[0], scd30_buffer[1], scd30_buffer[2]);	
			ESP_LOGI(SCD30_TAG, "Current SCD30 Configs:\nASC:%d FRC:%d Temp_offset:%d Alt_offset:%d\n", asc_status, frc, temp_offset, alt_comp);
			// Total Delay is 5.5 seconds if read occurs.
			vTaskDelay(3500 / portTICK_PERIOD_MS);
		}else if(status == 0){
			ESP_LOGI("SCD_READ_STATUS", "not ready.");
		}else{
			ESP_LOGI("SCD_READ_STATUS", "Corrupted read from get_status");
		}
		
		
	}
	free(scd30_buffer);
}