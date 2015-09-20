/*
	结论：这套东西，分两层
	1.顶层是一个处理自己id系统，即从id到offset之类的偏移量的运算；
	2.底层也是一层调用 fs_open,fs_close,fs_write,fs_read 函数的操作；就和普通C语言文件操作一样；
	3.fs_open等的操作，是建立在 任务锁 一类的操作 和 sfs_put,sfs_get 的操作。
	
	我想，这类似于标准C库的操作，所以我想知道，标准C里这里到底是怎么回事。
	是否可以类似于printf等重定向简化问题？
*/

最终调用的 
gt_para_process()

//----------------------------------
下列函数需要使用的东西

// mem_map_st 结构体定义
typedef struct
{
	U16 bFlag;
	U16 size;
	U16 crc;
	U16 bModify;
	U16 met;
	U16 nChkSec;
	U16 nChkCnt;
	U16 wCnt;
	U16 rCnt;
	U8* pbuf;
}mem_map_st;

// sys_map_st 结构体定义
typedef struct
{
	int (*load)(U16);
	int (*save)(U8);
	const t_sysTable* table;
	U16 tSize;
	mem_map_st para;
}sys_map_st;

// 定义下来使用的结构体，0元是 sys，1元是met（这应该是个误名，应指其他的用户参数）
sys_map_st sys_map[2]=
{
//    *load       ,  *save,       *table     ,   tSize,                               para
	{sys_load_para,sys_save_para,sys_di_table,sizeof(sys_di_table)/sizeof(t_sysTable),{0}},
	{met_load_para,met_save_para,met_di_table,sizeof(met_di_table)/sizeof(t_sysTable),{0}},
};

// 这两个函数
// sys_load_para(met_load_para) 和 sys_save_para(met_save_para)

// t_sysTable的结构体定义
typedef struct
{
	uint32 di;          //数据标识
	uint16 group;		//组号
	uint16 size;		//数据大小
	uint16 offset;		//偏移量(0为自动)
	uint16 ver_code;	//参数版本
	uint8  save_type;	//写入方法：0,定时，1,次数，2,立即写入，3不处理
	uint8  gat;         //数据集合
	uint8* pVal;		//初始值
	uint8  r;
	uint8  w;
	uint8  rsv[6];		//预留－使用者自行定义
}t_sysTable;


函数定义
int gt_para_process (U32 sno,U32 di,U8* pbuf,int len,U8 rw,U8 force)
{
	U32 offset=0;
	sys_map_st *pMap;
	const t_sysTable *pt;
	int itemlen, ret;	

	if(sno)
		pMap=&sys_map[1];	
	else 
		pMap=&sys_map[0];

	pt=pMap->table;			// t_sysTable
	itemlen=pMap->tSize;	// table数

	while (itemlen--)		// 这里貌似是根据table数，计算偏移量
	{
		if(pt->offset)
			offset=pt->offset;
		if (pt->di==di)
			if(pt->ver_code==0)break;
		offset+=pt->size;
		pt++;
	}

	if(itemlen < 0)
		return 0;
	if(len > pt->size)	// 确定len长度，
		len=pt->size;

	ret = para_lock(sno?0:1);	// 参数锁操作
	
	if(!pMap->load(sno))
		len=0;
	else
	{
		offset+=(sno%MET_MAX_ITEM)*MET_ITEM_SIZE;		// 确定实际操作的 偏移位置
		
		if(rw=='w'||rw=='W')	// 写操作
		{
			if (ret == FALSE)
				return 0;
			if(memcmp(pMap->para.pbuf+offset,pbuf,len)!=0)	// 那，这其实就只是一个地址而已，para.pbuf一般是0，那么，这个memcpy操作？
			{
				memcpy(pMap->para.pbuf+offset,pbuf,len);
				pMap->para.crc=CHECK_CRC(pMap->para.pbuf,pMap->para.size);
				pMap->para.bModify=1;
				switch(pt->save_type)
				{
					case 0:pMap->para.nChkSec=3;break;	//定时写				
					case 1:pMap->para.nChkCnt=10;break;	//计数写
					case 2:pMap->save(1);break;			//立即写
					default:break;						//不写入			
				}
				//gt_event_write(0xE2010014, sno, &di,4, 0);	
// 				if((di&0xFF000000)==0xE0000000)
// 				{
// 					add_event_reg(sno,0xE2010014,&di,4);
// 				}
// 				else 
				if(di!=0xF1000000  && sno)
				{
					//msg_out("\n<met%d,di:%08x>:",sno,di);
					//msg_trace(pbuf,len);
				}
				if(sno==0&&di!=0xF1008001)
				{
					//msg_out("\n[ter di:%08x]:",di);
					//msg_trace(pbuf,len);
				}
			}
		}
		else	// 读操作咯？
		{
			memcpy(pbuf,pMap->para.pbuf+offset,len);
		}
	}
	para_unlock(sno?0:1);
	return len;
}

