#include <stdint.h>

#include "i2c.h"
#include "i2c_manager.h"
#include "logger/logger.h"

#define NUMBER_OF_I2C_DEVICES I2C_DEVICE_SENTINEL

struct i2c_manager_device {
    I2C_HandleTypeDef *hi2c;
    uint16_t address;
};

static struct i2c_manager_device devices[NUMBER_OF_I2C_DEVICES] = {
    [I2C_DEVICE_MPU6050] = {
        .hi2c    = &hi2c3,
        .address = 0x68 << 1, // or 0x69
    },
};

HAL_StatusTypeDef i2c_manager_write(i2c_sensor_t sensor, uint8_t *buf, size_t len) {
    assert_param(sensor < NUMBER_OF_I2C_DEVICES);
    assert_param(buf != NULL);

    return HAL_I2C_Master_Transmit(devices[sensor].hi2c, devices[sensor].address, buf, len, HAL_MAX_DELAY);
}

HAL_StatusTypeDef
i2c_manager_read(i2c_sensor_t sensor, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len) {
    assert_param(sensor < NUMBER_OF_I2C_DEVICES);
    assert_param(tx_buf != NULL);
    assert_param(rx_buf != NULL);
    HAL_StatusTypeDef status;

    if (tx_len == 1) {
        // Używamy Mem_Read, które poprawnie obsługuje "Repeated Start" wymagany przez MPU6050
        return HAL_I2C_Mem_Read(devices[sensor].hi2c,
                                devices[sensor].address,
                                tx_buf[0],
                                I2C_MEMADD_SIZE_8BIT,
                                rx_buf,
                                rx_len,
                                HAL_MAX_DELAY);
    } else {
        status = HAL_I2C_Master_Transmit(devices[sensor].hi2c, devices[sensor].address, tx_buf, tx_len, HAL_MAX_DELAY);
        if (status != HAL_OK)
            return status;

        return HAL_I2C_Master_Receive(devices[sensor].hi2c, devices[sensor].address, rx_buf, rx_len, HAL_MAX_DELAY);
    }
}
