#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_vfs_dev.h>
#include <esp_console.h>
#include <nvs_flash.h>
#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>

#include "i2c_master.h"
#include "mcp4725.h"
#include "ads1115.h"
#include "console_commands.h"
#include "rpm.h"

#define CONSOLE_UART_NUM UART_NUM_0

static const char *TAG = "Main";

void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized successfully");

    // Initialize I2C
    i2c_master_init();

    // Initialize ADS1115
    ads1115_init();

    // Run I2C Scanner
    i2c_scanner();

    // Initialize RPM sensor
    rpm_init();

    // Create RPM task
    xTaskCreate(rpm_task, "rpm_task", 2048, NULL, 5, NULL);

    // Initialize UART for console and logging
    ESP_LOGI(TAG, "Initializing UART for console and logging");
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(CONSOLE_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(CONSOLE_UART_NUM);

    // Initialize console
    ESP_LOGI(TAG, "Initializing console");
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    linenoiseSetCompletionCallback(NULL);
    linenoiseHistorySetMaxLen(100);
    linenoiseAllowEmpty(false);

    // Register commands
    register_set_dac();
    register_get_ads1115();
    register_loop_dac();
    register_get_rpm();

    const char* prompt = LOG_COLOR_I "ESPizza> " LOG_RESET_COLOR;

    printf("\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n");

    while (true) {
        char* line = linenoise(prompt);
        if (line == NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));  // Add a small delay to yield to the scheduler
            continue;
        }
        linenoiseHistoryAdd(line);
        int ret = esp_console_run(line, &ret);
        if (ret == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (ret == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (ret != ESP_OK) {
            printf("Command returned error code: 0x%x\n", ret);
        }
        linenoiseFree(line);
        vTaskDelay(pdMS_TO_TICKS(10));  // Add a small delay to yield to the scheduler
    }
}