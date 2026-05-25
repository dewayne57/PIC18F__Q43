/* ******************************************************************************************
 * I2C Library Header File
 * Author: Your Name
 * Date: YYYY-MM-DD
 * Description: This file contains the function prototypes and definitions for the I2C library.
 ****************************************************************************************** */

 #ifndef I2CLIB_H
 #define I2CLIB_H   

#include <stdint.h>
#include <stdbool.h>  

typedef enum
{
    I2C_SUCCESS = 0,
    I2C_ERROR_TIMEOUT,
    I2C_ERROR_NOT_INITIALIZED,
    I2C_ERROR_ILLEGAL_STATE,
    I2C_ERROR_BUS_COLLISION
} i2c_status_t;

typdef enum {
    I2C_MODE_MASTER_7BIT = 0,
    I2C_MODE_MASTER_10BIT,
    I2C_MODE_SLAVE_7BIT,
    I2C_MODE_SLAVE_10BIT,
    I2C_MODE_MULTI_MASTER_7BIT,
    I2C_MODE_MULTI_MASTER_10BIT
} i2c_mode_t;

typedef struct
{
    uint8_t *tx_buffer;              // Pointer to the transmit buffer
    uint8_t *rx_buffer;              // Pointer to the receive buffer
    uint16_t tx_buffer_size;         // Size of the transmit buffer in bytes
    uint16_t rx_buffer_size;         // Size of the receive buffer in bytes
    i2c_mode_t mode;                 // I2C operating mode

    // Everything below this line is for internal driver use and should not 
    // be modified by the application. 
    i2c_status_t status;             // I2C status
    uint8_t tx_pos;                  // Current position in the transmit buffer
    uint8_t rx_pos;                  // Current position in the receive buffer

    bool initialized;                // Flag to indicate if the handle has been initialized
    volatile i2c_operation_t current_operation; // Current I2C operation (read/write/none)
    volatile uint8_t rx_pos;         // Current position in the receive buffer
    volatile uint8_t tx_pos;         // Current position in the transmit buffer

} i2c_handle_t;

#endif // I2CLIB_H