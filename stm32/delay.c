#include "delay.h"

void delayuS(int us) {

    SysTick->LOAD = 16 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 0x5;
    for (int i = 0; i < us; i++)
    {
        while (!(SysTick->CTRL & 0x10000))
        {
        }
    }
    SysTick->CTRL = 0;
}

void delay(int ms) {

    SysTick->LOAD = 16000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 0x5;
    for (int i = 0; i < ms; i++)
    {
        while (!(SysTick->CTRL & 0x10000))
        {
        }
    }
    SysTick->CTRL = 0;
}
