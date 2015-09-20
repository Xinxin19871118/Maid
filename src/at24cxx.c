/*
    发布到eeworld后补充：

    at24cxx 系列不同容量的片子 在很多操作的地方存在差异,比如
    1.设备地址 A0 A1 A2；
    2.存储地址的字节数；
    3.不同容量的地址范围，页写最大长度；
    另外，还需要多增加一些错误标志位，既然采用 no-ack这个写法就要继续，比如至少目前想到的
超出读写地址范围，这些应以位记录；
    
    这些，基本上都和不同容量 型号 有关，而这个源码中只考虑到不同容量的时序延时上的差别，其他
都还没考虑到，待后续完善；
    同时，初始化函数应增加一个和型号有关的形参；
*/

/*
    参考各类文档以及 飞利浦官方2014 i2c文档，对i2c有以下简单几点说明：
    1.时钟线、数据线皆为高时，意味着总线空闲；
    2.这个源文件实现的i2c总线读写操作不考虑多主机冲突和仲裁的情形，仅提供一主多从的处理；
所以，在开始信号之后，时钟线为低，以占住总线，并且只在结束信号之后，才恢复 总线空闲；
初始状态：(合适的话，作为每个动作后的结束状态)
    3.因为 开始信号和结束信号 都以 时钟线高时的 脉冲沿来标识，故此，需要通过代码上来避免
时钟线在不同的操作后可能的处于高电平时，避免此时数据线的电平变化误触发 开始、结束信号，
而改变过去默认让每个动作过后，时钟线都保持在低作为默认状态；
    4.在一主多从的情况下，不需要担心同一个i2c设备被不同进程同时操作——即使当前i2c设备
处在一个读写过程中，即总线忙的状态下，也可以通过重新发起一次开始信号重新开始一次新的操作；
    至于前面的数据读写失败，作废，这已经不能通过i2c本身来仲裁了，这种情形为了避免进程读写
操作冲突，只能通过信号锁等其他上层机制来保证了。

*/
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"


#include "at24cxx.h"

// 微妙级延时：此处有一些奇怪，我曾发现，我把i循环调到几百次几千次后，发现延时时间的变化
// 不符合数值变化（不是线性变化），原因未明，恐怕要查到汇编级，但看了也看不出太大毛病

void delay(void)
{
    int i;
    
    for(i = 0;i < 25;i++)
        __ASM("NOP");
}

#define _1us {delay();}

#define _5us {_1us;_1us;_1us;_1us;_1us;}
#define _10us {_5us;_5us;}

// 时序延时   
// 限于测试工具，所以这里没有到1us以下的测试；另外，由于硬件上没有其他at24cxx器件，所以也
// 没测试其他型号。只是这里用宏方便管理和修改，待用到的时候可仔细测试；
                                           //512 pdf (1.7V/2.5V)
#define SETUP_START  _1us                  //0.6us/0.25us(min)
#define HOLD_START   _1us                  //0.6us/0.25us(min)

#define SETUP_STOP   _1us;                 //0.6us/0.25us(min)

#define SETUP_DATA   _1us;                 //0.6us/0.25us(min)
#define HOLD_DATA    {}                    //0

#define CLOCK_LOW_DATA_VALID _1us;         //0.05~0.9us/0.05~0.55us
#define DATA_HOLD    _1us;                 //50ns(min)
#define HIGH_TIME    _1us;                 //0.6us/0.4us
#define LOW_TIME     {_1us;_1us;}          //1.3us/0.4us

// tBuf min between start and stop 1.3us/0.5us


// end of 微妙级延时 ----------------------------


// BSRRL和BSRRH反过来了？？

#define I2C1_SCK_OUT    {GPIOB->MODER &= 0xfffcffff;GPIOB->MODER |= 0x00010000;}
#define I2C1_SCK_HIGH   GPIOB->BSRRL |= 0x0100
#define I2C1_SCK_LOW    GPIOB->BSRRH |= 0x0100

