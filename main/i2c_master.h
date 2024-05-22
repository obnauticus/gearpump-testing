#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <stdint.h>

#define I2C_MASTER_SCL_IO 6       /* GPIO number for I2C master clock */
#define I2C_MASTER_SDA_IO 7       /* GPIO number for I2C master data  */
#define I2C_MASTER_FREQ_HZ 100000  /* I2C master clock frequency */
#define I2C_MASTER_NUM I2C_NUM_0   /* I2C master I2C port number */

void i2c_master_init();
void i2c_scanner();

#endif // I2C_MASTER_H
