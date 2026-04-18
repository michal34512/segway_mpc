#ifndef DRIVERS_SPI_MANAGER_SPI_MANAGER_H_
#define DRIVERS_SPI_MANAGER_SPI_MANAGER_H_

#include <stm32h7xx_hal.h>

typedef enum {
    SPI_DEVICE_LSM6DSV,
    SPI_DEVICE_BMI088_ACC,
    SPI_DEVICE_BMI088_GYR,
    SPI_DEVICE_SENTINEL,
} spi_sensor_t;

HAL_StatusTypeDef spi_manager_write(spi_sensor_t sensor, uint8_t *buf, size_t len);

HAL_StatusTypeDef spi_manager_read(spi_sensor_t sensor, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len);

#define SPI_READ_LSM6DSV(tx_buf, tx_len, rx_buf, rx_len) \
    spi_manager_read(SPI_DEVICE_LSM6DSV, tx_buf, tx_len, rx_buf, rx_len)
#define SPI_WRITE_LSM6DSV(BUFFER, LEN) spi_manager_write(SPI_DEVICE_LSM6DSV, BUFFER, LEN)

#define SPI_READ_BMI088_ACC(tx_buf, tx_len, rx_buf, rx_len) \
    spi_manager_read(SPI_DEVICE_BMI088_ACC, tx_buf, tx_len, rx_buf, rx_len)
#define SPI_WRITE_BMI088_ACC(BUFFER, LEN) spi_manager_write(SPI_DEVICE_BMI088_ACC, BUFFER, LEN)

#define SPI_READ_BMI088_GYR(tx_buf, tx_len, rx_buf, rx_len) \
    spi_manager_read(SPI_DEVICE_BMI088_GYR, tx_buf, tx_len, rx_buf, rx_len)
#define SPI_WRITE_BMI088_GYR(BUFFER, LEN) spi_manager_write(SPI_DEVICE_BMI088_GYR, BUFFER, LEN)

#endif  // DRIVERS_SPI_MANAGER_SPI_MANAGER_H_
