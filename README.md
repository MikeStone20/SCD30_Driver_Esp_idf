# Driver for SCD30

My personal take on implementing a driver for the SCD30. Project is meant to be cross-compiled and flashed using esp_idf tool chains. All SCD30 functionalities have been implemented with respect to Sensirion datasheet.

# Additional Context
  - [SCD30 Datasheet](https://sensirion.com/media/documents/4EAF6AF8/61652C3C/Sensirion_CO2_Sensors_SCD30_Datasheet.pdf)
  - [SCD30 Communication Interface](https://sensirion.com/media/documents/D7CEEF4A/6165372F/Sensirion_CO2_Sensors_SCD30_Interface_Description.pdf)
  - [Espressif Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)
  - [Espressif Start Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)

# Unknown ESP_TIMEOUT_ERR
At times when using ***driver/i2c.h*** library there are moments when the underlying ***i2c_master_write_to_device*** function will return an ESP_TIMEOUT_ERR. This seems to occur regardless of what the timeout is set to. Similar issues have been reported with other [sensors](https://github.com/espressif/esp-idf/issues/680) and with other [projects](https://github.com/espressif/esp-idf/issues/8786). It seems that, if writes occur too quickly back to back the sensor bus becomes locked and needs to be reset. I advise adding delays in range [1, 3] seconds between commands.
