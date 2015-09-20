/*
	���ۣ����׶�����������
	1.������һ�������Լ�idϵͳ������id��offset֮���ƫ���������㣻
	2.�ײ�Ҳ��һ����� fs_open,fs_close,fs_write,fs_read �����Ĳ������ͺ���ͨC�����ļ�����һ����
	3.fs_open�ȵĲ������ǽ����� ������ һ��Ĳ��� �� sfs_put,sfs_get �Ĳ�����
	
	���룬�������ڱ�׼C��Ĳ�������������֪������׼C�����ﵽ������ô���¡�
	�Ƿ����������printf���ض�������⣿
*/

���յ��õ� 
gt_para_process()

//----------------------------------
���к�����Ҫʹ�õĶ���

// mem_map_st �ṹ�嶨��
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

// sys_map_st �ṹ�嶨��
typedef struct
{
	int (*load)(U16);
	int (*save)(U8);
	const t_sysTable* table;
	U16 tSize;
	mem_map_st para;
}sys_map_st;

// ��������ʹ�õĽṹ�壬0Ԫ�� sys��1Ԫ��met����Ӧ���Ǹ�������Ӧָ�������û�������
sys_map_st sys_map[2]=
{
//    *load       ,  *save,       *table     ,   tSize,                               para
	{sys_load_para,sys_save_para,sys_di_table,sizeof(sys_di_table)/sizeof(t_sysTable),{0}},
	{met_load_para,met_save_para,met_di_table,sizeof(met_di_table)/sizeof(t_sysTable),{0}},
};

// ����������
// sys_load_para(met_load_para) �� sys_save_para(met_save_para)

// t_sysTable�Ľṹ�嶨��
typedef struct
{
	uint32 di;          //���ݱ�ʶ
	uint16 group;		//���
	uint16 size;		//���ݴ�С
	uint16 offset;		//ƫ����(0Ϊ�Զ�)
	uint16 ver_code;	//�����汾
	uint8  save_type;	//д�뷽����0,��ʱ��1,������2,����д�룬3������
	uint8  gat;         //���ݼ���
	uint8* pVal;		//��ʼֵ
	uint8  r;
	uint8  w;
	uint8  rsv[6];		//Ԥ����ʹ�������ж���
}t_sysTable;


��������
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
	itemlen=pMap->tSize;	// table��

	while (itemlen--)		// ����ò���Ǹ���table��������ƫ����
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
	if(len > pt->size)	// ȷ��len���ȣ�
		len=pt->size;

	ret = para_lock(sno?0:1);	// ����������
	
	if(!pMap->load(sno))
		len=0;
	else
	{
		offset+=(sno%MET_MAX_ITEM)*MET_ITEM_SIZE;		// ȷ��ʵ�ʲ����� ƫ��λ��
		
		if(rw=='w'||rw=='W')	// д����
		{
			if (ret == FALSE)
				return 0;
			if(memcmp(pMap->para.pbuf+offset,pbuf,len)!=0)	// �ǣ�����ʵ��ֻ��һ����ַ���ѣ�para.pbufһ����0����ô�����memcpy������
			{
				memcpy(pMap->para.pbuf+offset,pbuf,len);
				pMap->para.crc=CHECK_CRC(pMap->para.pbuf,pMap->para.size);
				pMap->para.bModify=1;
				switch(pt->save_type)
				{
					case 0:pMap->para.nChkSec=3;break;	//��ʱд				
					case 1:pMap->para.nChkCnt=10;break;	//����д
					case 2:pMap->save(1);break;			//����д
					default:break;						//��д��			
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
		else	// ����������
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
			if (init == 0x38732560)			// ֻ����ϵͳ�ϵ�ʱ�����ļ�
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
			//if(fp==NULL)fp=FOPEN("sys.bin","wb");		// ϵͳ����ʱ�ᵼ�¶�����, 2014.2.18ȥ��
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

// ������д�������������ϵͳ������HMI��Ҳ�ǻص�����
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
		fp=FOPEN("met.bin","r");		//ԭ�����
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

// ���� fs_open��fs_close,fs_read,fs_write�� ��һ�׶����Լ�ʵ�ֵġ�
// ���׶������� ���� ������ �Ȳ����� xx_get,xx_put,����Ϊ����C����� put,get��һ������Ķ�����
// sfs_get sfs_put����ҿ�������
