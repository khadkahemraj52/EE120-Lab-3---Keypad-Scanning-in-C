/*
 * main.c  -  Lab 3: Keypad Scanning in C
 *
 * Part C : scan the 4x4 keypad in an infinite loop and show the key on the OLED
 * Part D : keep the last N keypresses in a buffer, register each press exactly
 *          once (on release), and software-debounce the keypad
 * Part E : "something cool" - auto-repeat: a key held down re-enters itself
 *          every AUTO_REPEAT_MS milliseconds
 */

#include "stm32l476xx.h"
#include "SysClock.h"
#include "LED.h"
#include "UART.h"
#include "keypad.h"
#include "OLED_I2C.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------------ */
/* Configuration                                                            */
/* ------------------------------------------------------------------------ */
#define NO_KEY            0xFFU   /* value keypad_scan() returns when idle    */
#define BUFFER_SIZE       16U     /* N = number of keypresses remembered      */
                                  /* (16 chars * 7 px = 112 px, fits 128 px)  */
#define DEBOUNCE_MS       20U     /* key must read the same after this delay  */
#define SCAN_PERIOD_MS    10U     /* pause between scans of the main loop     */
#define AUTO_REPEAT_MS    2000U   /* Part E: repeat interval while key held   */

/* Optional: pick a key to act as backspace (Part E "backspace feature").
 * The 0..F key map has no '*' key, so leave this as NO_KEY to disable, or
 * set it to e.g. 'F' to sacrifice that key for backspace.                   */
#define BACKSPACE_KEY     NO_KEY

/* ------------------------------------------------------------------------ */
/* Part D: keypress buffer                                                  */
/* ------------------------------------------------------------------------ */
static char key_buffer[BUFFER_SIZE + 1U];   /* +1 for the terminating '\0'  */
static uint32_t key_count = 0U;             /* how many slots are filled    */

/* Buffer Initialization: fill with spaces so the display shows "empty". */
static void Buffer_Init(void)
{
    uint32_t i;
    for (i = 0U; i < BUFFER_SIZE; i++) {
        key_buffer[i] = ' ';
    }
    key_buffer[BUFFER_SIZE] = '\0';
    key_count = 0U;
}

/* Append one key.  When the buffer is full, shift everything left by one
 * position (Buffer Content Shifting) so the newest key goes at the end and
 * chronological order is preserved. */
static void Buffer_Push(char key)
{
    uint32_t i;

    if (key_count >= BUFFER_SIZE) {
        for (i = 0U; i < (BUFFER_SIZE - 1U); i++) {
            key_buffer[i] = key_buffer[i + 1U];
        }
        key_buffer[BUFFER_SIZE - 1U] = key;
    } else {
        key_buffer[key_count] = key;
        key_count++;
    }
}

/* Remove the most recent key (used only if BACKSPACE_KEY is enabled). */
static void Buffer_Pop(void)
{
    if (key_count > 0U) {
        key_count--;
        key_buffer[key_count] = ' ';
    }
}

/* ------------------------------------------------------------------------ */
/* Part D: LCD update mechanism                                             */
/* ------------------------------------------------------------------------ */
/*
 * Redraw the whole 128x64 OLED:
 *   line 0 : title  (Font_11x18, 18 px high)
 *   line 2 : the keypress buffer (Font_7x10, 7 px per char)
 */
static void OLED_ShowBuffer(void)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0U, 0U);
    ssd1306_WriteString("Keypad:", Font_11x18, White);

    ssd1306_SetCursor(0U, 24U);
    ssd1306_WriteString(key_buffer, Font_7x10, White);

    /* Nothing appears on the physical OLED until this is called. */
    ssd1306_UpdateScreen();
}

/* ------------------------------------------------------------------------ */
/* Part D: software debouncing                                              */
/* ------------------------------------------------------------------------ */
/*
 * Read the keypad, then read it again DEBOUNCE_MS later.  Only if both reads
 * agree is the key accepted.  Contact bounce on a mechanical switch lasts a
 * few milliseconds, so a value that is stable for 20 ms is a real press
 * (or a real release) rather than bounce.
 */
static unsigned char Keypad_ReadDebounced(void)
{
    unsigned char first = keypad_scan();
    unsigned char second;

    waitms(DEBOUNCE_MS);
    second = keypad_scan();

    if (first == second) {
        return first;
    }
    return NO_KEY;      /* still bouncing - ignore this sample */
}

/* ------------------------------------------------------------------------ */
/* Handle one accepted keypress: update buffer, LCD, (optionally) UART, LED */
/* ------------------------------------------------------------------------ */
static void Register_Key(char key)
{
    if ((unsigned char)key == BACKSPACE_KEY) {
        Buffer_Pop();
    } else {
        Buffer_Push(key);
    }

    OLED_ShowBuffer();
    /* USART_Write(USART2, (uint8_t *)&key, 1U);   // also echo to serial terminal */
    LED_Toggle();
}

/* ------------------------------------------------------------------------ */
int main(void)
{
    unsigned char key;
    unsigned char last_key = NO_KEY;   /* Part D: last key registered        */
    uint32_t held_ms = 0U;             /* Part E: how long last_key is held  */

    System_Clock_Init();
    LED_Init();
    UART2_Init();

    /* Initialize the 4x4 keypad on GPIOC (Part C). */
    Keypad_Pin_Init();

    /* Initialize SSD1306 on I2C1: PB8 = SCL, PB9 = SDA (Part A). */
    OLED_I2C_Init();
    ssd1306_Init();

    /* If the OLED does not acknowledge, stop here with LD2 ON. */
    if (OLED_I2C_GetLastStatus() != OLED_I2C_OK) {
        LED_On();
        while (1) {
        }
    }

    Buffer_Init();
    OLED_ShowBuffer();

    /* Part C: continuously call keypad_scan() and show the result on the LCD. */
    while (1) {
        key = Keypad_ReadDebounced();       /* NO_KEY (0xFF) when idle */

        if (key == NO_KEY) {
            /* Key released: reset the tracking variable so the SAME key can
             * be registered again on the next press.  0xFF itself is never
             * stored in the buffer.                                        */
            last_key = NO_KEY;
            held_ms  = 0U;
        }
        else if (key != last_key) {
					char display[2];
				
            /* A different key than last time -> a new press. Register once. */
           last_key = key;
           held_ms  = 0U;
					display[0]=key;
					display[1]='\0';
					ssd1306_Fill(Black);
					ssd1306_SetCursor(0, 0);
					ssd1306_WriteString(display, Font_11x18, White);
					ssd1306_UpdateScreen();
					
            //Register_Key((char)key);
        }
        else {
            /* Part E: same key still held.  Count how long, and re-enter it
             * every AUTO_REPEAT_MS.  (One scan + debounce + pause is roughly
             * DEBOUNCE_MS + SCAN_PERIOD_MS, so the interval is approximate.) */
            held_ms += DEBOUNCE_MS + SCAN_PERIOD_MS;
            if (held_ms >= AUTO_REPEAT_MS) {
                held_ms = 0U;
                Register_Key((char)key);
            }
        }

        waitms(SCAN_PERIOD_MS);
    }
}