#define I2C1_SDA_OUT    {GPIOB->MODER &= 0xffff3fff;GPIOB->MODER |= 0x00004000;}
#define I2C1_SDA_IN      GPIOB->MODER &= 0xffff3fff
#define I2C1_SDA_HIGH    GPIOB->BSRRL |= 0x0080
#define I2C1_SDA_LOW     GPIOB->BSRRH |= 0x0080
#define I2C1_SDA_STATUS (GPIOB->IDR & 0x0080)

// 对接收不到的应答进行计数，以避免让简单的写字节函数直接返回这个信息，可在发现了错误操作后
// 通过这个函数查询，这是在字节层面上做检查；
// 可简化写字节过程的检查 也利于高速；
static int i2c1_ack_lack_times = 0;

int is_i2c1_ack_not_found(void)
{
    return i2c1_ack_lack_times;
}


void i2c1_gpio_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
    
    I2C1_SCK_OUT;
    I2C1_SDA_OUT;
    
    GPIOB->OSPEEDR |= 0x003c000;     // pin 8 7，设为 0x11;100Mhz
    GPIOB->OTYPER &= 0xfffffe7f;    // pin 8 7,设为 0；    默认输出模式
    GPIOB->PUPDR &= 0xfffc3fff;     // pin 8 7，设为 0x00; 无上拉下拉
}

//------------------------------------------------------------------------------


void i2c1_init(void)
{					     
    i2c1_gpio_init();
    I2C1_SCK_HIGH;  
    I2C1_SDA_HIGH;
}

// 应确保所有涉及改SDA为输入状态的操作完后都恢复成默认输出状态
void i2c1_start(void)
{
    I2C1_SDA_HIGH;

    I2C1_SCK_HIGH;   
    SETUP_START;
    
    I2C1_SDA_LOW;
    HOLD_START;
}	  

//产生IIC停止信号
void i2c1_stop(void)
{
    I2C1_SDA_LOW;
 
    I2C1_SCK_HIGH; 
    SETUP_STOP;
    I2C1_SDA_HIGH;	
}

void i2c1_wait_ack(void)
{
	char time=0;

    I2C1_SDA_IN;        //切换回数据输入
    I2C1_SCK_HIGH;      //拉高总线 等待ACK
 
	while(I2C1_SDA_STATUS)
	{
		time++;
		if(time>250)
		{
            if(i2c1_ack_lack_times < i2c1_ack_lack_times+1)
                i2c1_ack_lack_times++;
            return;
		}
	}
    
    i2c1_ack_lack_times = 0;    // 一旦有一次应答，则清除该标志
	I2C1_SCK_LOW; 	 //成功收到slave回应的ACK,拉低时钟结束
    I2C1_SDA_OUT;
} 

//产生ACK应答
void i2c1_ack(void)
{
    I2C1_SCK_LOW;  
    SETUP_DATA;    
      
    I2C1_SDA_OUT;
    I2C1_SDA_LOW;   // 应答
    HOLD_DATA;
    
	I2C1_SCK_HIGH;  // 拉高时钟线数据有效
    HIGH_TIME;
	I2C1_SCK_LOW;   //应该与 结束信号配合有关
}

//不产生ACK应答		    
void i2c1_nack(void)
{
    I2C1_SCK_LOW;   
    SETUP_DATA;    
      
    I2C1_SDA_OUT;
    I2C1_SDA_HIGH;  // 不应答
    HOLD_DATA;
    
	I2C1_SCK_HIGH;  // 拉高时钟线数据有效
    HIGH_TIME;
	I2C1_SCK_LOW;  //应该与 结束信号配合有关
}					 				     
	  
