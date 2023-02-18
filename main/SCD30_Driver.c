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

static void wait(int milli){
	vTaskDelay(milli / portTICK_PERIOD_MS);
}

void app_main(void)
{
	esp_err_t esp_status;
	ESP_ERROR_CHECK(i2c_master_init());
	esp_status = start_measurement(0);

	int retries = 5;
	while(esp_status != ESP_OK && retries < 0){
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_status);
		wait(1000);
		esp_status = start_measurement(0);
		--retries;
	}
	// Failed to start SCD30 in 5 seconds.
	if(retries == 0){
		return;
	}

	// Failed to set measurement interval in 5 seconds.
	retries = 5;
	esp_status = set_measurement(5);
	while(esp_status != ESP_OK && retries < 0){
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_status);
		wait(1000);
		esp_status = start_measurement(0);
		--retries;
	}

	// Failed to set measurement interval.
	if(retries == 0){
		return;
	}

	float *scd30_buffer = (float *)malloc(3 * sizeof(float));
	if(scd30_buffer == NULL){
		ESP_LOGI(SYSTEM_TAG, "Failed to allocate SCD30 data buffer. Exiting");
		exit(1);
	}

	scd30_buffer[0] = -1;
	scd30_buffer[1] = -1;
	scd30_buffer[2] = -1;

	// Setting temp offset
	ESP_ERROR_CHECK_WITHOUT_ABORT(set_temp_offset(0));

	// Altitude comp set to 1000m
	ESP_ERROR_CHECK_WITHOUT_ABORT(set_altitude_compsensation(0));

	// Reading config settings
	int8_t asc_status = get_asc_status();
	int16_t frc = get_frc();
	int16_t temp_offset = get_temp_offset();
	int16_t alt_comp = get_altitude_compensation();

	while(1){
		int8_t status = get_status();
		if(status == 1){
			read_measuremeants(scd30_buffer);
			ESP_LOGI(SCD30_TAG, "Current SCD30 Readings:\nCo2:%f temp:%f RH:%f\n", scd30_buffer[0], scd30_buffer[1], scd30_buffer[2]);	
			ESP_LOGI(SCD30_TAG, "Current SCD30 Configs:\nASC:%d FRC:%d Temp_offset:%d Alt_offset:%d\n", asc_status, frc, temp_offset, alt_comp);
		}else if(status == 0){
			ESP_LOGI("SCD_READ_STATUS", "not ready.");
		}else{
			ESP_LOGI("SCD_READ_STATUS", "Unable to retrive SCD30 status.");
		}
		wait(5000);	
	}
	free(scd30_buffer);
}
