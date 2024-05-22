#ifndef RPM_H
#define RPM_H

#include <stdint.h>

#define RPM_INPUT_GPIO GPIO_NUM_4  // Define the GPIO pin connected to the RPM sensor

void rpm_init();
uint32_t get_rpm();

#endif // RPM_H