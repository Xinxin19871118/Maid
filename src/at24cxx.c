/*
    ������eeworld�󲹳䣺

    at24cxx ϵ�в�ͬ������Ƭ�� �ںܶ�����ĵط����ڲ���,����
    1.�豸��ַ A0 A1 A2��
    2.�洢��ַ���ֽ�����
    3.��ͬ�����ĵ�ַ��Χ��ҳд��󳤶ȣ�
    ���⣬����Ҫ������һЩ�����־λ����Ȼ���� no-ack���д����Ҫ��������������Ŀǰ�뵽��
������д��ַ��Χ����ЩӦ��λ��¼��
    
    ��Щ�������϶��Ͳ�ͬ���� �ͺ� �йأ������Դ����ֻ���ǵ���ͬ������ʱ����ʱ�ϵĲ������
����û���ǵ������������ƣ�
    ͬʱ����ʼ������Ӧ����һ�����ͺ��йص��βΣ�
*/

/*
    �ο������ĵ��Լ� �����ֹٷ�2014 i2c�ĵ�����i2c�����¼򵥼���˵����
    1.ʱ���ߡ������߽�Ϊ��ʱ����ζ�����߿��У�
    2.���Դ�ļ�ʵ�ֵ�i2c���߶�д���������Ƕ�������ͻ���ٲõ����Σ����ṩһ����ӵĴ���
���ԣ��ڿ�ʼ�ź�֮��ʱ����Ϊ�ͣ���ռס���ߣ�����ֻ�ڽ����ź�֮�󣬲Żָ� ���߿��У�
��ʼ״̬��(���ʵĻ�����Ϊÿ��������Ľ���״̬)
    3.��Ϊ ��ʼ�źźͽ����ź� ���� ʱ���߸�ʱ�� ����������ʶ���ʴˣ���Ҫͨ��������������
ʱ�����ڲ�ͬ�Ĳ�������ܵĴ��ڸߵ�ƽʱ�������ʱ�����ߵĵ�ƽ�仯�󴥷� ��ʼ�������źţ�
���ı��ȥĬ����ÿ����������ʱ���߶������ڵ���ΪĬ��״̬��
    4.��һ����ӵ�����£�����Ҫ����ͬһ��i2c�豸����ͬ����ͬʱ����������ʹ��ǰi2c�豸
����һ����д�����У�������æ��״̬�£�Ҳ����ͨ�����·���һ�ο�ʼ�ź����¿�ʼһ���µĲ�����
    ����ǰ������ݶ�дʧ�ܣ����ϣ����Ѿ�����ͨ��i2c�������ٲ��ˣ���������Ϊ�˱�����̶�д
������ͻ��ֻ��ͨ���ź����������ϲ��������֤�ˡ�

*/
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"


#include "at24cxx.h"

// ΢���ʱ���˴���һЩ��֣��������֣��Ұ�iѭ���������ٴμ�ǧ�κ󣬷�����ʱʱ��ı仯
// ��������ֵ�仯���������Ա仯����ԭ��δ��������Ҫ�鵽��༶��������Ҳ������̫��ë��

void delay(void)
{
    int i;
    
    for(i = 0;i < 25;i++)
        __ASM("NOP");
}

#define _1us {delay();}

#define _5us {_1us;_1us;_1us;_1us;_1us;}
#define _10us {_5us;_5us;}

// ʱ����ʱ   
// ���ڲ��Թ��ߣ���������û�е�1us���µĲ��ԣ����⣬����Ӳ����û������at24cxx����������Ҳ
// û���������ͺš�ֻ�������ú귽�������޸ģ����õ���ʱ�����ϸ���ԣ�
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


// end of ΢���ʱ ----------------------------


// BSRRL��BSRRH�������ˣ���

#define I2C1_SCK_OUT    {GPIOB->MODER &= 0xfffcffff;GPIOB->MODER |= 0x00010000;}
#define I2C1_SCK_HIGH   GPIOB->BSRRL |= 0x0100
#define I2C1_SCK_LOW    GPIOB->BSRRH |= 0x0100

