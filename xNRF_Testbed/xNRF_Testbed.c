/*
 * xNRF_Testbed.c
 *
 * Project: xNRF_Testbed
 * Copyright (c) 2014 Shelby Merrick
 * http://www.forkineye.com
 *
 *  This program is provided free for you to use in any way that you wish,
 *  subject to the laws and regulations where you are using it.  Due diligence
 *  is strongly suggested before using this code.  Please give credit where due.
 *
 *  The Author makes no warranty of any kind, express or implied, with regard
 *  to this program or the documentation contained in this document.  The
 *  Author shall not be liable in any event for incidental or consequential
 *  damages in connection with, or arising out of, the furnishing, performance
 *  or use of these programs.
 *
 */ 

#ifndef F_CPU
#   define F_CPU 32000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include "XNRF24L01.h"
#include "XSPI.h"

static xnrf_config_t xnrf_config = {
    .spi = &SPIC,
    .spi_port = &PORTC,
    .ss_port = &PORTC,
    .ss_pin = 4,
    .ce_port = &PORTC,
    .ce_pin  = 2,
    .addr_width = 5,
    .confbits = 0b01011100
    //.confbits = ((1 << EN_CRC) | (1 << CRCO)) //TODO: move confbits elsewhere for runtime config changes
};

void init() {
    // Configure clock to 32MHz
    OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;    /* Enable the internal 32MHz & 32KHz oscillators */
    while(!(OSC.STATUS & OSC_RC32KRDY_bm));         /* Wait for 32Khz oscillator to stabilize */
    while(!(OSC.STATUS & OSC_RC32MRDY_bm));         /* Wait for 32MHz oscillator to stabilize */
    DFLLRC32M.CTRL = DFLL_ENABLE_bm ;               /* Enable DFLL - defaults to calibrate against internal 32Khz clock */
    CCP = CCP_IOREG_gc;                             /* Disable register security for clock update */
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;                /* Switch to 32MHz clock */
    OSC.CTRL &= ~OSC_RC2MEN_bm;                     /* Disable 2Mhz oscillator */
	
    //PORTE.DIRSET = PIN1_bm; /* A4U LED */
    PORTA.DIRSET = PIN0_bm; /* E5 LED */
}

int main(void) {
    init();

    // Initialize XNRF driver
	xnrf_init(&xnrf_config);
	
    uint8_t	testdata[32] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                            0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
                            0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
                            0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80};

    uint64_t tx_addr = 0xF0F0F0F0D2LL;
    uint64_t rx_addr1 = 0xF0F0F0F0E1LL;
	
    // set channel, speed, and disable ack's.
    xnrf_set_channel(&xnrf_config, 100);
    xnrf_set_datarate(&xnrf_config, XNRF_2MBPS);
    xnrf_write_register(&xnrf_config, EN_AA, 0);
    
    // set addresses
    xnrf_set_tx_address(&xnrf_config, (uint8_t*)&tx_addr);
    xnrf_set_rx0_address(&xnrf_config, (uint8_t*)&tx_addr);
    xnrf_set_rx1_address(&xnrf_config, (uint8_t*)&rx_addr1);

    // power-up transmitter and give 5ms to stabilize
    xnrf_powerup_tx(&xnrf_config);
    _delay_ms(5);
	
    while(1) {
        // Toggle LED
        //PORTE.OUTTGL = PIN1_bm; /* A4U LED */
        PORTA.OUTTGL = PIN0_bm; /* E5 LED */
		
        // reset TX interrupt and write the payload
        xnrf_write_register(&xnrf_config, NRF_STATUS, (1 << TX_DS));
        xnrf_write_payload(&xnrf_config, testdata, 32);

        xnrf_enable(&xnrf_config); /* send it off */
        _delay_us(15);
        xnrf_disable(&xnrf_config);

        // and wait
        _delay_ms(1000);
    }
}