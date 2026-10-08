#include "OLED_I2C.h"
#include "ssd1306_conf.h"

/* STM32 I2C CR2 stores a 7-bit address in SADD[7:1]. */
#define SSD1306_ADDR_CR2           (SSD1306_I2C_ADDR_7BIT << 1)

/*
 * I2C kernel clock is deliberately selected as HSI16 (16 MHz), independent
 * of the 80 MHz system/APB clock used by the original lab project.
 *
 * STM32L4 RM0351 example timing for fI2CCLK = 16 MHz, Standard-mode 100 kHz:
 *   PRESC  = 3
 *   SCLDEL = 4
 *   SDADEL = 2
 *   SCLH   = 0x0F
 *   SCLL   = 0x13
 * TIMINGR = 0x30420F13
 */
#define OLED_I2C_TIMING_100KHZ     0x30420F13U

/* Prevent a disconnected OLED from hanging the CPU forever. */
#define OLED_I2C_TIMEOUT_LOOPS     1000000U

static OLED_I2C_Status_t g_last_status = OLED_I2C_OK;

static OLED_I2C_Status_t OLED_I2C_WaitForFlag(uint32_t flag)
{
    uint32_t timeout = OLED_I2C_TIMEOUT_LOOPS;

    while ((I2C1->ISR & flag) == 0U) {
        if ((I2C1->ISR & I2C_ISR_NACKF) != 0U) {
            I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
            return OLED_I2C_NACK;
        }

        if ((I2C1->ISR & (I2C_ISR_BERR | I2C_ISR_ARLO)) != 0U) {
            I2C1->ICR = I2C_ICR_BERRCF | I2C_ICR_ARLOCF |
                        I2C_ICR_NACKCF | I2C_ICR_STOPCF;
            return OLED_I2C_BUS_ERROR;
        }

        if (timeout == 0U) {
            return OLED_I2C_TIMEOUT;
        }
        timeout--;
    }

    return OLED_I2C_OK;
}

static OLED_I2C_Status_t OLED_I2C_WaitBusFree(void)
{
    uint32_t timeout = OLED_I2C_TIMEOUT_LOOPS;

    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U) {
        if (timeout == 0U) {
            return OLED_I2C_TIMEOUT;
        }
        timeout--;
    }

    return OLED_I2C_OK;
}

static OLED_I2C_Status_t OLED_I2C_Write(uint8_t control,
                                        const uint8_t *data,
                                        uint16_t size)
{
    OLED_I2C_Status_t status;
    uint16_t i;
    uint16_t total_bytes;

    /* One control byte plus payload must fit NBYTES[7:0]. */
    if ((data == 0 && size != 0U) || size > 254U) {
        g_last_status = OLED_I2C_BAD_LENGTH;
        return g_last_status;
    }

    total_bytes = (uint16_t)(size + 1U);

    status = OLED_I2C_WaitBusFree();
    if (status != OLED_I2C_OK) {
        g_last_status = status;
        return status;
    }

    /* Clear stale completion/error flags before starting a transfer. */
    I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF |
                I2C_ICR_BERRCF | I2C_ICR_ARLOCF;

    /*
     * Configure a write transfer:
     *   SADD    = SSD1306 address
     *   RD_WRN  = 0 (write)
     *   NBYTES  = control byte + payload
     *   AUTOEND = generate STOP automatically after NBYTES
     */
    I2C1->CR2 = SSD1306_ADDR_CR2 |
                ((uint32_t)total_bytes << 16) |
                I2C_CR2_AUTOEND;

    I2C1->CR2 |= I2C_CR2_START;

    /* First byte tells the SSD1306 whether the payload is command or data. */
    status = OLED_I2C_WaitForFlag(I2C_ISR_TXIS);
    if (status != OLED_I2C_OK) {
        g_last_status = status;
        return status;
    }
    I2C1->TXDR = control;

    for (i = 0U; i < size; i++) {
        status = OLED_I2C_WaitForFlag(I2C_ISR_TXIS);
        if (status != OLED_I2C_OK) {
            g_last_status = status;
            return status;
        }
        I2C1->TXDR = data[i];
    }

    status = OLED_I2C_WaitForFlag(I2C_ISR_STOPF);
    if (status != OLED_I2C_OK) {
        g_last_status = status;
        return status;
    }

    I2C1->ICR = I2C_ICR_STOPCF;
    g_last_status = OLED_I2C_OK;
    return OLED_I2C_OK;
}

void OLED_I2C_Init(void)
{
    /* Ensure HSI16 is running; the I2C1 kernel clock will use it directly. */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
    }

    /* Enable GPIOB and I2C1 peripheral clocks. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* PB8 and PB9 -> Alternate Function mode (10). */
    GPIOB->MODER &= ~((3U << (8U * 2U)) | (3U << (9U * 2U)));
    GPIOB->MODER |=  ((2U << (8U * 2U)) | (2U << (9U * 2U)));

    /* I2C pins are open-drain. */
    GPIOB->OTYPER |= (1U << 8U) | (1U << 9U);

    /* Very-high GPIO speed. */
    GPIOB->OSPEEDR &= ~((3U << (8U * 2U)) | (3U << (9U * 2U)));
    GPIOB->OSPEEDR |=  ((3U << (8U * 2U)) | (3U << (9U * 2U)));

    /* Enable internal pull-ups as a backup to the pull-ups on most OLED modules. */
    GPIOB->PUPDR &= ~((3U << (8U * 2U)) | (3U << (9U * 2U)));
    GPIOB->PUPDR |=  ((1U << (8U * 2U)) | (1U << (9U * 2U)));

    /* PB8 = AF4 and PB9 = AF4. Both are in AFR[1]. */
    GPIOB->AFR[1] &= ~((0xFU << 0U) | (0xFU << 4U));
    GPIOB->AFR[1] |=  ((4U << 0U) | (4U << 4U));

    /* Reset I2C1 into a known state. */
    RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C1RST;
    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C1RST;

    /* I2C1SEL = 10 -> HSI16 clock. */
    RCC->CCIPR &= ~RCC_CCIPR_I2C1SEL;
    RCC->CCIPR |= RCC_CCIPR_I2C1SEL_1;

    /* Peripheral must be disabled before TIMINGR is changed. */
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->TIMINGR = OLED_I2C_TIMING_100KHZ;

    /* Enable I2C1. Analog filter remains enabled at reset defaults. */
    I2C1->CR1 |= I2C_CR1_PE;

    g_last_status = OLED_I2C_OK;
}

OLED_I2C_Status_t OLED_I2C_WriteCommand(uint8_t command)
{
    /* Co = 0, D/C# = 0: following byte is a command. */
    return OLED_I2C_Write(0x00U, &command, 1U);
}

OLED_I2C_Status_t OLED_I2C_WriteData(const uint8_t *data, uint16_t size)
{
    /* Co = 0, D/C# = 1: following bytes are display RAM data. */
    return OLED_I2C_Write(0x40U, data, size);
}

OLED_I2C_Status_t OLED_I2C_GetLastStatus(void)
{
    return g_last_status;
}

void OLED_DelayMs(uint32_t ms)
{
    /*
     * The original lab calls System_Clock_Init() first and runs the Cortex-M4
     * at 80 MHz. Use SysTick as a polling timer so the SSD1306 power-up delay
     * is not dependent on compiler optimization of an empty software loop.
     * No SysTick interrupt is enabled.
     */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 80000U - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while (ms != 0U) {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {
        }
        ms--;
    }

    SysTick->CTRL = 0U;
}
