/*
===============================================================================
 Name        : ov7670.c
 Author      : Upi Tamminen
 Version     : 1.0
 Copyright   : Upi Tamminen (2012)
 Description : ov7670 register access & image reading
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <NXP/crp.h>

#include <stdio.h>

#include "ov7670.h"
#include "ov7670reg.h"
#include "type.h"
#include "i2c.h"
#include "delay.h"

/* use these to check if pin is high */
#define ST_D0 (LPC_GPIO2->FIOPIN & (1 << 0))
#define ST_D1 (LPC_GPIO2->FIOPIN & (1 << 1))
#define ST_D2 (LPC_GPIO2->FIOPIN & (1 << 2))
#define ST_D3 (LPC_GPIO2->FIOPIN & (1 << 3))
#define ST_D4 (LPC_GPIO2->FIOPIN & (1 << 4))
#define ST_D5 (LPC_GPIO2->FIOPIN & (1 << 5))
#define ST_D6 (LPC_GPIO2->FIOPIN & (1 << 6))
#define ST_D7 (LPC_GPIO2->FIOPIN & (1 << 7))

#define ST_VSYNC (LPC_GPIO2->FIOPIN & (1 << 8))
#define ST_HREF (LPC_GPIO2->FIOPIN & (1 << 11))
#define ST_PCLK (LPC_GPIO2->FIOPIN & (1 << 12))

/* due to memory constraints, only read the upper half of the image */
uint8_t qqvgaframe1[QQVGA_HEIGHT * QQVGA_WIDTH]; /* first rgb565 byte */
__DATA(RAM2) uint8_t qqvgaframe2[QQVGA_HEIGHT * QQVGA_WIDTH]; /* second rgb565 byte */

uint32_t ov7670_set(uint8_t addr, uint8_t val)
{
    i2c_clearbuffers();

    I2CWriteLength = 3;
    I2CReadLength = 0;
    I2CMasterBuffer[0] = OV7670_ADDR;   /* i2c address */
    I2CMasterBuffer[1] = addr;          /* key */
    I2CMasterBuffer[2] = val;           /* value */

    return I2CEngine();
}

uint8_t ov7670_get(uint8_t addr)
{
    i2c_clearbuffers();
    I2CWriteLength = 2;
    I2CReadLength = 0;
    I2CMasterBuffer[0] = OV7670_ADDR;   /* i2c address */
    I2CMasterBuffer[1] = addr;          /* key */

    I2CEngine();

    delay(1);

    i2c_clearbuffers();
    I2CWriteLength = 0;
    I2CReadLength = 1;
    I2CMasterBuffer[0] = OV7670_ADDR | RD_BIT;

    while (I2CEngine() == I2CSTATE_SLA_NACK);

    return I2CSlaveBuffer[0];
}

