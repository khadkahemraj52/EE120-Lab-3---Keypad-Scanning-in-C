#ifndef OLED_I2C_H
#define OLED_I2C_H

#include "stm32l476xx.h"
#include <stdint.h>

/*
 * Register-level I2C1 driver for NUCLEO-L476RG + 4-pin SSD1306 OLED.
 *
 * Wiring:
 *   OLED VCC -> Nucleo 3.3V
 *   OLED GND -> Nucleo GND
 *   OLED SCL -> Nucleo D15 / PB8  (I2C1_SCL, AF4)
 *   OLED SDA -> Nucleo D14 / PB9  (I2C1_SDA, AF4)
 */

typedef enum {
    OLED_I2C_OK = 0,
    OLED_I2C_TIMEOUT,
    OLED_I2C_NACK,
    OLED_I2C_BUS_ERROR,
    OLED_I2C_BAD_LENGTH
} OLED_I2C_Status_t;

void OLED_I2C_Init(void);
OLED_I2C_Status_t OLED_I2C_WriteCommand(uint8_t command);
OLED_I2C_Status_t OLED_I2C_WriteData(const uint8_t *data, uint16_t size);
OLED_I2C_Status_t OLED_I2C_GetLastStatus(void);
void OLED_DelayMs(uint32_t ms);

#endif /* OLED_I2C_H */
