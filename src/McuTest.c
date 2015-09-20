/*
    一些用于测试MCU特性的有用小函数
    如：
    系统时钟频率
*/

#include "McuTest.h"

#include "stm32f4xx_rcc.h"

// small test for stm32 system freq
void test_system_clock_freq(void)
{
  static unsigned char source_kind;
  static RCC_ClocksTypeDef clock;
  
  source_kind = RCC_GetSYSCLKSource();  // PLL 0x08
  RCC_GetClocksFreq(&clock);        // predefine HSE_VALUE as the real hardware
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t* file, uint32_t line)
{ 
  while (1);
}
#endif


// end of file -------------------


