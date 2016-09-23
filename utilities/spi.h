#include <avr/io.h>
#include <stdint.h>

#ifndef UTILITIES_SPI_H
#define UTILITIES_SPI_H


typedef enum spi_mode_t { SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3 } spi_mode_t;
typedef enum spi_order_t { SPI_MSB, SPI_LSB } spi_order_t;

void spi0_init(uint32_t speed, spi_mode_t mode=SPI_MODE0, spi_order_t order=SPI_MSB);
uint8_t spi0_exchange(uint8_t val=0);

#endif
