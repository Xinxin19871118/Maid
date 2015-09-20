/*
    ����FreeRTOS(Ŀǰֻʹ�������� ���������
    �����û����룬Ҳ��FreeRTOS���ֵ�����������λ��
*/

#include "FreertosUser.h"

#include "FreeRTOS.h"
#include "task.h"   //FreeRTOS kernel file

#include "Led.h"
#include "UartConsole.h"

#include <stdio.h>

// FreeRTOS multi task try

static void vLEDTask(void *pvParameters)
{
	while(1)
	{
        printf("led task per 50 ticks!\n");
        led_toggle();  
        vTaskDelay(50); 
	}
}

static void vSerialTask(void *pvParameters)
{
    int s;
    
    s = 0;
    
	while(1)
	{
        s++;
        printf("serial value is %d\n",s);
        vTaskDelay(500);
	}
} 


void freertos_main(void)
{
    xTaskCreate(
                vLEDTask,
                "LEDTask",
                configMINIMAL_STACK_SIZE,
                NULL,
                1,
                ( TaskHandle_t * ) NULL
             );

    xTaskCreate(
                vSerialTask,
                "SerialTask",
                configMINIMAL_STACK_SIZE,
                NULL,
                2,
                ( TaskHandle_t * ) NULL
             );
    
    vTaskStartScheduler();  // ����������
}

// end of FreeRTOS multi task try

// end of file -------------------




