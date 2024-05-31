#ifndef RPM_H
#define RPM_H

#include <stdint.h>

// Define the GPIO pin connected to the RPM sensor
#define RPM_INPUT_GPIO GPIO_NUM_4

// Function to initialize the RPM sensor
void rpm_init();

// Function to get the RPM value
uint32_t get_rpm();

#endif // RPM_H