void ov7670_init(void)
{
    printf("Initializing ov7670");

    /* 0.22 used to reset the camera */
    LPC_PINCON->PINSEL1 &= ~(3 << 12); /* set as gpio */
    LPC_GPIO0->FIODIR |= (1 << 22); /* set as output */
    LPC_PINCON->PINMODE1 &= ~(1 << 12); /* no pulldown/up */
    LPC_PINCON->PINMODE1 |= (1 << 13); /* no pulldown/up */

    /* ports D0..D7 go to P2.0..P2.7 */
    LPC_PINCON->PINSEL4 &= ~(0xffff); /* function = gpio */
    LPC_GPIO2->FIODIR &= ~(0xff); /* direction = input */

    /* port 2.8 vsync */
    LPC_PINCON->PINSEL4 &= ~(3 << 16); /* function = gpio */
    LPC_GPIO2->FIODIR &= ~(1 << 8); /* direction = input */

    /* port 2.11 href */
    LPC_PINCON->PINSEL4 &= ~(3 << 22); /* function = gpio */
    LPC_GPIO2->FIODIR &= ~(1 << 11); /* direction = input */

    /* port 2.12 pclk */
    LPC_PINCON->PINSEL4 &= ~(3 << 24); /* function = gpio */
    LPC_GPIO2->FIODIR &= ~(1 << 12); /* direction = input */

    printf("...reset");
    LPC_GPIO0->FIOCLR |= (1 << 22); /* low */
    delay(100);
    LPC_GPIO0->FIOSET |= (1 << 22); /* high */
    delay(100);

    printf("...settings");
    if (ov7670_get(REG_PID) != 0x76) {
        printf("PANIC! REG_PID != 0x76!\n");
        while (1);
    }
    ov7670_set(REG_COM7, 0x80); /* reset to default values */
    ov7670_set(REG_CLKRC, 0x80);
    ov7670_set(REG_COM11, 0x0A);
    ov7670_set(REG_TSLB, 0x04);
    ov7670_set(REG_TSLB, 0x04);
    ov7670_set(REG_COM7, 0x04); /* output format: rgb */

    ov7670_set(REG_RGB444, 0x00); /* disable RGB444 */
    ov7670_set(REG_COM15, 0xD0); /* set RGB565 */

    /* not even sure what all these do, gonna check the oscilloscope and go
     * from there... */
    ov7670_set(REG_HSTART, 0x16);
    ov7670_set(REG_HSTOP, 0x04);
    ov7670_set(REG_HREF, 0x24);
    ov7670_set(REG_VSTART, 0x02);
    ov7670_set(REG_VSTOP, 0x7a);
    ov7670_set(REG_VREF, 0x0a);
    ov7670_set(REG_COM10, 0x02);
    ov7670_set(REG_COM3, 0x04);
    ov7670_set(REG_COM14, 0x1a); // divide by 4
    //ov7670_set(REG_COM14, 0x1b); // divide by 8
    ov7670_set(REG_MVFP, 0x27);
    ov7670_set(0x72, 0x22); // downsample by 4
    //ov7670_set(0x72, 0x33); // downsample by 8
    ov7670_set(0x73, 0xf2); // divide by 4
    //ov7670_set(0x73, 0xf3); // divide by 8

    // test pattern
    //ov7670_set(0x70, 1 << 7);
    //ov7670_set(0x70, 0x0);

    // COLOR SETTING
    ov7670_set(0x4f, 0x80);
    ov7670_set(0x50, 0x80);
    ov7670_set(0x51, 0x00);
    ov7670_set(0x52, 0x22);
    ov7670_set(0x53, 0x5e);
    ov7670_set(0x54, 0x80);
    ov7670_set(0x56, 0x40);
    ov7670_set(0x58, 0x9e);
    ov7670_set(0x59, 0x88);
    ov7670_set(0x5a, 0x88);
    ov7670_set(0x5b, 0x44);
    ov7670_set(0x5c, 0x67);
    ov7670_set(0x5d, 0x49);
    ov7670_set(0x5e, 0x0e);
    ov7670_set(0x69, 0x00);
    ov7670_set(0x6a, 0x40);
    ov7670_set(0x6b, 0x0a);
    ov7670_set(0x6c, 0x0a);
    ov7670_set(0x6d, 0x55);
    ov7670_set(0x6e, 0x11);
    ov7670_set(0x6f, 0x9f);

    ov7670_set(0xb0, 0x84);

    printf("...done.\n");
}

void ov7670_readframe(void)
{
    while (ST_VSYNC); /* wait for the old frame to end */
    while (!ST_VSYNC); /* wait for a new frame to start */

    uint32_t i = 0;

    while (ST_VSYNC) {
        //if (y >= (QQVGA_HEIGHT / 2)) break;
        while (ST_VSYNC && !ST_HREF); /* wait for a line to start */
        if (!ST_VSYNC) break; /* line didn't start, but frame ended */
        while (ST_HREF) { /* wait for a line to end */
            /* first byte */
            while (!ST_PCLK); /* wait for clock to go high */
            /* no time to do anything fancy here! */
            /* this grabs the first 8 bits, rest gets chopped off */
            qqvgaframe1[i] = LPC_GPIO2->FIOPIN;
            while (ST_PCLK); /* wait for clock to go back low */

            /* second byte */
            while (!ST_PCLK); /* wait for clock to go high */
            qqvgaframe2[i] = LPC_GPIO2->FIOPIN;
            while (ST_PCLK); /* wait for clock to go back low */
            i ++;
        }
    }
}

void ov7670_clear_buffers(void)
{
    uint16_t i;

    /* clear the buffers with zero.. a crude method to checking if the
     * buffer is filled later with ov7670_check_missing.
     * DOES NOT WORK IN DARK :D */
    for (i = 0; i < QQVGA_HEIGHT * QQVGA_WIDTH; i ++) {
        qqvgaframe1[i] = 0;
        qqvgaframe2[i] = 0;
    }
}

void ov7670_check_missing(void)
{
    uint32_t m;
    uint16_t i;

    m = 0;
    for (i = 0; i < 19200; i ++) {
        if (qqvgaframe1[i] == 0) m ++;
    }
    printf("Missing pixels in frame 1: %d\n", m);

    m = 0;
    for (i = 0; i < 19200; i ++) {
        if (qqvgaframe2[i] == 0) m ++;
    }
    printf("Missing pixels in frame 2: %d\n", m);
}

/* vim: set et sw=4: */
