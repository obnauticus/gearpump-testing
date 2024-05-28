#include "mcp4725.h"
#include "i2c_master.h"
#include <driver/i2c.h>
#include <esp_log.h>

static const char *TAG = "MCP4725";

void mcp4725_write_dac(uint16_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (value >> 8) & 0x0F, true)); // Upper data bits and control bits
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, value & 0xFF, true));        // Lower data bits
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to DAC: %s", esp_err_to_name(ret));
    }
    /*} else {
        ESP_LOGE(TAG, "Failed to write to DAC: %s", esp_err_to_name(ret));
    }*/
}
