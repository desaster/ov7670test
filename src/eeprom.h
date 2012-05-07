#ifndef __EEPROM_H 
#define __EEPROM_H

#include "type.h"
#include "i2c.h"

#define EEPROM_ADDR     0xA0

uint32_t eeprom_set(uint16_t addr, uint8_t val);
uint8_t eeprom_get(uint16_t addr);

#endif

/* vim: set et sw=4: */
