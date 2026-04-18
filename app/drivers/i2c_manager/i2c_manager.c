#include <stdint.h>

#include "i2c.h"
#include "i2c_manager.h"

#define NUMBER_OF_I2C_DEVICES I2C_DEVICE_SENTINEL

struct i2c_manager_device {
    I2C_HandleTypeDef *hi2c;
    uint16_t address;
};

static struct i2c_manager_device devices[NUMBER_OF_I2C_DEVICES] = {
    [I2C_DEVICE_BMM350] = {
        .hi2c    = &hi2c4,
        .address = 0x14 << 1,
    },
    [I2C_DEVICE_BMP581] = {
        .hi2c    = &hi2c4,
        .address = 0x46 << 1,
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

    status = HAL_I2C_Master_Transmit(devices[sensor].hi2c, devices[sensor].address, tx_buf, tx_len, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_I2C_Master_Receive(devices[sensor].hi2c, devices[sensor].address, rx_buf, rx_len, HAL_MAX_DELAY);
}
