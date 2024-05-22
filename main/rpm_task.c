#include "rpm.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>

extern SemaphoreHandle_t rpm_semaphore;

static const char *TAG = "RPM_Task";

void rpm_task(void *arg) {
    while (1) {
        if (xSemaphoreTake(rpm_semaphore, portMAX_DELAY) == pdTRUE) {
            uint32_t pulses = get_rpm();
            // Calculate RPM from pulses
            // Using the formula: RPM = Frequency * 60 / 32
            // Frequency [Hz] = pulses per second
            uint32_t rpm = (pulses * 60) / 32;

            ESP_LOGI(TAG, "Motor RPM: %" PRIu32, rpm);
        }
    }
}
