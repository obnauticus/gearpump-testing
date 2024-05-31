#include "rpm.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/pulse_cnt.h>
#include <esp_log.h>
#include <sys/time.h>
#include <esp_console.h>
#include <esp_timer.h>

static const char *TAG = "RPM";

volatile uint32_t rpm_value = 0;
int64_t last_time_us = 0;

// PCNT unit and channel
#define PCNT_UNIT PCNT_UNIT_0
#define PCNT_INPUT_SIG_IO RPM_INPUT_GPIO  // Pulse Input GPIO
#define PCNT_H_LIM_VAL 32767
#define PCNT_L_LIM_VAL -32768

pcnt_unit_handle_t pcnt_unit;
pcnt_channel_handle_t pcnt_channel;

void rpm_init() {
    // PCNT unit configuration
    pcnt_unit_config_t unit_config = {
        .high_limit = PCNT_H_LIM_VAL,
        .low_limit = PCNT_L_LIM_VAL,
    };

    // Create a new PCNT unit
    esp_err_t err = pcnt_new_unit(&unit_config, &pcnt_unit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PCNT unit: %s", esp_err_to_name(err));
        return;
    }

    // PCNT channel configuration
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = PCNT_INPUT_SIG_IO,
        .level_gpio_num = -1,  // Not used
    };

    // Create a new PCNT channel
    err = pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PCNT channel: %s", esp_err_to_name(err));
        return;
    }

    // Set PCNT channel actions
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP));

    // Enable the PCNT unit
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));

    // Clear the PCNT unit counter
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

    // Start the PCNT unit
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    ESP_LOGI(TAG, "RPM sensor initialized successfully");

    last_time_us = esp_timer_get_time();
}

uint32_t get_rpm() {
    int64_t current_time_us = esp_timer_get_time();
    float elapsed_time_sec = (current_time_us - last_time_us) / 1000000.0f;

    last_time_us = current_time_us;

    int count;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &count));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

    float rpm = (count / elapsed_time_sec) * 60.0f / 32.0f;

    return (uint32_t)rpm;
}