#define I2C1_SDA_OUT    {GPIOB->MODER &= 0xffff3fff;GPIOB->MODER |= 0x00004000;}
#define I2C1_SDA_IN      GPIOB->MODER &= 0xffff3fff
#define I2C1_SDA_HIGH    GPIOB->BSRRL |= 0x0080
#define I2C1_SDA_LOW     GPIOB->BSRRH |= 0x0080
#define I2C1_SDA_STATUS (GPIOB->IDR & 0x0080)

// �Խ��ղ�����Ӧ����м������Ա����ü򵥵�д�ֽں���ֱ�ӷ��������Ϣ�����ڷ����˴��������
// ͨ�����������ѯ���������ֽڲ���������飻
// �ɼ�д�ֽڹ��̵ļ�� Ҳ���ڸ��٣�
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
    
    GPIOB->OSPEEDR |= 0x003c000;     // pin 8 7����Ϊ 0x11;100Mhz
    GPIOB->OTYPER &= 0xfffffe7f;    // pin 8 7,��Ϊ 0��    Ĭ�����ģʽ
    GPIOB->PUPDR &= 0xfffc3fff;     // pin 8 7����Ϊ 0x00; ����������
}

//------------------------------------------------------------------------------


void i2c1_init(void)
{					     
    i2c1_gpio_init();
    I2C1_SCK_HIGH;  
    I2C1_SDA_HIGH;
}

// Ӧȷ�������漰��SDAΪ����״̬�Ĳ�����󶼻ָ���Ĭ�����״̬
void i2c1_start(void)
{
    I2C1_SDA_HIGH;

    I2C1_SCK_HIGH;   
    SETUP_START;
    
    I2C1_SDA_LOW;
    HOLD_START;
}	  

//����IICֹͣ�ź�
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

    I2C1_SDA_IN;        //�л�����������
    I2C1_SCK_HIGH;      //�������� �ȴ�ACK
 
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
    
    i2c1_ack_lack_times = 0;    // һ����һ��Ӧ��������ñ�־
	I2C1_SCK_LOW; 	 //�ɹ��յ�slave��Ӧ��ACK,����ʱ�ӽ���
    I2C1_SDA_OUT;
} 

//����ACKӦ��
void i2c1_ack(void)
{
    I2C1_SCK_LOW;  
    SETUP_DATA;    
      
    I2C1_SDA_OUT;
    I2C1_SDA_LOW;   // Ӧ��
    HOLD_DATA;
    
	I2C1_SCK_HIGH;  // ����ʱ����������Ч
    HIGH_TIME;
	I2C1_SCK_LOW;   //Ӧ���� �����ź�����й�
}

//������ACKӦ��		    
void i2c1_nack(void)
{
    I2C1_SCK_LOW;   
    SETUP_DATA;    
      
    I2C1_SDA_OUT;
    I2C1_SDA_HIGH;  // ��Ӧ��
    HOLD_DATA;
    
	I2C1_SCK_HIGH;  // ����ʱ����������Ч
    HIGH_TIME;
	I2C1_SCK_LOW;  //Ӧ���� �����ź�����й�
}					 				     
	  
void i2c1_send_byte(char txd)
{                        
    char t;   
    
	I2C1_SCK_LOW;
    I2C1_SDA_OUT;
    
    // ʱ�ӵ͵�ƽʱ��
    
    for(t=0;t<8;t++)
    {      
        if( (txd&0x80)>>7 == 1)
          I2C1_SDA_HIGH;
        else
          I2C1_SDA_LOW;
        
        txd<<=1; 
        
        SETUP_DATA;
        
		I2C1_SCK_HIGH;      // ����������Ч
        HIGH_TIME;
        I2C1_SCK_LOW;       // ������һλ��
        HOLD_DATA;        
    }	
    
    i2c1_wait_ack();
} 	    
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
char i2c1_read_byte(char ack)
{
	char i,receive=0;
    
    I2C1_SDA_IN;

    for(i=0;i<8;i++ )
	{
        I2C1_SCK_LOW;
        CLOCK_LOW_DATA_VALID;
		I2C1_SCK_HIGH;
        
        receive<<=1;            //���ݽ��ղ���
        if(I2C1_SDA_STATUS) 
            receive++;   
        
        DATA_HOLD;
    }	
    
    if (!ack)
        i2c1_nack();//����nACK
    else
        i2c1_ack(); //����ACK   
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
     
     // ȱһ�����ҳд����
     
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
     
     // ����ֻ�д洢����С��Χ
     
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

