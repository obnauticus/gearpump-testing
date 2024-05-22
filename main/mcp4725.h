#ifndef MCP4725_H
#define MCP4725_H

#include <stdint.h>

#define MCP4725_ADDR 0x62  /* MCP4725 device address */

void mcp4725_write_dac(uint16_t value);

#endif // MCP4725_H
