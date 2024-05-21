#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_vfs_dev.h>
#include <esp_console.h>
#include <nvs_flash.h>
#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>

#define I2C_MASTER_SCL_IO 6       /* GPIO number for I2C master clock */
#define I2C_MASTER_SDA_IO 7       /* GPIO number for I2C master data  */
#define I2C_MASTER_FREQ_HZ 100000  /* I2C master clock frequency */
#define I2C_MASTER_NUM I2C_NUM_0   /* I2C master i2c port number */
#define MCP4725_ADDR 0x62
          /* MCP4725 device address */

#define CONSOLE_UART_NUM UART_NUM_0

static const char *TAG = "MCP4725";

void i2c_master_init() {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C initialized successfully");
}

void mcp4725_write_dac(uint16_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (value >> 8) & 0x0F, true)); // Upper data bits and control bits
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, value & 0xFF, true));        // Lower data bits
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DAC output set to %d", value);
    } else {
        ESP_LOGE(TAG, "Failed to write to DAC: %s", esp_err_to_name(ret));
    }
}

static struct {
    struct arg_int *value;
    struct arg_end *end;
} set_dac_args;

int set_dac_value(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&set_dac_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, set_dac_args.end, argv[0]);
        return 1;
    }
    int value = set_dac_args.value->ival[0];
    if (value < 0 || value > 4095) {
        printf("Error: DAC value must be between 0 and 4095\n");
        return 1;
    }
    mcp4725_write_dac(value);
    return 0;
}

void register_set_dac() {
    set_dac_args.value = arg_int1(NULL, NULL, "<value>", "DAC value (0-4095)");
    set_dac_args.end = arg_end(2);
    const esp_console_cmd_t cmd = {
        .command = "set_dac",
        .help = "Set the DAC value",
        .hint = NULL,
        .func = &set_dac_value,
        .argtable = &set_dac_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "set_dac command registered successfully");
}

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
