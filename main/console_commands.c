#include "console_commands.h"
#include <argtable3/argtable3.h>
#include <esp_console.h>
#include <esp_log.h>
#include "mcp4725.h"
#include "ads1115.h"

static const char *TAG = "ConsoleCommands";

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

static struct {
    struct arg_int *channel;
    struct arg_end *end;
} get_ads1115_args;

int get_ads1115_value(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&get_ads1115_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, get_ads1115_args.end, argv[0]);
        return 1;
    }
    int channel = get_ads1115_args.channel->ival[0];
    if (channel < 0 || channel > 3) {
        printf("Error: ADC channel must be between 0 and 3\n");
        return 1;
    }
    uint16_t value = ads1115_read_adc(channel);
    printf("ADS1115 Channel %d ADC value: %d\n", channel, value);
    return 0;
}

void register_get_ads1115() {
    get_ads1115_args.channel = arg_int1(NULL, NULL, "<channel>", "ADC channel (0-3)");
    get_ads1115_args.end = arg_end(2);
    const esp_console_cmd_t cmd = {
        .command = "get_ads1115",
        .help = "Get the ADS1115 ADC value",
        .hint = NULL,
        .func = &get_ads1115_value,
        .argtable = &get_ads1115_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "get_ads1115 command registered successfully");
}