void i2c1_send_byte(char txd)
{                        
    char t;   
    
	I2C1_SCK_LOW;
    I2C1_SDA_OUT;
    
    // 时钟低电平时间
    
    for(t=0;t<8;t++)
    {      
        if( (txd&0x80)>>7 == 1)
          I2C1_SDA_HIGH;
        else
          I2C1_SDA_LOW;
        
        txd<<=1; 
        
        SETUP_DATA;
        
		I2C1_SCK_HIGH;      // 拉高数据有效
        HIGH_TIME;
        I2C1_SCK_LOW;       // 允许下一位变
        HOLD_DATA;        
    }	
    
    i2c1_wait_ack();
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
char i2c1_read_byte(char ack)
{
	char i,receive=0;
    
    I2C1_SDA_IN;

    for(i=0;i<8;i++ )
	{
        I2C1_SCK_LOW;
        CLOCK_LOW_DATA_VALID;
		I2C1_SCK_HIGH;
        
        receive<<=1;            //数据接收操作
        if(I2C1_SDA_STATUS) 
            receive++;   
        
        DATA_HOLD;
    }	
    
    if (!ack)
        i2c1_nack();//发送nACK
    else
        i2c1_ack(); //发送ACK   
    return receive;
}

//------------------------------------------------------------------------------

void at24cxx_byte_write(short Add,char Byte)
{
     i2c1_start();
     
     i2c1_send_byte(0xAE);

     i2c1_send_byte((char)((Add>>8)&0xff));
     i2c1_send_byte((char)((Add>>0)&0xff));    
     i2c1_send_byte(Byte);

     i2c1_stop();
}


void at24cxx_page_write(short Add,char *data,int len)
{
     int i;
     
     i2c1_start();
     
     i2c1_send_byte(0xAE);

     i2c1_send_byte((char)((Add>>8)&0xff));
     i2c1_send_byte((char)((Add>>0)&0xff));
     
     // 缺一个最大页写限制
     
     for(i = 0;i < len;i++)
        i2c1_send_byte(data[i]);

     i2c1_stop();    
}


char at24cxx_byte_read(short Add)
{
     char Recv = 0;
     
     i2c1_start();
     
     i2c1_send_byte(0xAE);
     
     i2c1_send_byte((char)((Add>>8)&0xff));
     i2c1_send_byte((char)((Add>>0)&0xff));
     
     i2c1_start();
     i2c1_send_byte(0xAF);
     
     Recv = i2c1_read_byte(0);
     
     i2c1_stop();
     
     return Recv;
}

int at24cxx_sequence_read(short Add,char *buff,int len)
{
     int i;
     
     i2c1_start();
     
     i2c1_send_byte(0xAE);
     
     i2c1_send_byte((char)((Add>>8)&0xff));
     i2c1_send_byte((char)((Add>>0)&0xff));
     
     i2c1_start();
     i2c1_send_byte(0xAF);
     
     // 限制只有存储器大小范围
     
     do
     {
        buff[i] = i2c1_read_byte(1);
        i++;
     }while(i<len - 1);
     
     buff[i] = i2c1_read_byte(0);
     
     i2c1_stop();
     
     return i;
}


//------------------------------------------------------------------------------
static void delay_ms(void)
{
    int i;
    
    for(i = 0;i < 1000000;i++)
        __ASM("NOP");
}

// test 

#include <stdio.h>

void i2c1_gpio_test(void)
{
    char u;
    char t[3] = {0x04,0x33,0x14}; 
    char r[3];
  
    at24cxx_page_write(0x0001,t,3);
    delay_ms();
    at24cxx_byte_write(0x0004,0x34);
    delay_ms();
    at24cxx_byte_write(0x0005,0x03);
      
    delay_ms();
    
    u = at24cxx_byte_read(0x0033);
    delay_ms();
    u = at24cxx_byte_read(0x0001);
    delay_ms();
    at24cxx_sequence_read(0x0002,r,4);
    delay_ms();
    u = at24cxx_byte_read(0x0004); 
    
    printf("read out %d\n",u);
}

// end of file -----------------------

