#include "ads1115.h"
#include "i2c_master.h"
#include <driver/i2c.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char *TAG = "ADS1115";

void ads1115_init() {
    uint16_t config = ADS1115_CONFIG_OS_SINGLE    |
                      ADS1115_CONFIG_MUX_SINGLE_0 |
                      ADS1115_CONFIG_GAIN_ONE     |
                      ADS1115_CONFIG_MODE_SINGLE  |
                      ADS1115_CONFIG_DR_1600SPS   |
                      ADS1115_CONFIG_COMP_QUE_DIS;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, ADS1115_REG_CONFIG, true);
    i2c_master_write_byte(cmd, (config >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, config & 0xFF, true);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);
    ESP_LOGI(TAG, "ADS1115 initialized successfully");
}

uint16_t ads1115_read_adc(uint8_t channel) {
    uint16_t config = ADS1115_CONFIG_OS_SINGLE    |
                      ADS1115_CONFIG_GAIN_ONE     |
                      ADS1115_CONFIG_MODE_SINGLE  |
                      ADS1115_CONFIG_DR_1600SPS   |
                      ADS1115_CONFIG_COMP_QUE_DIS;

    switch (channel) {
        case 0:
            config |= ADS1115_CONFIG_MUX_SINGLE_0;
            break;
        case 1:
            config |= ADS1115_CONFIG_MUX_SINGLE_1;
            break;
        case 2:
            config |= ADS1115_CONFIG_MUX_SINGLE_2;
            break;
        case 3:
            config |= ADS1115_CONFIG_MUX_SINGLE_3;
            break;
        default:
            ESP_LOGE(TAG, "Invalid channel: %d", channel);
            return 0;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, ADS1115_REG_CONFIG, true);
    i2c_master_write_byte(cmd, (config >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, config & 0xFF, true);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);

    // Wait for conversion to complete
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t read_buf[2];
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, ADS1115_REG_CONVERSION, true);
    i2c_master_start(cmd); // Repeated start
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, read_buf, sizeof(read_buf), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);

    return (read_buf[0] << 8) | read_buf[1];
}
