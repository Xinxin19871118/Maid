#include "Delay.h"

#include "stm32f4xx.h"

void delay_ms(int ms)
{
    int i = 0,j;
    
    while(i < ms)
    {
      i++;
      for(j = 0;j < 0x5c00;j++)
          __ASM("NOP");
    }
}

// end of file -------------------

