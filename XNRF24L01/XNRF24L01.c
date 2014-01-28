/*
 * XNRF24L01.c
 *
 * Project: XNRF24L01
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

#include <avr/io.h>
#include "XSPI.h"
#include "XNRF24L01.h"

void xnrf_read_register_buffer(xnrf_config_t *config, uint8_t reg, uint8_t *data, uint8_t len) {
	xnrf_select(config);
	xspi_transfer_byte(config->spi, (R_REGISTER | (REGISTER_MASK & reg)));
	while (len--)
		*data++ = xspi_transfer_byte(config->spi, NRF_NOP);
	xnrf_deselect(config);	
}

void xnrf_write_register_buffer(xnrf_config_t *config, uint8_t reg, uint8_t *data, uint8_t len) {
	xnrf_select(config);
	xspi_transfer_byte(config->spi, (W_REGISTER | (REGISTER_MASK & reg)));
	while (len--)
		xspi_transfer_byte(config->spi, *data++);
	xnrf_deselect(config);	
}

void xnrf_read_payload(xnrf_config_t *config, uint8_t *data, uint8_t len) {
	xnrf_select(config);
	xspi_transfer_byte(config->spi, R_RX_PAYLOAD);
	while (len--)
		*data++ = xspi_transfer_byte(config->spi, NRF_NOP);
	xnrf_deselect(config);	
}

void xnrf_write_payload(xnrf_config_t *config, uint8_t *data, uint8_t len) {
	xnrf_select(config);
	xspi_transfer_byte(config->spi, W_TX_PAYLOAD);
	while (len--)
		xspi_transfer_byte(config->spi, *data++);
	xnrf_deselect(config);	
}
