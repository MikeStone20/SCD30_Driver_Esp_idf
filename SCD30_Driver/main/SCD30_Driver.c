#include "freertos/FreeRTOS.h"
#include "SCD30.c"

#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO 26
#define I2C_MASTER_SCL_IO 27
#define I2C_MASTER_FREQ_HZ 30000

static const char * SYSTEM_TAG = "SYSTEM_TAG";

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
	vTaskDelay(1000/ portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(start_measurement(0));
	float *scd30_buffer = (float *)malloc(3 * sizeof(float));
	if(scd30_buffer == NULL){
		ESP_LOGI(SYSTEM_TAG, "Failed to allocate SCD30 data buffer. Exiting");
		exit(1);
	}

	scd30_buffer[0] = -1;
	scd30_buffer[1] = -1;
	scd30_buffer[2] = -1;
	vTaskDelay(1000/ portTICK_PERIOD_MS);

	ESP_ERROR_CHECK(set_temp_offset(0));
		vTaskDelay(1000/ portTICK_PERIOD_MS);

	ESP_ERROR_CHECK(set_altitude_compsensation(0));
		vTaskDelay(1000/ portTICK_PERIOD_MS);

	while(1){
		int8_t status = get_status();	

		if(status == 1){
			read_measuremeants(scd30_buffer);
			ESP_LOGI("SCD_READ_STATUS", "Co2:%f, temp:%f, RH:%f", scd30_buffer[0], scd30_buffer[1], scd30_buffer[2]);
		}else if(status == 0){
			ESP_LOGI("SCD_READ_STATUS", "not ready.");
		}else{
			ESP_LOGI("SCD_READ_STATUS", "Corrupted read from get_status");
		}
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	free(scd30_buffer);
}