#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

/* This project uses the I2C version of the 4-pin SSD1306 module. */
#define SSD1306_USE_I2C

/* Typical 4-pin SSD1306 7-bit I2C address. */
#define SSD1306_I2C_ADDR_7BIT   0x3CU

/* Common module size. Change HEIGHT to 32 if your panel is 128x32. */
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64

/* Include the fonts supplied with the Aleksander Alekseev library. */
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26
#define SSD1306_INCLUDE_FONT_16x24
#define SSD1306_INCLUDE_FONT_16x15

/* Uncomment only if your particular module appears mirrored/inverted. */
/* #define SSD1306_MIRROR_VERT */
/* #define SSD1306_MIRROR_HORIZ */
/* #define SSD1306_INVERSE_COLOR */

#endif /* __SSD1306_CONF_H__ */