//---------------------------
static int sys_load_para(U16 sno)
{
	static int cnt=0, init = 0x38732560;
	if(sys_mem.bFlag==0x5A)
		if(sys_mem.pbuf!=NULL && sys_mem.size <= SYS_BUF_SIZE)
		{
			if (++cnt < 100)
				return 1;
			cnt = 0;
			if(sys_mem.crc==CHECK_CRC(sys_mem.pbuf,sys_mem.size))
				return 1;
		}
	//msg_out("\nLoad sys.bin");
	if(sys_mem.pbuf==NULL)sys_mem.pbuf=(U8*)mem_sys;//malloc(SYS_BUF_SIZE);
	if(sys_mem.pbuf!=NULL)
	{
		FHANDLE fp;
		fp=FOPEN("sys.bin","r");
		if(fp)
		{
			FREAD(sys_mem.pbuf,SYS_BUF_SIZE,1,fp);
			FCLOSE(fp);
		}
		else
		{
			if (init == 0x38732560)			// 只允许系统上电时创建文件
			{
				FS_MSG("\nCreate sys.bin");
				fp=FOPEN("sys.bin","w");
				if(fp)
				{
					memset(sys_mem.pbuf,0xFF,SYS_BUF_SIZE);
					FWRITE(sys_mem.pbuf,SYS_BUF_SIZE,1,fp);
					FCLOSE(fp);
				}
			}
		}
		sys_mem.rCnt++;
		sys_mem.size=SYS_BUF_SIZE;
		sys_mem.crc=CHECK_CRC(sys_mem.pbuf,sys_mem.size);
		sys_mem.bFlag=0x5A;
		sys_mem.met=0;	
		init = 0;
		return 1;
	}
	return 0;
}
static int sys_save_para(U8 bForce)
{
	if(sys_mem.bModify)
	{
		if(sys_mem.nChkSec==1 || sys_mem.nChkCnt==1 || bForce)
		{
			FHANDLE fp;
			fp=FOPEN("sys.bin","w");
			//if(fp==NULL)fp=FOPEN("sys.bin","wb");		// 系统出错时会导致丢参数, 2014.2.18去掉
			if(fp)
			{
				if(sys_mem.pbuf!=NULL)
					FWRITE(sys_mem.pbuf,SYS_BUF_SIZE,1,fp);
				FCLOSE(fp);
// 				sys_mem.crc=CHECK_CRC(sys_mem.pbuf,sys_mem.size);
				sys_mem.wCnt++;
				sys_mem.bModify=0;
// 				return 1;
			}
			sys_mem.nChkSec=0;
			sys_mem.nChkCnt=0;
		}
		if(sys_mem.nChkSec)sys_mem.nChkSec--;
// 		if(sys_mem.nChkCnt)sys_mem.nChkCnt--;
	}
	return 1;
}

// 真正读写操作在这里，这套系统类似于HMI，也是回调函数
//=====================================================================================
static int met_load_para(U16 sno)
{
	if(met_mem.bFlag==0x5A)
		if(met_mem.pbuf!=NULL && met_mem.size <= MET_BUF_SIZE)
			if(met_mem.crc==CHECK_CRC(met_mem.pbuf,met_mem.size))
			{
				if(!sno || (met_mem.met<=sno&&sno<(met_mem.met+MET_MAX_ITEM)))
					return 1;
				met_save_para(1);
			}
// 	msg_out("\nLoad met.bin");
	if(met_mem.pbuf==NULL)met_mem.pbuf=(U8*)mem_met;//malloc(MET_BUF_SIZE);
	if(met_mem.pbuf!=NULL)
	{
		FHANDLE fp;
		memset(met_mem.pbuf,0xFF,MET_BUF_SIZE);
		fp=FOPEN("met.bin","r");		//原来如此
		if(fp)
		{
			met_mem.met=(sno/MET_MAX_ITEM)*MET_MAX_ITEM;
			//FSEEK(fp,met_mem.met*MET_ITEM_SIZE,SEEK_SET);
			FREAD(met_mem.pbuf,MET_BUF_SIZE,1,fp);
			FCLOSE(fp);
		}
		else
		{
			//msg_out("\nCreate met.bin");
			fp=FOPEN("met.bin","w");
			if(fp)
			{
				FWRITE(met_mem.pbuf,MET_BUF_SIZE,1,fp);
				FCLOSE(fp);
			}
		}
		met_mem.rCnt++;			
		met_mem.met=(sno/MET_MAX_ITEM)*MET_MAX_ITEM + 1;
		met_mem.size=MET_BUF_SIZE;
		met_mem.crc=CHECK_CRC(met_mem.pbuf,met_mem.size);
		met_mem.bFlag=0x5A;
		return 1;		
	}
	return 0;
}
static int met_save_para(U8 bForce)
{
	if(met_mem.bModify)
	{
		if(met_mem.nChkSec==1 || met_mem.nChkCnt==1 || bForce)
		{
			FHANDLE fp;
			fp=FOPEN("met.bin","w");
			if(fp==NULL)fp=FOPEN("met.bin","wb");
			if(fp)
			{
				//FSEEK(fp,met_mem.met*MET_ITEM_SIZE,SEEK_SET);
				if(met_mem.pbuf!=NULL)
					FWRITE(met_mem.pbuf,MET_BUF_SIZE,1,fp);
				FCLOSE(fp);
				met_mem.wCnt++;
				met_mem.bModify=0;
			}
			met_mem.nChkSec=0;
			met_mem.nChkCnt=0;
		}
		if(met_mem.nChkSec)met_mem.nChkSec--;
	}
	return 1;
}

// 它的 fs_open，fs_close,fs_read,fs_write等 这一套都是自己实现的。
// 这套东西属于 加了 任务锁 等操作的 xx_get,xx_put,我认为它和C里面的 put,get是一个层面的东西。
// sfs_get sfs_put这个我看不到。
