#include "flash.h"

#include "stm32f4xx_flash.h"

// 要特别注意，Flash每个sector大小是不一致的
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

// 够傻逼的啊.......
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
    FLASH_Unlock();       // 纯寄存器操作，无视，直接调用外设库即可；
      
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);     //同上

    start_sector = GetSector(START_USER_ADDR);   //此算法，需要sector起始地址表
    end_sector = GetSector(END_USER_ADDR);    
   
    for (i = start_sector; i < end_sector; i += 8)  // 之所以加8,和FLASH_EraseSector()第一个参数的递增有关系
    {
      if(FLASH_EraseSector(i, VoltageRange_3) != FLASH_COMPLETE)     // VoltageRange3 代表 2.7V-3.6V
          while (1);                                                    // 所以，这里需要的是一个足够的空间，挪出来，否则怎么删除？！
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

// 所以读出，是极其简单的，而且没有任何限制

int read_flash_32(int addr)
{
    if( (addr > FLASH_HIGHEST_ADDR) || (addr < FLASH_LOWEST_ADDR) )
        return -1;
    
    return (*(int *)addr);
}

// 所以两种思路
// 1.提供随机存取；
// 2.类似FIFO一样使用；
int write_flash_32(int addr,int value)
{
    int sector;
    
    if( (addr > FLASH_HIGHEST_ADDR) || (addr < FLASH_LOWEST_ADDR) )
        return -1;    
    
    FLASH_Unlock();       
      
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
    
    sector = GetSector(addr); 
    
    // 读出sector的所有内容，转存至指定的临时temp
    
}




// end of file -----------------------------------------------------------------

