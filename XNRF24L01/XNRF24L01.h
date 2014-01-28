/*
 * XNRF24L01.h
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

#ifndef XNRF24L01_H_
#define XNRF24L01_H_

#include "nRF24L01.h"
#include "XSPI.h"

//TODO: Update this to support USART and move confbits elsewhere.
/*! \brief Structure which defines some items needed for nRF and SPI control.
 *  \param spi Pointer to the SPI module this nRF is connected to.
 *  \param ss_port Pointer to the port containing the Slave Select pin.
 *  \param ss_pin Slave Select pin number.
 *  \param ce_port Pointer to the port containing the Chip Enable pin.
 *  \param ce_pin Chip Enable pin number.
 *  \param confbits Configuration bits to be written to the CONFIG register. This shiznit needs to be moved elsewhere.
 */
typedef struct {
	SPI_t *spi;
	PORT_t *ss_port;
	uint8_t ss_pin;
	PORT_t *ce_port;
	uint8_t ce_pin;
	uint8_t confbits;
} xnrf_config_t;

/*! \brief Pulls the Slave Select line low and selects our nRF.
 *  \param config Pointer to a xnrf_config_t structure.
 */
static inline void xnrf_select(xnrf_config_t *config) {
	config->ss_port->OUTCLR = (1 << config->ss_pin);
}

/*! \brief Pulls the Slave Select line high and de-selects our nRF.
 *  \param config Pointer to a xnrf_config_t structure.
 */
static inline void xnrf_deselect(xnrf_config_t *config) {
	config->ss_port->OUTSET = (1 << config->ss_pin);
}

/*! \brief Flushes the RX FIFO.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \return Contents of the STATUS register.
 */
static inline uint8_t xnrf_flush_rx(xnrf_config_t *config) {
	xnrf_select(config);
	uint8_t status = xspi_transfer_byte(config->spi, FLUSH_RX);
	xnrf_deselect(config);
	return status;
}

/*! \brief Flushes the TX FIFO.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \return Contents of the STATUS register. 
 */
static inline uint8_t xnrf_flush_tx(xnrf_config_t *config) {
	xnrf_select(config);
	uint8_t status = xspi_transfer_byte(config->spi, FLUSH_TX);
	xnrf_deselect(config);
	return status;
}

/*! \brief Retrieves contents of the STATUS register.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \return Contents of the STATUS register.
 */
static inline uint8_t xnrf_get_status(xnrf_config_t *config) {
	xnrf_select(config);
	uint8_t status = xspi_transfer_byte(config->spi, NRF_NOP);
	xnrf_deselect(config);
	return status;
}

/*! \brief Returns a single byte for the given register.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \param reg Register to query.
 *  \return Contents of the register.
 */
static inline uint8_t xnrf_read_register(xnrf_config_t *config, uint8_t reg) {
	xnrf_select(config);
	xspi_transfer_byte(config->spi, (R_REGISTER | (REGISTER_MASK & reg)));
	uint8_t result = xspi_transfer_byte(config->spi, NRF_NOP);
	xnrf_deselect(config);
	return result;
}

/*! \brief Writes a single byte to the given register.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \param reg Register to write.
 *  \param val Value to write to the register.
 */
static inline void xnrf_write_register(xnrf_config_t *config, uint8_t reg, uint8_t val) {
	xnrf_select(config);
	xspi_transfer_byte(config->spi, (W_REGISTER | (REGISTER_MASK & reg)));
	xspi_transfer_byte(config->spi, val);
	xnrf_deselect(config);
}

//TODO: need to move confbits elsewhere to they can change at runtime */
/*! \brief Powers up the nRF in TX mode.
 *  \param config Pointer to a xnrf_config_t structure.
 */
static inline void xnrf_powerup_tx (xnrf_config_t *config) {
	xnrf_write_register(config, CONFIG, (config->confbits | ((1<<PWR_UP) | (0<<PRIM_RX))));
}

//TODO: need to move confbits elsewhere to they can change at runtime */
/*! \brief Powers up the nRF in RX mode.
 *  \param config Pointer to a xnrf_config_t structure.
 */
static inline void xnrf_powerup_rx (xnrf_config_t *config) {
	xnrf_write_register(config, CONFIG, (config->confbits | ((1<<PWR_UP) | (1<<PRIM_RX))));
}

/*! \brief Powers down the nRF.
 *  \param config Pointer to a xnrf_config_t structure.
 */
//TODO: need to move confbits elsewhere to they can change at runtime */
static inline void xnrf_powerdown (xnrf_config_t *config) {
	xnrf_write_register(config, CONFIG, (config->confbits | (0<<PWR_UP)));
}

/*! \brief Sets the nRF channel.
 *  \param channel Desired channel number (1-127).  We don't check so stay in range.  We're embedded FFS, don't be a tard.
 */
static inline void xnrf_set_channel (xnrf_config_t *config, uint8_t channel) {
	xnrf_write_register(config, RF_CH, channel);
}

/*! \brief Retrieves an array of bytes for the given register.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \param reg Register to trigger the query.
 *  \param data Address of a buffer to hold the returned data.
 *  \param len Length of the data you're retrieving.
 */
void xnrf_read_register_buffer(xnrf_config_t *config, uint8_t reg, uint8_t *data, uint8_t len);

/*! \brief Writes an array of bytes for the given register.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \param reg Register to trigger the write.
 *  \param data Pointer to the data we are sending.
 *  \param len Size of the data we are sending.
 */
void xnrf_write_register_buffer(xnrf_config_t *config, uint8_t reg, uint8_t *data, uint8_t len);

/*! \brief Retrieves a payload.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \param data Address of a buffer to hold the returned payload
 *  \param len Length of the payload you're retrieving.
 */
void xnrf_read_payload(xnrf_config_t *config, uint8_t *data, uint8_t len);

/*! \brief Writes a payload to be transmitted.
 *  \param config Pointer to a xnrf_config_t structure.
 *  \param data Pointer to the payload we are sending.
 *  \param len Size of the payload we are sending.
 */
void xnrf_write_payload(xnrf_config_t *config, uint8_t *data, uint8_t len);

#endif /* XNRF24L01_H_ */