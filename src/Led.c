/*
    作为一个最基础的GPIO测试，此源文件直接访问单片机（通过库或者寄存器）。
*/
#include "Led.h"

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

// led部分 -----

void led_init(void)
{
  GPIO_InitTypeDef  gpio;
 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

  gpio.GPIO_Pin = GPIO_Pin_3;
  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_OType = GPIO_OType_OD;
  gpio.GPIO_Speed = GPIO_Speed_100MHz;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &gpio);
}

void Delay(__IO uint32_t nCount)
{
  while(nCount--);
}

void led_toggle(void)
{
    static int i = 0;
    
    i++;
    if(i & 0x01)
      GPIO_SetBits(GPIOE, GPIO_Pin_3);
    else
      GPIO_ResetBits(GPIOE, GPIO_Pin_3);   
}

// end of led部分

// end of file -------------------

