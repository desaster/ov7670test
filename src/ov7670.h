#ifndef __OV7670_H 
#define __OV7670_H

#include "type.h"
#include "ov7670reg.h"
#include "i2c.h"

#define OV7670_ADDR     0x42

#define QQVGA_HEIGHT 120
#define QQVGA_WIDTH 160

extern uint8_t qqvgaframe1[QQVGA_HEIGHT * QQVGA_WIDTH];
extern __DATA(RAM2) uint8_t qqvgaframe2[QQVGA_HEIGHT * QQVGA_WIDTH];

uint32_t ov7670_set(uint8_t addr, uint8_t val);
uint8_t ov7670_get(uint8_t addr);
void ov7670_init(void);
void ov7670_readframe(void);
void ov7670_clear_buffers(void);
void ov7670_check_missing(void);

#endif

/* vim: set et sw=4: */
