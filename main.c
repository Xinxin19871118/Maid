#include <stdio.h>

#include "UartConsole.h"
#include "Led.h"

#include "flash.h"
#include "at24cxx.h"
#include "delay.h"

#include "FreertosUser.h"


int main(void)       
{
    led_init();
    uart5_initial();
    
    i2c1_init();

    i2c1_gpio_test();
    
    printf("fuck!\n");
            
    while(1)     
    {
        delay_ms(1000);
        led_toggle();
    }
}



// end of file ------------------------------------

