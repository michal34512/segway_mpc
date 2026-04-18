#ifndef DRIVERS_I2C_MANAGER_I2C_MANAGER_H_
#define DRIVERS_I2C_MANAGER_I2C_MANAGER_H_

#include <stm32h7xx_hal.h>

typedef enum {
    I2C_DEVICE_BMM350,
    I2C_DEVICE_BMP581,
    I2C_DEVICE_SENTINEL,
} i2c_sensor_t;

HAL_StatusTypeDef i2c_manager_write(i2c_sensor_t sensor, uint8_t *buf, size_t len);

HAL_StatusTypeDef i2c_manager_read(i2c_sensor_t sensor, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len);

#define I2C_READ_BMM350(tx_buf, tx_len, rx_buf, rx_len) \
    i2c_manager_read(I2C_DEVICE_BMM350, tx_buf, tx_len, rx_buf, rx_len)
#define I2C_WRITE_BMM350(BUFFER, LEN) i2c_manager_write(I2C_DEVICE_BMM350, BUFFER, LEN)

#define I2C_READ_BMP581(tx_buf, tx_len, rx_buf, rx_len) \
    i2c_manager_read(I2C_DEVICE_BMP581, tx_buf, tx_len, rx_buf, rx_len)
#define I2C_WRITE_BMP581(BUFFER, LEN) i2c_manager_write(I2C_DEVICE_BMP581, BUFFER, LEN)

#endif  // DRIVERS_I2C_MANAGER_I2C_MANAGER_H_
