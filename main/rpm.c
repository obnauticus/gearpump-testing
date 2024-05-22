#include "rpm.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>

static const char *TAG = "RPM";

static volatile uint32_t rpm_count = 0;
static SemaphoreHandle_t rpm_semaphore = NULL;

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
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE, // Trigger on rising edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << RPM_INPUT_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    gpio_isr_handler_add(RPM_INPUT_GPIO, rpm_isr_handler, NULL);
    ESP_LOGI(TAG, "RPM sensor initialized successfully");
}

uint32_t get_rpm() {
    uint32_t count = rpm_count;
    rpm_count = 0; // Reset counter
    return count; // Return pulses counted in the interval
}
