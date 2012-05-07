/*
===============================================================================
 Name        : eeprom.c
 Author      : Upi Tamminen
 Version     : 1.0
 Copyright   : Upi Tamminen (2012)
 Description : i2c calls for the 24lc512 eeprom chip
===============================================================================
*/

#include <stdio.h>

#include "delay.h"
#include "eeprom.h"
#include "type.h"
#include "i2c.h"

uint32_t eeprom_set(uint16_t addr, uint8_t val)
{
    i2c_clearbuffers();

    I2CWriteLength = 4;
    I2CReadLength = 0;
    I2CMasterBuffer[0] = EEPROM_ADDR;           /* EEPROM address */
    I2CMasterBuffer[1] = (addr & 0xF0);         /* key */
    I2CMasterBuffer[2] = (addr & 0x0F);         /* key */
    I2CMasterBuffer[3] = val;                   /* value */

    return I2CEngine();
}

uint8_t eeprom_get(uint16_t addr)
{
    i2c_clearbuffers();

    I2CWriteLength = 3;
    I2CReadLength = 1;
    I2CMasterBuffer[0] = EEPROM_ADDR;   /* EEPROM address */
    I2CMasterBuffer[1] = (addr & 0xF0); /* key */
    I2CMasterBuffer[2] = (addr & 0x0F); /* key */
    I2CMasterBuffer[3] = EEPROM_ADDR | RD_BIT;

    while (I2CEngine() == I2CSTATE_SLA_NACK);

    return I2CSlaveBuffer[0];
}

/* vim: set et sw=4: */
