#include <gpio.h>
#include <spi.h>
#include <stdint.h>

#include "spi_manager.h"

#define NUMBER_OF_SPI_DEVICES SPI_DEVICE_SENTINEL

struct spi_manager_device {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
};

static struct spi_manager_device devices[NUMBER_OF_SPI_DEVICES] = {
    [SPI_DEVICE_LSM6DSV] = {
        .hspi    = &hspi2,
        .cs_port = GPIOB,
        .cs_pin =  GPIO_PIN_1,
    },
    [SPI_DEVICE_BMI088_ACC] = {
        .hspi    = &hspi1,
        .cs_port = GPIOC,
        .cs_pin =  GPIO_PIN_7,
    },
    [SPI_DEVICE_BMI088_GYR] = {
        .hspi    = &hspi1,
        .cs_port = GPIOB,
        .cs_pin =  GPIO_PIN_0,
    },
};

HAL_StatusTypeDef spi_manager_write(spi_sensor_t sensor, uint8_t *buf, size_t len) {
    assert_param(sensor < NUMBER_OF_SPI_DEVICES);
    assert_param(buf != NULL);

    HAL_StatusTypeDef status;

    HAL_GPIO_WritePin(devices[sensor].cs_port, devices[sensor].cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(devices[sensor].hspi, buf, len, 10);
    HAL_GPIO_WritePin(devices[sensor].cs_port, devices[sensor].cs_pin, GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef
spi_manager_read(spi_sensor_t sensor, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len) {
    assert_param(sensor < NUMBER_OF_SPI_DEVICES);
    assert_param(tx_buf != NULL);
    assert_param(rx_buf != NULL);

    HAL_StatusTypeDef status;

    HAL_GPIO_WritePin(devices[sensor].cs_port, devices[sensor].cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(devices[sensor].hspi, tx_buf, tx_len, 10);
    if (status == HAL_OK) {
        status = HAL_SPI_Receive(devices[sensor].hspi, rx_buf, rx_len, 10);
    }
    HAL_GPIO_WritePin(devices[sensor].cs_port, devices[sensor].cs_pin, GPIO_PIN_SET);
    return status;
}
