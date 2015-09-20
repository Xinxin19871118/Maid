#include "flash.h"

#include "stm32f4xx_flash.h"

// Ҫ�ر�ע�⣬Flashÿ��sector��С�ǲ�һ�µ�
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

#define START_USER_ADDR   ADDR_FLASH_SECTOR_2   /* Start @ of user Flash area */
#define END_USER_ADDR     ADDR_FLASH_SECTOR_5   /* End @ of user Flash area */

#define STORE_DATA32  0x5a5a5a00

// ��ɵ�Ƶİ�.......
int GetSector(int Address)
{
  int sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_Sector_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_Sector_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_Sector_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_Sector_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_Sector_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_Sector_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_Sector_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_Sector_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_Sector_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_Sector_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_Sector_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_Sector_11;  
  }

  return sector;
}

void flash_demo(void)
{
    int start_sector,end_sector;
    static int error_times = 0;
    int i;
    int addr;
    
    /*
    FLASH_Unlock();       // ���Ĵ������������ӣ�ֱ�ӵ�������⼴�ɣ�
      
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);     //ͬ��

    start_sector = GetSector(START_USER_ADDR);   //���㷨����Ҫsector��ʼ��ַ��
    end_sector = GetSector(END_USER_ADDR);    
   
    for (i = start_sector; i < end_sector; i += 8)  // ֮���Լ�8,��FLASH_EraseSector()��һ�������ĵ����й�ϵ
    {
      if(FLASH_EraseSector(i, VoltageRange_3) != FLASH_COMPLETE)     // VoltageRange3 ���� 2.7V-3.6V
          while (1);                                                    // ���ԣ�������Ҫ����һ���㹻�Ŀռ䣬Ų������������ôɾ������
    } 
    
    addr = START_USER_ADDR;
    
    while (addr < END_USER_ADDR)
    {
      if (FLASH_ProgramWord(addr, STORE_DATA32) == FLASH_COMPLETE)
          addr +=4;
      else
          while(1);
    }
    
    FLASH_Lock();
    */
    
    addr = START_USER_ADDR;
    error_times = 0;
    
    while (addr < END_USER_ADDR)
    {
        if (*(int *)addr != STORE_DATA32)
            error_times++;

        addr += 4;
    }    
}

//------------------------

#define FLASH_LOWEST_ADDR  0x08000000
#define FLASH_HIGHEST_ADDR 0x080FFFFF

// ���Զ������Ǽ���򵥵ģ�����û���κ�����

int read_flash_32(int addr)
{
    if( (addr > FLASH_HIGHEST_ADDR) || (addr < FLASH_LOWEST_ADDR) )
        return -1;
    
    return (*(int *)addr);
}

// ��������˼·
// 1.�ṩ�����ȡ��
// 2.����FIFOһ��ʹ�ã�
int write_flash_32(int addr,int value)
{
    int sector;
    
    if( (addr > FLASH_HIGHEST_ADDR) || (addr < FLASH_LOWEST_ADDR) )
        return -1;    
    
    FLASH_Unlock();       
      
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
    
    sector = GetSector(addr); 
    
    // ����sector���������ݣ�ת����ָ������ʱtemp
    
}




// end of file -----------------------------------------------------------------

