/*
===============================================================================
 Name        : main.c
 Author      : Upi Tamminen
 Version     : 1.0
 Copyright   : Upi Tamminen (2012)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <NXP/crp.h>

// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
__CRP const unsigned int CRP_WORD = CRP_NO_CRP ;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "delay.h"
#include "ov7670.h"
#include "type.h"
#include "i2c.h"
#include "uart0.h"

void init_board(void)
{
    /* clkout of 10mhz on 1.27 */
    LPC_PINCON->PINSEL3 &=~(3<<22);
    LPC_PINCON->PINSEL3 |= (1<<22);
    LPC_SC->CLKOUTCFG = (1<<8)|(14<<4); //enable and divide by 12

    UART0_Init(921600);

    if (I2CInit((uint32_t) I2CMASTER) == 0) {
        printf("Fatal error!\n");
        while (1);
    }
}

int main(void)
{
    uint8_t addr1, addr2; /* i2c addresses */
    uint16_t x, y;
    char buf[128]; /* temporary string buffer for various stuff */

    /* uart stuff */
    char rcvbuf[32], c;
    uint8_t rcvbufpos = 0;

    init_board();
    ov7670_init();

    printf("Camtest says hi!\n");
    printf("System clock: [%d]\n", SystemCoreClock);

    UART0_PrintString("Camtest says hi!\r\n");
    while (1) {
        c = UART0_Getchar();
        if (c == EOF) {
            continue;
        } else if ((c >= 32) && (c <= 126)) {
            rcvbuf[rcvbufpos++] = c;
        } else if (c == 13) {
            rcvbuf[rcvbufpos++] = 0;
            rcvbufpos = 0;
            if (strcmp(rcvbuf, "getimage") == 0) {
                ov7670_readframe();
                UART0_PrintString("OK\r\n");
            } else if (strlen(rcvbuf) >= 9 &&
                    strncmp(rcvbuf, "getline ", 8) == 0) {
                y = atoi(rcvbuf + 8);
                for (x = 0; x < 160; x ++) {
                    UART0_Sendchar(qqvgaframe1[x + (y * 160)]);
                    UART0_Sendchar(qqvgaframe2[x + (y * 160)]);
                }
            } else if (strlen(rcvbuf) == 9 &&
                    strncmp(rcvbuf, "regr 0x", 7) == 0) {
                addr1 = strtoul(rcvbuf + 7, NULL, 16);
                sprintf(buf, "0x%.2x 0x%.2x\r\n", addr1, ov7670_get(addr1));
                printf("%s", buf);
                UART0_PrintString(buf);
            } else if (strlen(rcvbuf) == 14 &&
                    strncmp(rcvbuf, "regw 0x", 7) == 0) {
                strncpy(buf, rcvbuf + 7, 2);
                buf[2] = 0;
                addr1 = strtoul((char *) buf, NULL, 16);
                addr2 = strtoul(rcvbuf + 12, NULL, 16);
                ov7670_set(addr1, addr2);
                sprintf(buf, "0x%.2x 0x%.2x\r\n", addr1, addr2);
                UART0_PrintString(buf);
            } else {
                UART0_PrintString("ERR\r\n");
                printf("Unknown command: [%s]\n", rcvbuf);
            }
        }
    }

    return 0;
}

/* vim: set et sw=4: */
