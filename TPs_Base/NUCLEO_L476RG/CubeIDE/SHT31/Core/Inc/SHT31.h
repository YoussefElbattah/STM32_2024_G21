/*
 * SHT31.h
 *
 *  Created on: Feb 10, 2024
 *      Author: Home
 */

#ifndef INC_SHT31_H_
#define INC_SHT31_H_

#include <stdbool.h>
#include "stm32l4xx_hal.h"

#ifndef SHT3X_I2C_TIMEOUT
#define SHT3X_I2C_TIMEOUT 30
#endif

#define SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW 0x44
#define SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_HIGH 0x45

/**
 * Structure defining a handle describing a SHT3x device.
 */
typedef struct {

	/**
	 * The handle to the I2C bus for the device.
	 */
	I2C_HandleTypeDef *i2c_handle;

	/**
	 * The I2C device address.
	 * @see{PCA9865_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW} and @see{SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_HIGH}
	 */
	uint16_t device_address;

} sht3x_handle_t;

/**
 * Checks if an SHT3x is reachable using the given handle.
 * @param handle Handle to the SHT3x device.
 * @return True on success, false otherwise.
 */
bool sht3x_init(sht3x_handle_t *handle);

/**
 * Takes a single temperature and humidity measurement.
 * @param handle Handle to the SHT3x device.
 * @param temperature Pointer to the storage location for the sampled temperature.
 * @param humidity Pointer to the storage location for the sampled humidity.
 * @return True on success, false otherwise.
 */
bool sht3x_read_temperature_and_humidity(sht3x_handle_t *handle, float *temperature, float *humidity);



#endif /* INC_SHT31_H_ */
