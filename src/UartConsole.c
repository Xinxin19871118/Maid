/*
    ��Ϊһ��������Ĵ��ڣ���Դ�ļ�ֱ�ӷ��ʵ�Ƭ����ͨ������߼Ĵ�������
    ͬʱ���ض��򵽱�׼������������������ԣ�����Ϊ��������Ļ�����ʩ
*/

#include "UartConsole.h"

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"

#include <stdio.h>

// �����ض��� ----------------
int fputc(int ch,FILE *f)
{
    while(USART_GetFlagStatus(UART5,USART_FLAG_TXE) == RESET);  
    USART_SendData(UART5,ch);
    return ch;
}
// end of�����ض��� -----------

// ���ڲ��� ----
void uart5_initial(void)
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef uart;
    
  // rcc enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  // gpio initial
  // UART4 TX PC12 
  gpio.GPIO_Pin = GPIO_Pin_12;
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_Init(GPIOC, &gpio);

  // UART4 RX PD2
  gpio.GPIO_Pin = GPIO_Pin_2;
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  gpio.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_Init(GPIOD, &gpio);
	
  // Connect USART pins to AF 
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_UART5);  
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_UART5);  

  // usart
  uart.USART_BaudRate = 9600;
  uart.USART_WordLength = USART_WordLength_8b;
  uart.USART_StopBits = USART_StopBits_1;
  uart.USART_Parity = USART_Parity_No;
  uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

  uart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(UART5, &uart);

  USART_Cmd(UART5, ENABLE);  
}

// end of ���ڲ��� -------



// end of file -------------------

