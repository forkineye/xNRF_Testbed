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

//TODO:  Add circular buffers

#ifndef F_CPU
#   define F_CPU 32000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
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
    .payload_width = 32,
    //.confbits = 0b01011100  //  TX interrupt enabled
    .confbits = 0b00111100    //  RX interrupt enabled
    //.confbits = 0b01111100  //  All interrupts disabled
    //.confbits = 0b00001100  //  All interrupts enabled
    //.confbits = ((1 << EN_CRC) | (1 << CRCO)) //TODO: move confbits elsewhere for runtime config changes
};

uint8_t rxbuff[32];    /* global RX buffer */

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

/* Loop for TX testing */
void tx_loop() {
    uint8_t	testdata[32] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                            0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
                            0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
                            0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80};


    // power-up transmitter and give 5ms to stabilize
    xnrf_powerup_tx(&xnrf_config);
    _delay_ms(5);
    
    while(1) {
        // Toggle status LED
        //PORTE.OUTTGL = PIN1_bm; /* A4U LED */
        PORTA.OUTTGL = PIN0_bm; /* E5 LED */
        
        // reset TX interrupt and write the payload
        xnrf_write_register(&xnrf_config, NRF_STATUS, (1 << TX_DS));
        xnrf_write_payload(&xnrf_config, testdata, 32);

        //TODO: If we were in a faster loop, the 130us state transition would need to be considered as to not overflow the TX FIFOs.
        xnrf_enable(&xnrf_config); /* send it off */
        _delay_us(15);
        xnrf_disable(&xnrf_config);

        // and wait
        _delay_ms(1000);
    }    
}

/* Loop for interrupt driven RX testing */
void rx_int_loop() {
    // power-up receiver and give 5ms to stabilize
    xnrf_powerup_rx(&xnrf_config);
    _delay_ms(5);
    
    // setup interrupt listener
    PORTC_PIN3CTRL = PORT_ISC_FALLING_gc;   /* Setup PC3 to sense falling edge */
    PORTC.INTMASK = PIN3_bm;                /* Enable pin change interrupt for PC3 */
    PORTC.INTCTRL = PORT_INTLVL_LO_gc;      /* Set Port C for low level interrupts */
    PMIC.CTRL |= PMIC_LOLVLEN_bm;           /* Enable low interrupts */
    sei();                                  /* Enable global interrupt flag */                              
    
    // start listening
    xnrf_enable(&xnrf_config);
    //_delay_ms(130); /* do we need to delay for state transition? interrupts shouldn't fire until radio is ready i would assume */
    
    while (1) {
        /* go about our business and let interrupts handle nRF stuff */
    }
}

/* Interrupt handler for rx_int_loop() */
ISR(PORTC_INT_vect) {
    xnrf_read_payload(&xnrf_config, rxbuff, xnrf_config.payload_width);     /* retrieve the payload */
    xnrf_write_register(&xnrf_config, NRF_STATUS, (1 << RX_DR));            /* reset the RX_DR status */
    //TODO: Check FIFO status to keep reading payloads if needed

    // Toggle status LED
    PORTA.OUTTGL = PIN0_bm; /* E5 LED */
    
    // Clean interrupt flag for PC3
    PORTC.INTFLAGS = PIN3_bm;
}

/* Loop for polled RX testing */
void rx_poll_loop() {
    // power-up receiver and give 5ms to stabilize
    xnrf_powerup_rx(&xnrf_config);
    _delay_ms(5);

    // start listening
    xnrf_enable(&xnrf_config);
    //_delay_ms(130); /* do we need to delay for state transition? status should return RX_DR empty util radio is ready i would assume */
    
    while (1) {
        if(xnrf_get_status(&xnrf_config) & (1 << RX_DR)) {                          /* check the RX_DR status to see if we have a packet */
            xnrf_read_payload(&xnrf_config, rxbuff, xnrf_config.payload_width);     /* retrieve the payload */
            xnrf_write_register(&xnrf_config, NRF_STATUS, (1 << RX_DR));            /* reset the RX_DR status */
            //TODO: Check FIFO status to keep reading payloads if needed

            // Toggle status LED
            PORTA.OUTTGL = PIN0_bm; /* E5 LED */
        }
    }
}

int main(void) {
    init();

    // Initialize XNRF driver
	xnrf_init(&xnrf_config);
	
    // configure the radio
    xnrf_set_channel(&xnrf_config, 100);                /* set our channel */
    xnrf_set_datarate(&xnrf_config, XNRF_250KBPS);      /* set our data rate */
    xnrf_write_register(&xnrf_config, EN_AA, 0);        /* disable auto ack's */
    xnrf_write_register(&xnrf_config, EN_RXADDR, 3);    /* listen on pipes 0 & 1 */
    
    uint64_t tx_addr = 0xF0F0F0F0E1LL;
    uint64_t rx_addr1 = 0xF0F0F0F0D2LL;
    
    // set addresses
    xnrf_set_tx_address(&xnrf_config, (uint8_t*)&tx_addr);
    xnrf_set_rx0_address(&xnrf_config, (uint8_t*)&tx_addr);
    xnrf_set_rx1_address(&xnrf_config, (uint8_t*)&rx_addr1);

    // TX test loop
    //tx_loop();
    
    // RX text loop - interrupt driven
    rx_int_loop();
    
    // RX test loop - polled
    //rx_poll_loop();
    
}