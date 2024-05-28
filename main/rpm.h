#ifndef RPM_H
#define RPM_H

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Define the GPIO pin connected to the RPM sensor
#define RPM_INPUT_GPIO GPIO_NUM_4

// Semaphore to signal new RPM data
extern SemaphoreHandle_t rpm_semaphore;

// Function to initialize the RPM sensor
void rpm_init();

// Function to get the RPM value
uint32_t get_rpm();

// Declaration of the rpm_task function
void rpm_task(void *arg);

// Declaration of the get_rpm_command function
int get_rpm_command(int argc, char **argv);

#endif // RPM_H