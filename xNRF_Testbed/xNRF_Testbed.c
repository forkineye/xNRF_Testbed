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
#	define F_CPU 32000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include "XNRF24L01.h"
#include "XSPI.h"

// SPI slave selects
//#define SELECT PORTC.OUTCLR = PIN4_bm;
//#define DESELECT PORTC.OUTSET = PIN4_bm;

static xnrf_config_t xnrf_config = {
	.spi = &SPIC,
	.ss_port = &PORTC,
	.ss_pin = 4,
	.ce_port = &PORTA,
	.ce_pin  = 1,
	.confbits = ((1<<EN_CRC) | (0<<CRCO)) //TODO: move confbits elsewhere for runtime config changes
};

void init() {
	// Configure clock to 32MHz
	OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;// Enable the internal 32MHz & 32KHz oscillators
	while(!(OSC.STATUS & OSC_RC32KRDY_bm))		// Wait for 32Khz oscillator to stabilize
	while(!(OSC.STATUS & OSC_RC32MRDY_bm));		// Wait for 32MHz oscillator to stabilize
	DFLLRC32M.CTRL = DFLL_ENABLE_bm ;			// Enable DFLL - defaults to calibrate against internal 32Khz clock
	CCP = CCP_IOREG_gc;							// Disable register security for clock update
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;			// Switch to 32MHz clock
	OSC.CTRL &= ~OSC_RC2MEN_bm;					// Disable 2Mhz oscillator
	
	// Initialize SPI
	PORTC.DIRSET = PIN4_bm;  // slave select pin
	xspi_master_init(&PORTC, &SPIC, SPI_MODE_0_gc, false, SPI_PRESCALER_DIV16_gc, false);
	//xspi_usart_master_init(&PORTC, &USARTC0, SPI_MODE_0_gc, 8000000);
	
	PORTE.DIRSET = PIN1_bm; // led
}

int main(void) {
	init();
	
	uint8_t	data[4] = {0x61, 0x62, 0x63, 0x64};
	
    while(1) {
		// Toggle LED
		PORTE.OUTTGL = PIN1_bm;
		//xspi_transfer_byte(&SPIC, 0x55);
		//xspi_send_packet(&SPIC, data, 4);
		xnrf_read_register(&xnrf_config, NRF_STATUS);
		_delay_ms(125);
    }
}