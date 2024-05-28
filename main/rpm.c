#include "rpm.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <sys/time.h>
#include <esp_console.h>

static const char *TAG = "RPM_Task";

volatile uint32_t rpm_count = 0;
volatile uint32_t rpm_value = 0; // Global variable to store RPM value
SemaphoreHandle_t rpm_semaphore = NULL;
struct timeval last_time;

static void IRAM_ATTR rpm_isr_handler(void* arg) {
    rpm_count++;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(rpm_semaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void rpm_init() {
    rpm_semaphore = xSemaphoreCreateBinary();
    if (rpm_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE, // Trigger on rising edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << RPM_INPUT_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3); // Use a valid interrupt flag
    gpio_isr_handler_add(RPM_INPUT_GPIO, rpm_isr_handler, NULL);
    ESP_LOGI(TAG, "RPM sensor initialized successfully");

    // Initialize the last_time variable
    gettimeofday(&last_time, NULL);
}

uint32_t get_rpm() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    // Calculate elapsed time in seconds
    float elapsed_time = (current_time.tv_sec - last_time.tv_sec) +
                         (current_time.tv_usec - last_time.tv_usec) / 1000000.0f;

    // Save the current time for the next calculation
    last_time = current_time;

    // Read and reset the counter
    uint32_t count = rpm_count;
    rpm_count = 0;

    // Calculate RPM: RPM = (count / elapsed_time) * 60 / pulses_per_rev
    // Assuming 32 pulses per revolution
    float rpm = (count / elapsed_time) * 60.0f / 32.0f;

    return (uint32_t)rpm;
}

int get_rpm_command(int argc, char **argv) {
    ESP_LOGI(TAG, "Current RPM: %" PRIu32, rpm_value);
    printf("Current RPM: %" PRIu32 "\n", rpm_value);
    return 0;
}

void rpm_task(void *arg) {
    while (1) {
        if (xSemaphoreTake(rpm_semaphore, portMAX_DELAY) == pdTRUE) {
            rpm_value = get_rpm();
        }
    }
}