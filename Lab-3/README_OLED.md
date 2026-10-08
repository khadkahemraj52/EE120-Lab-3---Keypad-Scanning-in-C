# STM32L476RGT6 Nucleo + SSD1306 OLED (Keil uVision)

This folder is a ready-to-open modification of the uploaded `Lab_03_Keypad` Keil project.
It keeps the original bare-metal/CMSIS style and **does not require STM32 HAL**.
The uploaded Aleksander Alekseev SSD1306 library was adapted so its I2C transport calls
`OLED_I2C.c` instead of `HAL_I2C_Mem_Write()`.

## Hardware assumed

- Board: NUCLEO-L476RG / STM32L476RGT6
- Display: 4-pin I2C SSD1306, normally 128x64, address 0x3C
- Keil project: `project.uvprojx`

## Wiring

| SSD1306 | NUCLEO-L476RG | STM32 pin |
|---|---|---|
| VCC | 3.3V | - |
| GND | GND | - |
| SCL / SCLD | D15 | PB8 / I2C1_SCL |
| SDA | D14 | PB9 / I2C1_SDA |

Do not connect VCC to a higher voltage unless you have verified your exact OLED module.
The project uses 3.3 V logic.

Most 4-pin SSD1306 breakout boards already contain I2C pull-up resistors. The code also
enables the STM32's internal pull-ups. If the bus is unreliable and your board has no
on-board pull-ups, add external pull-ups (commonly about 4.7 kOhm) from SCL and SDA to 3.3 V.

## What to open in Keil

Open:

`project.uvprojx`

The project already includes:

- `main.c` - simple OLED Hello test
- `OLED_I2C.c/.h` - register-level STM32L476 I2C1 transport
- `ssd1306.c/.h` - adapted SSD1306 graphics library
- `ssd1306_fonts.c/.h` - font data
- `ssd1306_conf.h` - display configuration
- Original lab sources (`SysClock`, LED, UART, keypad)

The project intentionally does **not** use the separately downloaded
`stm32l4xx_hal_i2c.h`. A HAL header alone is not a complete HAL driver.

## First test

1. Wire the OLED exactly as shown above.
2. Connect the Nucleo to USB/ST-LINK.
3. Open `project.uvprojx` in Keil uVision.
4. Build the project.
5. Flash/Download it to the board.
6. Reset the board if needed.

Expected display:

```
Hello!
STM32L476
SSD1306 OLED
```

`main.c` is deliberately small so you can validate the OLED independently of the keypad.

## How the software works

1. `System_Clock_Init()` keeps the original lab's 80 MHz CPU clock.
2. `OLED_I2C_Init()` configures PB8/PB9 for AF4 open-drain I2C1.
3. I2C1's own kernel clock is selected from HSI16, so the 100 kHz I2C timing does not depend
   on the 80 MHz APB setting.
4. `ssd1306_Init()` sends the controller initialization commands.
5. `ssd1306_WriteString()` draws characters into a RAM framebuffer.
6. `ssd1306_UpdateScreen()` sends the framebuffer to the display over I2C.

## Display size

`ssd1306_conf.h` is currently configured for the common 128x64 display:

```c
#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64
```

If your OLED is 128x32, change only the height to:

```c
#define SSD1306_HEIGHT 32
```

Then rebuild.

## I2C address

This project assumes the common SSD1306 address 0x3C. If your module uses 0x3D, change the single setting in `ssd1306_conf.h`:

```c
#define SSD1306_I2C_ADDR_7BIT 0x3DU
```

`OLED_I2C.c` reads this setting directly, so there is only one address to maintain.

## If the screen stays blank

Check these in order:

1. VCC is 3.3 V and ground is common.
2. SCL is PB8/D15 and SDA is PB9/D14 (not reversed).
3. The display is actually SSD1306-compatible and I2C, not SH1106 or SPI.
4. Try address 0x3D if 0x3C does not acknowledge.
5. If the onboard LED remains ON immediately after reset, the example detected an I2C
   error during SSD1306 initialization. Recheck wiring/address/pull-ups.
6. If the image is shifted, mirrored, or upside down, use the mirror/offset settings in
   `ssd1306_conf.h`.
7. If the panel is 128x32, change `SSD1306_HEIGHT` to 32.

## Keypad integration later

`main_keypad_oled_example.c.txt` is included but deliberately excluded from the Keil build,
so it cannot conflict with the Hello-test `main.c`.

After the simple OLED test works, you can copy its contents over `main.c` to display the
keypad string on the OLED as keys are pressed.

`main_keypad_original.c.txt` is a backup of the original uploaded `main.c`.
