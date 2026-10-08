#ifndef __STM32L476G_KEYPAD_H
#define __STM32L476G_KEYPAD_H

#include "stm32l476xx.h"

void Keypad_Pin_Init(void);
unsigned char keypad_scan(void);
void waitms(unsigned int ms);

#endif /* __STM32L476G_KEYPAD_H */
