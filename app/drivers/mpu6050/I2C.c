#include "mpu6050/I2C.h"
#include "i2c_manager/i2c_manager.h"
#include <string.h>

HAL_StatusTypeDef i2c_write(uint8_t slave_addr, uint8_t reg_addr, uint8_t length, uint8_t const *data) {
    if (length + 1 > 128)
        return HAL_ERROR;
    uint8_t buf[128];
    buf[0] = reg_addr;
    if (length > 0 && data != NULL) {
        memcpy(buf + 1, data, length);
    }
    return I2C_WRITE_MPU6050(buf, length + 1);
}

HAL_StatusTypeDef i2c_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t length, uint8_t *data) {
    return I2C_READ_MPU6050(&reg_addr, 1, data, length);
}

HAL_StatusTypeDef IICwriteBit(uint8_t slave_addr, uint8_t reg_addr, uint8_t bitNum, uint8_t data) {
    uint8_t tmp;
    i2c_read(slave_addr, reg_addr, 1, &tmp);
    tmp = (data != 0) ? (tmp | (1 << bitNum)) : (tmp & ~(1 << bitNum));
    return i2c_write(slave_addr, reg_addr, 1, &tmp);
};

HAL_StatusTypeDef IICwriteBits(uint8_t slave_addr, uint8_t reg_addr, uint8_t bitStart, uint8_t length, uint8_t data) {
    uint8_t tmp, dataShift;
    HAL_StatusTypeDef status = i2c_read(slave_addr, reg_addr, 1, &tmp);
    if (status == HAL_OK) {
        uint8_t mask = (((1 << length) - 1) << (bitStart - length + 1));
        dataShift    = data << (bitStart - length + 1);
        tmp &= ~mask;
        tmp |= dataShift;
        return i2c_write(slave_addr, reg_addr, 1, &tmp);
    } else {
        return status;
    }
}
