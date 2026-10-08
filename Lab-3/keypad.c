/*
 * keypad.c  -  Lab 3: Keypad Scanning in C
 *
 * 4x4 keypad wiring (see Part B of the lab handout):
 *   Rows    R1..R4 -> PC0, PC1, PC2, PC3     (digital OUTPUT, driven by MCU)
 *   Columns C1..C4 -> PC4, PC10, PC11, PC12  (digital INPUT, external 2.2k pull-ups)
 *
 * Scanning idea (Textbook Ch. 15.9):
 *   Columns idle HIGH because of the external pull-ups.  If a row is driven LOW
 *   and a key in that row is pressed, the key connects the row to its column and
 *   the column input reads LOW.  Driving one row LOW at a time therefore lets us
 *   find both the row and the column of the pressed key.
 */

#include "keypad.h"

/* Look-up table: keymap[row][col] -> key character (Part C, Part 4).
 * Row 0 is the top row of the keypad, column 0 the left-most column.
 * As specified in the handout:
 *   (row 0, col 0) -> '0'   (row 0, col 1) -> '1'   (row 1, col 1) -> '5'
 * i.e. keys are numbered 0..F reading left-to-right, top-to-bottom. */
static const unsigned char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* Row output bits (ODR) and column input bits (IDR).
 * NOTE: the column pins are NOT consecutive (4, 10, 11, 12), so they are
 * indexed through this table instead of shifting a single 4-bit field. */
static const uint32_t rowBits[4] = { GPIO_ODR_OD0, GPIO_ODR_OD1,  GPIO_ODR_OD2,  GPIO_ODR_OD3  };
static const uint32_t colBits[4] = { GPIO_IDR_ID4, GPIO_IDR_ID10, GPIO_IDR_ID11, GPIO_IDR_ID12 };

#define ROW_MASK  (GPIO_ODR_OD0 | GPIO_ODR_OD1  | GPIO_ODR_OD2  | GPIO_ODR_OD3 )
#define COL_MASK  (GPIO_IDR_ID4 | GPIO_IDR_ID10 | GPIO_IDR_ID11 | GPIO_IDR_ID12)

/*
 * Keypad_Pin_Init
 * ---------------
 * Configure GPIOC: PC0..PC3 as outputs (rows), PC4/10/11/12 as inputs (columns).
 * Mask / value for MODER were calculated in the pre-lab.
 */
void Keypad_Pin_Init(void)
{
    /* 1. Enable the clock for GPIO Port C */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    /* 2. PC0, PC1, PC2, PC3 -> digital output (MODER = 01)
     *    GPIO Mode: Input(00), Output(01), AlterFunc(10), Analog(11, reset) */
    GPIOC->MODER &= ~(GPIO_MODER_MODE0   | GPIO_MODER_MODE1   | GPIO_MODER_MODE2   | GPIO_MODER_MODE3);
    GPIOC->MODER |=  (GPIO_MODER_MODE0_0 | GPIO_MODER_MODE1_0 | GPIO_MODER_MODE2_0 | GPIO_MODER_MODE3_0);

    /* 3. PC4, PC10, PC11, PC12 -> digital input (MODER = 00).
     *    No internal pull-up is enabled: the keypad board provides external
     *    2.2 kOhm pull-ups (the internal ~40 kOhm pull-up is too weak here). */
    GPIOC->MODER &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE10 | GPIO_MODER_MODE11 | GPIO_MODER_MODE12);

    /* 4. Row outputs as open-drain (OTYPER = 1).
     *    Open-drain rows only ever pull LOW or float, so if two keys in the
     *    same column are pressed at once, a row driven HIGH cannot short
     *    against a row driven LOW.  (Push-pull would also work for single keys.) */
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT0 | GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);

    /* Idle state: all rows driven LOW so ANY keypress pulls a column LOW. */
    GPIOC->ODR &= ~ROW_MASK;
}

/*
 * keypad_scan
 * -----------
 * Non-blocking scan of the keypad.
 *   returns  the ASCII character of the pressed key, or
 *            0xFF if no key is currently pressed.
 *
 * Because this function does not wait for a key to be pressed or released,
 * main() is responsible for registering each press only once (Part D) and
 * for debouncing.
 */
unsigned char keypad_scan(void)
{
    unsigned char row, col;

    /* ---- Part 1: Basic input reading -----------------------------------
     * Drive all four rows LOW (0b0000) and read the four column inputs.
     * With no key pressed every column is pulled HIGH by the external
     * resistors, so IDR & COL_MASK == COL_MASK.                            */
    GPIOC->ODR &= ~ROW_MASK;
    waitms(1);                                  /* let the lines settle   */

    if ((GPIOC->IDR & COL_MASK) == COL_MASK) {
        return 0xFF;                            /* no key pressed         */
    }

    /* ---- Part 3: Row determination -------------------------------------
     * Drive exactly ONE row LOW at a time: 0b0111, 0b1011, 0b1101, 0b1110.
     * Clear all row bits first, set them all HIGH, then clear the one row
     * under test.  A short delay is needed before reading the columns so
     * the input register does not still hold the previous row's result.  */
    for (row = 0; row < 4; row++) {
        GPIOC->ODR |=  ROW_MASK;                /* all rows HIGH           */
        GPIOC->ODR &= ~rowBits[row];            /* this row LOW            */
        waitms(1);

        /* ---- Part 2: Column scanning -----------------------------------
         * Check each column bit individually (bits 4, 10, 11, 12).
         * The first column that reads LOW is the pressed column.          */
        for (col = 0; col < 4; col++) {
            if ((GPIOC->IDR & colBits[col]) == 0) {
                /* ---- Part 4: Key value resolution --------------------- */
                GPIOC->ODR &= ~ROW_MASK;        /* restore idle state      */
                return keymap[row][col];
            }
        }
    }

    /* Column went LOW in Part 1 but no row/column pair was found -
     * most likely the key was released mid-scan (bounce).  Treat as no key. */
    GPIOC->ODR &= ~ROW_MASK;
    return 0xFF;
}

/*
 * waitms
 * ------
 * Crude busy-wait delay, roughly `ms` milliseconds.
 * The inner loop count is tuned for the default clock configuration used in
 * this lab; the exact value is not critical for keypad scanning.
 */
void waitms(unsigned int ms)
{
    volatile unsigned int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 4000U; j++) {
            /* spin */
        }
    }
}