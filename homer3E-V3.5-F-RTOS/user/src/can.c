


/*****************************************
**
**
*******************************************/
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"

#include "fr_drv_iap.h"
#include "fr_drv_can.h"
#include "fr_drv_mem.h"


#include "data_type.h"
#include "mon.h"
#include "md5.h"
#include "config.h"
#include "pro_data.h"
#include "ddp.h"
#include "fifo.h"



#define CAN_TEST_MODE    		 0x1FFFFFAA					  //
#define CAN_IAP_REV_STOP     0x1FFFFFB9 					// 接收暂停命令
#define CAN_IAP_REV_CMD      0x1FFFFFBA 					// 接收连接命令
#define CAN_IAP_REV_DATA     0x1FFFFFBB 					// 接收升级数据ID
#define CAN_IAP_REV_CHECK    0x1FFFFFBC 					// 接收校验
#define CAN_IAP_TXD_CMD      0x1FFFFFBD 					// 响应连接指令
#define CAN_IAP_TXD_CHECK    0x1FFFFFBE 					// 发送校验完成

#define CAN_IAP_READ_ID			 0x1FFFFFB8						//




/*********************CAN升级相关全局变量***********************/

static unsigned int						gBinFieSize;               //文件升级长度
static unsigned short int			gBinCanTotal_Cnt;    //总包数
static unsigned char					gRev_MainVre;      
static unsigned char					gRev_UserVre;
static unsigned int 					gRxdBinFieSize;
//static unsigned char					gCanUpgradeStatus = 0;
static unsigned short int			gBinRxd_len = 0;
static unsigned char 					gCanBinWriteFlag = 0; 	//

/*****************CAN信息及操作相关全局变量*********************/

static struct can_info_str 			can_info;    				//发动机CAN信息(采集到的CAN信息状态)

static unsigned char 						can_connect_state;  //CAN连接状态

struct lock_current_str					lock_current;     //当前实际锁车状态

struct lock_expect_str					lock_expect;     //期望锁车状态（需要保存到外部EEPROM）

static unsigned short int 			can_num;				 //CAN协议号

QueueHandle_t  									lock_cmd_queue = NULL;   				//定义队列变量-系统监控队列

struct can_rx_data_str					can_rx_msg[30];				//CAN接收缓冲区


static unsigned char 						ReFlag1;								//暂时这样使用（）
		
static unsigned char 						ReFlag2;								//暂时这样使用（）

static TimerHandle_t						HeartRequest					= NULL;  			//




/***************************
**	读取期望锁车状态
***************************/

unsigned char read_lock_expect_state(void)
{
	if(lock_expect.lock_expect_state > 0)
		return 1;
	
	return 0;
}

/***************************
**	读取激活期望状态
***************************/

unsigned char read_mon_expect_state(void)
{
	if(lock_expect.mon_expect_state > 0)
		return 1;
	
	return 0;
}


/***************************
**	读取CAN数据
***************************/

void read_can_info(struct can_info_str *pcan)
{
	if(pcan == NULL)
		return;
	
	memcpy((unsigned char *)pcan,(const unsigned char *)&can_info,sizeof(can_info));
}



/********************
**	CAN连接状态
***********************/

unsigned char read_can_connect_state(void)
{
	return can_connect_state;
}


/********************
**	发动机转速
***********************/

unsigned short int read_can_actual_rotate(void)
{
	return can_info.engine_actual_rotate;
}



/*****************************
**	保存锁车状态
******************************/

unsigned char save_lock_state(void)
{
	unsigned char rv;
	
	rv = fr_fram_write(1024,(unsigned char *)&lock_expect, sizeof(lock_expect));
	
	
	return rv;
}


/*****************************
**	读取锁车状态
******************************/

void read_lock_state(void)
{
	unsigned char rv;
	
	rv = fr_fram_read(1024,(unsigned char *)&lock_expect, sizeof(lock_expect));
	
	if(rv > 0)
	{
		lock_expect.lock_expect_state = 0;
		lock_expect.mon_expect_state = 0;
	}
}



/****************************
**	:读取设备号，Homer3S使用
*****************************/

void read_can_id_handle(void)
{
	unsigned char i;
	unsigned char *p;
	unsigned char tmp_c[16];
	struct can_tx_data_str  mRespon;
		
	mRespon.id = CAN_IAP_READ_ID;
	p = &mRespon.data0;
	read_terminal_id(tmp_c,16);
	for(i = 0;i < 8;i++)
	{
		*(p + i) = tmp_c[i + 2];
	}
	fr_can1_tx_msg(250, &mRespon);
}



/***********************
**	
**	发送终检
************************/

void can_checkout_handle(struct fr_event *p_event)
{
	unsigned char tmp_c[200];
	unsigned char i;
	unsigned short int len;
	unsigned short int info_len;
	unsigned char tmp,info_check;
	
	struct can_tx_data_str  mRespon;
	
	read_terminal_id(tmp_c,16);
	memcpy(tmp_c + 16,(unsigned char *)&p_event->arg1,8);
	tmp = StrCompare(tmp_c + 8,tmp_c + 16,8);      //比较设备号
	if(tmp == 0)
		return;
	info_len = build_config_info((char *)tmp_c,200,1);   //生成终检信息
	if(info_len == 0)
		return;
	
	info_check  = 0;
	len = 0;
	
	do
	{
		mRespon.id = 0x1FFFFFAB;   	//
		for(i = 0; i < 8; i++)
    {
			if(len < info_len)
      {
        *(unsigned char *)(&mRespon.data0 + i) = tmp_c[len];
      }
      else
      {
				*(unsigned char *)(&mRespon.data0 + i) = 0xFF;    //
      }
			len++;
      info_check ^= *(unsigned char *)(&mRespon.data0 + i);
    }
		fr_can1_tx_msg(250, &mRespon);
	}while(len < info_len);
	
	mRespon.id = 0x1FFFFFAC;
  mRespon.data0 = info_check;
  mRespon.data1 = (unsigned char)(len & 0x00FF); 						//
  mRespon.data2 = (unsigned char)((len & 0xFF00) >> 8); 		//
  mRespon.data3 = 0x55;
  mRespon.data4 = 0xAA;
  mRespon.data5 = 0x55;
  mRespon.data6 = 0xAA;
  mRespon.data7 = 0x55;
  info_check = 0;
  len = 0;
  info_len = 0;
				
	fr_can1_tx_msg(250, &mRespon);
}



/******************************
**	:
**	:更新STM32应用程序
**	
*******************************/

void can_update_app_link_handle(unsigned char *data)
{
	struct can_tx_data_str  mRespon;
	
	gBinFieSize = *(unsigned int *)(data + 0);               //文件升级长度
	gBinCanTotal_Cnt = *(unsigned short int *)(data + 4);    //总包数
	gRev_MainVre = *(data + 6);      
	gRev_UserVre = *(data + 7); 
	
  if(BackupEraseHandle() != FLASH_COMPLETE)
		return;
   
	mRespon.id = CAN_IAP_TXD_CMD;
  mRespon.data0 = 0x55;
  mRespon.data1 = 0xAB;				//
  mRespon.data2 = 0xCD;				//
  mRespon.data3 = 0x55;
  mRespon.data4 = 0xAA;
  mRespon.data5 = 0x55;
  mRespon.data6 = 0xAA;
  mRespon.data7 = 0x55;
  
	fr_can1_tx_msg(250, &mRespon);
	
	//gCanUpgradeStatus = 0;
	gBinRxd_len = 0;
	fr_printf(">The App Main Ver:%u,The App UserVer:%u\r\n",gRev_MainVre,gRev_UserVre);
	fr_printf(">The Bin Fie Size:%u,The BinCanTotal:%u\r\n",gBinFieSize,gBinCanTotal_Cnt);
}


/***************************
**
******************************/

void can_update_app_data_handle(unsigned char *data)
{
	gBinCanTotal_Cnt--;
	if(gCanBinWriteFlag == 1)
	{
			//gCanUpgradeStatus = 0;
			return;							//
	}
	
	memcpy((unsigned char *)can_rx_msg + gBinRxd_len,data,8);
	gBinRxd_len += 8;
	if(gBinRxd_len > 2400)
	{
		gCanBinWriteFlag = 1;
		//gCanUpgradeStatus = 0;
		return;
	}
	//gCanUpgradeStatus = 0;
}
			
			
/***************************
**	CAN接受数据结束
******************************/

void can_update_data_stop_handle(unsigned char *data)
{
	if(Receive_Packet((unsigned char *)can_rx_msg, gBinRxd_len) == 1) 		//把接收到的数据写入单片机闪存
  {
        gCanBinWriteFlag = 1;
        return;
  }
  gRxdBinFieSize += gBinRxd_len;
   
	fr_printf(">Recv Data :%d,%d\r\n",gRxdBinFieSize,gBinRxd_len);
	gBinRxd_len = 0;
}
		

/***************************
**
******************************/
		
void can_update_data_check_handle(unsigned char *data)
{
	struct can_tx_data_str  mRespon;
	unsigned int gBinRxd_Check;
	unsigned int gBinTxd_Check;
	unsigned char tmp_c[16];
	
	gBinTxd_Check = *(unsigned int *)&data[4];
	gBinRxd_Check = calc_CRC32((unsigned char *)ADDR_APP_BKP,gRxdBinFieSize);
	fr_printf("\r\n> 累计接收字节: %d Byte\r\n ", gRxdBinFieSize);
		
  if(gCanBinWriteFlag == 1) 			//升级写入失败
  {
		fr_printf("\r\n\r\n**********************************\r\n");
    fr_printf("> Write UpdateFlag 'R'.\r\n");
    fr_printf("> Failed to update.\r\n");
    fr_printf("> Jump to user application.\r\n");
    fr_printf("\r\n**********************************\r\n\r\n");
    UpgradeFailReset();			//升级失败，复位应用程序，重启系统
  }
    
	if(gBinTxd_Check != gBinRxd_Check) 					//升级检验失败
  {
		fr_printf("\r\n********************************************\r\n");
    fr_printf("* 升级文件异或校验不正确\r\n");
    fr_printf("* gBinTxd_Check = %d , gBinRxd_Check = %d\r\n", gBinTxd_Check, gBinRxd_Check);
    fr_printf("> Write UpdateFlag 'R'.\r\n");
    fr_printf("> Failed to update.\r\n");
    fr_printf("> Jump to user application.\r\n");
    fr_printf("********************************************\r\n");
    UpgradeFailReset();
   }
   else
   {
        // 校验成功后发送设备编号流水号 
		 read_terminal_id(tmp_c,16);
     mRespon.id = CAN_IAP_TXD_CHECK;
     mRespon.data0 = tmp_c[11];		//'9';          //回应5位流水号  
     mRespon.data1 = tmp_c[12];		//'0'           //设备后五位
     mRespon.data2 = tmp_c[13];    //'0'
     mRespon.data3 = tmp_c[14];    //'0'
     mRespon.data4 = tmp_c[15];    //'7'
     mRespon.data5 = 0x55;
     mRespon.data6 = 0xAA;
     mRespon.data7 = gBinTxd_Check;/* 发送响应校验*/
     fr_can1_tx_msg(250, &mRespon);
     fr_printf("********************************************\r\n");
     fr_printf("* >升级文件异或校验正确\r\n");
     fr_printf("* >gBinTxd_Check = %d , gBinRxd_Check = %d\r\n", gBinTxd_Check, gBinRxd_Check);
     fr_printf("* > Download done. Reveived %d bytes.\r\n", gRxdBinFieSize);
       
     fr_printf("* > Write UpdateFlag 'Y'.\r\n");
     fr_printf("* > Device Firmware Upgrade Success!\r\n");
     fr_printf("********************************************\r\n");
		 UpgradeOkReset();   
   }
}




/***************************
**	相应ECU握手信号
****************************/

void respond_ecu_hand(unsigned char *buf)
{
	unsigned char TmpBuf[16];
	struct can_tx_data_str	mRespon;
	
	if(read_config_state() != 0x55)
		return;
	
	read_config_dev_secret(TmpBuf,3);
	TmpBuf[3] = *(buf + 0);
	TmpBuf[4] = *(buf + 1);
	TmpBuf[5] = *(buf + 2);
	TmpBuf[6] = *(buf + 3);
	TmpBuf[7] = *(buf + 4);
//	
	md5(TmpBuf,(unsigned int *)&TmpBuf,8);
	mRespon.id = 0x18FFD5FD;        //CAN消息包ID
	mRespon.data0 = TmpBuf[0];
	mRespon.data1 = TmpBuf[1];
	mRespon.data2 = TmpBuf[2];
	mRespon.data3 = TmpBuf[3];
	mRespon.data4 = TmpBuf[4];
	mRespon.data5 = TmpBuf[5];
	mRespon.data6 = TmpBuf[6];
	mRespon.data7 = TmpBuf[7];
	fr_can1_tx_msg(250, &mRespon);
}

/********************
**	解析CAN信息
***********************/

void analysis_can_info(void)
{
	BaseType_t 						xStatus;
	struct fr_event 			event;
	
	switch(can_num)
	{
		case 0:
			if(can_rx_msg[0].can_id == 0x18ff0000)
			{
				memset((char *)&can_rx_msg[1],0,sizeof(struct can_info_str));					//
			}
	
			if(can_rx_msg[1].can_id == 0x0cf00400)
			{
				can_info.engine_actual_rotate = *((unsigned short int *)&can_rx_msg[1].data[3]);		//  发动机转速
				can_info.engine_torque = can_rx_msg[1].data[2];																		//发动机实际扭矩
				
				memset((char *)&can_rx_msg[1],0,sizeof(struct can_info_str));					//
			}
			if(can_rx_msg[2].can_id == 0x0cf00300)	//
			{
				memset((char *)&can_rx_msg[2],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[3].can_id == 0x18f0000f)	//ECR1
			{
					
				memset((char *)&can_rx_msg[3],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[4].can_id == 0x18fee000)	//
			{
				can_info.once_travel = *(unsigned int *)&can_rx_msg[4].data[0];										//单次行驶里程			 OK  66816*0.125=8352
				can_info.total_travel = *(unsigned int *)&can_rx_msg[4].data[4];  								//总行驶里程         OK  1*0.125
				
				memset((char *)&can_rx_msg[4],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[5].can_id == 0x18fef200)	//
			{
				can_info.once_fuel = *(unsigned int *)&can_rx_msg[5].data[0];      								//单次油耗           66816*0.05  此处有问题？？？？
				can_info.total_fuel = *(unsigned int *)&can_rx_msg[5].data[4];  																										//累计油耗
				
				memset((char *)&can_rx_msg[5],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[6].can_id == 0x18fef100)		//
			{
				can_info.travel_speed = *(unsigned short int *)&can_rx_msg[6].data[1];						//行驶速度,暂时传输  OK  改的 2560/256  0
				//作业标志不再使用
				memset((char *)&can_rx_msg[6],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[7].can_id == 0x18ff0800)	//锁车状态标志
			{
				can_info.relative_oil_pressure = can_rx_msg[7].data[0];														//相对机油压力    OK  2*4K   
				
				memset((char *)&can_rx_msg[7],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[8].can_id == 0x18fedf00) //
			{
				can_info.engine_set_rotate = *(unsigned short int *)&can_rx_msg[8].data[1];				//发动机目的转速(预留) OK 17600*0.125
				
				memset((char *)&can_rx_msg[8],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[9].can_id == 0x18fef600)	//
			{
				can_info.relative_add_pressure = can_rx_msg[9].data[1];   //相对增压压力
				can_info.absolute_add_pressure = can_rx_msg[9].data[3];   //绝对增压压力
				memset((char *)&can_rx_msg[9],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[10].can_id == 0x18feef00)	//
			{
				can_info.absolute_oil_pressure = can_rx_msg[10].data[3];  //机油压力
				can_info.oil_position = can_rx_msg[10].data[2] * 0.4;    //机油液位
				
				memset((char *)&can_rx_msg[10],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[11].can_id == 0x18fee400)	//shut down
			{
				
				memset((char *)&can_rx_msg[11],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[12].can_id == 0x18feee00)//
			{
				can_info.temp_water = can_rx_msg[12].data[0];       
				can_info.fuel_temp = can_rx_msg[12].data[1];
				can_info.oil_temp = *(unsigned short int *)&can_rx_msg[12].data[2];
				memset((char *)&can_rx_msg[12],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[13].can_id == 0x18fe5600)	//TI1
			{
				
				memset((char *)&can_rx_msg[13],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[14].can_id == 0x18fef500)	//
			{
				can_info.air_pressure = can_rx_msg[14].data[0];
				can_info.air_temp = *(unsigned short int *)&can_rx_msg[14].data[3];
				can_info.entere_air_temp = can_rx_msg[14].data[5];
				memset((char *)&can_rx_msg[14],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[15].can_id == 0x18fef700)//
			{
				can_info.sys_vol = *(unsigned short int *)&can_rx_msg[15].data[6] * 0.5; //电瓶电压
				memset((char *)&can_rx_msg[15],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[16].can_id == 0x18fd0700)//
			{
				
				memset((char *)&can_rx_msg[16],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[17].can_id == 0x18fee500)	//
			{
				can_info.engine_work_time = *(unsigned int *)&can_rx_msg[17].data[0];//发动机工作时间
				memset((char *)&can_rx_msg[17],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[18].can_id == 0x18feff00)//
			{
				
				memset((char *)&can_rx_msg[18],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[19].can_id == 0x18FD0100)//ECU握手信号
			{
				respond_ecu_hand(can_rx_msg[19].data);
				memset((char *)&can_rx_msg[19],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[20].can_id == 0x18fee900)
			{
				can_info.once_fuel = *(unsigned int *)&can_rx_msg[20].data[0];   //单次油耗
				can_info.total_fuel = *(unsigned int *)&can_rx_msg[20].data[4];  //总油耗
				memset((char *)&can_rx_msg[20],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[21].can_id == 0x18EA0021)    //
			{
				ReFlag1 = 0;								//暂时这样使用
				ReFlag2 = 0;								//暂时这样使用
				memset((char *)&can_rx_msg[21],0,sizeof(struct can_info_str));
			}
			
			if(can_rx_msg[22].can_id == CAN_IAP_REV_CMD)    //升级开始命令
			{
				memset((char *)&can_rx_msg[22],0,sizeof(struct can_info_str));
			}
			
			if(can_rx_msg[23].can_id == CAN_IAP_REV_DATA)    //传送数据
			{
				memset((char *)&can_rx_msg[23],0,sizeof(struct can_info_str));
			}
			
			if(can_rx_msg[24].can_id == CAN_IAP_REV_STOP)    //停止传送数据
			{
				memset((char *)&can_rx_msg[24],0,sizeof(struct can_info_str));
			}
			
			if(can_rx_msg[25].can_id == CAN_IAP_REV_CHECK)    //校验
			{
				memset((char *)&can_rx_msg[25],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[26].can_id == CAN_TEST_MODE)    //测试
			{
				event.cmd = 2;
				memcpy((unsigned char *)&event.arg1,can_rx_msg[26].data,8);    
				
				xStatus = xQueueSendToBack(lock_cmd_queue, &event, 0);
				if(xStatus > 0)
				{
					
				}
				memset((char *)&can_rx_msg[26],0,sizeof(struct can_info_str));
			}
			if(can_rx_msg[27].can_id == CAN_IAP_READ_ID)    //读取设备号
			{
				memset((char *)&can_rx_msg[27],0,sizeof(struct can_info_str));
			}
			
			break;
		default:
			break;
	}
	
}



/**********************
**	CAN1接收数据回调函数
**	接收到CAN数据
************************/

static void can1_rx_handle(unsigned int can_id,unsigned char *data_buf)
{
	
	switch(can_id)
	{
		case 0x18ff0000:  	// 
			can_rx_msg[0].can_id = 0x18ff0000;
			memcpy(can_rx_msg[0].data,data_buf,8); 
			break;
		case 0x0cf00400:	//
			can_rx_msg[1].can_id = 0x0cf00400;
			memcpy(can_rx_msg[1].data,data_buf,8); 
			break;
		case 0x0cf00300:	//
			can_rx_msg[2].can_id = 0x0cf00300;
			memcpy(can_rx_msg[2].data,data_buf,8); 
			break;
		case 0x18f0000f:	//ECR1
			can_rx_msg[3].can_id = 0x18f0000f;
			memcpy(can_rx_msg[3].data,data_buf,8); 
			break;
		case 0x18fee000:	//
			can_rx_msg[4].can_id = 0x18fee000;
			memcpy(can_rx_msg[4].data,data_buf,8); 
			break;
		case 0x18fef200:	//
			can_rx_msg[5].can_id = 0x18fef200;
			memcpy(can_rx_msg[5].data,data_buf,8); 
			break;
		case 0x18fef100:		//
			can_rx_msg[6].can_id = 0x18fef100;
			memcpy(can_rx_msg[6].data,data_buf,8); 
			break;
		case 0x18ff0800:	//
			can_rx_msg[7].can_id = 0x18ff0800;
			memcpy(can_rx_msg[7].data,data_buf,8); 
			break;
		case 0x18fedf00: //
			can_rx_msg[8].can_id = 0x18fedf00;
			memcpy(can_rx_msg[8].data,data_buf,8); 
			break;
		case 0x18fef600:	//
			can_rx_msg[9].can_id = 0x18fef600;
			memcpy(can_rx_msg[9].data,data_buf,8); 
			break;
		case 0x18feef00:		//
			can_rx_msg[10].can_id = 0x18feef00;
			memcpy(can_rx_msg[10].data,data_buf,8); 
			break;
		case 0x18fee400:	//shut down
			can_rx_msg[11].can_id = 0x18fee400;
			memcpy(can_rx_msg[11].data,data_buf,8); 
			break;
		case 0x18feee00:	//
			can_rx_msg[12].can_id = 0x18feee00;
			memcpy(can_rx_msg[12].data,data_buf,8); 
			break;
		case 0x18fe5600:	//TI1
			can_rx_msg[13].can_id = 0x18fe5600;
			memcpy(can_rx_msg[13].data,data_buf,8); 
			break;
		case 0x18fef500:	//
			can_rx_msg[14].can_id = 0x18fef500;
			memcpy(can_rx_msg[14].data,data_buf,8); 
			break;
		case 0x18fef700://
			can_rx_msg[15].can_id = 0x18fef700;
			memcpy(can_rx_msg[15].data,data_buf,8); 
			break;
		case 0x18fd0700: //
			can_rx_msg[16].can_id = 0x18fd0700;
			memcpy(can_rx_msg[16].data,data_buf,8); 
			break;
		case 0x18fee500:	//
			can_rx_msg[17].can_id = 0x18fee500;
			memcpy(can_rx_msg[17].data,data_buf,8); 
			break;
		case 0x18feff00://
			can_rx_msg[18].can_id = 0x18feff00;
			memcpy(can_rx_msg[18].data,data_buf,8); 
			break;
		case 0x18fd0100:             //
			can_rx_msg[19].can_id = 0x18fd0100;
			memcpy(can_rx_msg[19].data,data_buf,8); 
			break;
		case 0x18FEE900:
			can_rx_msg[20].can_id = 0x18fee900;
			memcpy(can_rx_msg[20].data,data_buf,8); 
			break;
		case 0x18EA0021:    //
			can_rx_msg[21].can_id = 0x18ea0021;
			memcpy(can_rx_msg[21].data,data_buf,8); 
			break;
		case CAN_IAP_REV_CMD:/*0x1FFFFFBA*/ 	//
			can_rx_msg[22].can_id = CAN_IAP_REV_CMD;
			memcpy(can_rx_msg[22].data,data_buf,8); 
			break;
		case CAN_IAP_REV_DATA:/*0x1FFFFFBB*/
			can_rx_msg[23].can_id = CAN_IAP_REV_DATA;
			memcpy(can_rx_msg[23].data,data_buf,8); 
			break;
		case CAN_IAP_REV_STOP:/*0x1FFFFFB9*/						//
			can_rx_msg[24].can_id = CAN_IAP_REV_STOP;
			memcpy(can_rx_msg[24].data,data_buf,8); 
			break;
		case CAN_IAP_REV_CHECK:/*0x1FFFFFBC*/	//
			can_rx_msg[25].can_id = CAN_IAP_REV_CHECK;
			memcpy(can_rx_msg[25].data,data_buf,8); 
			break;
		case CAN_TEST_MODE:/*0x1FFFFFAA	*/		//
			can_rx_msg[26].can_id = CAN_TEST_MODE;
			memcpy(can_rx_msg[26].data,data_buf,8); 
			break;
		case CAN_IAP_READ_ID:/*0x1FFFFFB8*/           //
			can_rx_msg[27].can_id = CAN_IAP_READ_ID;
			memcpy(can_rx_msg[27].data,data_buf,8); 
			break;
		case 0x18feca00:  //单故障包
		case 0x18ecff00:	//多故障码信息通告
		case 0x18ebff00:	//多故障码数据包
			can_rx_msg[28].can_id = can_id;
			memcpy(can_rx_msg[28].data,data_buf,8);
			break;
		default:
			break;
	}
}







/**********************
** CAN接收任务
***********************/

void task_can_rx(void *data)
{
	data = data;
	
	fr_can1_init(250);       								//打开串口
	fr_set_can1_rx_hook(can1_rx_handle);    //设置接收CAN数据回调函数
	fr_init_can1_sema();
	
	can_num = 0;
	for(;;)
	{
		vTaskDelay(10);            //CAN解析
		analysis_can_info();
	}
}


/*****************************
**	发送锁车命令
**	1：执行成功；
**	2：执行失败
*****************************/
static unsigned char send_lock_cmd(unsigned short int n)
{
	unsigned char 				index;				//
	int16_to_char   						tmp_rotate;   //
	struct can_tx_data_str			tmp_cmd;  		//
	unsigned char 							tmp_c[8];		
	index = 0;
	
	if(lock_expect.mon_expect_state == 0)       //设备未激活，不发送锁车命令，直接返回失败
		return 0;
	
	if(n == 0)
		n = 3500;
	else if(n == 1)
		n = 1000;
	else if(n == 2)
		n = 0;
	
	read_config_dev_id(tmp_c,3);
		
	while(index < 5)
	{
		tmp_cmd.id = 0x18FFD6FD;				
		tmp_rotate.value = n * 8;              //发动机转速
		
		tmp_cmd.data0 = tmp_rotate.byte[0];		//发动机转速
		tmp_cmd.data1 = tmp_rotate.byte[1];   //限制转速的值
		tmp_cmd.data2 = 0xFF;			//
		tmp_cmd.data3 = 0xFF;			//
		tmp_cmd.data4 = 0xFF;			//

		tmp_cmd.data5 = tmp_c[0];			//
		tmp_cmd.data6 = tmp_c[1];			//
		tmp_cmd.data7 = tmp_c[2];			//
	
		fr_can1_tx_msg(250, &tmp_cmd);
		vTaskDelay(100);            //CAN解析
		if(lock_current.lock_current_state != lock_expect.lock_expect_state)
			return 1;
		index++;
	}
	return 0;
}


/*****************************
**	发送激活命令
*****************************/

static unsigned char send_activate_cmd(unsigned char n)
{
	unsigned char 				index;				//
	struct can_tx_data_str			tmp_cmd;  		//
	int16_to_char   						tmp_int16;   //
	unsigned char 				tmp_c[8];
	
	index = 0;
	
	read_config_dev_id(tmp_c,3);
	read_config_dev_secret(tmp_c  + 3,3);
	
	while(index < 5)
	{
		if(n == 0)
		{
			tmp_cmd.id = 0x18FFD4FD;
			tmp_int16.value = 0xFE34;
		}
		else if(n == 1)
		{
			tmp_cmd.id = 0x18FFD4FD;
			tmp_int16.value = 0xDE15;
		}
		tmp_cmd.data0 = tmp_int16.byte[0];    //激活密码
		tmp_cmd.data1 = tmp_int16.byte[1];
		tmp_cmd.data2 = tmp_c[0];  						//
		tmp_cmd.data3 = tmp_c[1];							//
    tmp_cmd.data4 = tmp_c[2];							//
		
		tmp_cmd.data5 = tmp_c[3];  						//GPS秘钥
		tmp_cmd.data6 = tmp_c[4];
		tmp_cmd.data7 = tmp_c[5]; 
		fr_can1_tx_msg(250, &tmp_cmd);
		vTaskDelay(100);            					//CAN解析
		if(lock_current.mon_current_state != lock_expect.mon_expect_state)
			return 1;
		index++;
	}
	return 0;
}



/***********************
**	定时器服务函数
**************************/

void send_heart_request(void)
{
	static unsigned char step;
	
	struct can_tx_data_str 					tmp_cmd;
	
	if(read_acc_state() == 0)     															//
		return;
	
	switch(step)
	{
		case 0:
			if(ReFlag1++ >= 4)
			{
				memset((unsigned char *)&tmp_cmd,0,sizeof(tmp_cmd));
				tmp_cmd.id = 0x18EA0021;
				tmp_cmd.data0 = 0xE5;
				tmp_cmd.data1 = 0xFE;
				ReFlag1 = 0;
				fr_can1_tx_msg(250, &tmp_cmd);
			}
			step++;
			break;
		case 1:
			if(ReFlag2++ >= 4)
			{
				memset((unsigned char *)&tmp_cmd,0,sizeof(tmp_cmd));
				tmp_cmd.id = 0x18EA0021;
				tmp_cmd.data0 = 0xE9;
				tmp_cmd.data1 = 0xFE;
				ReFlag2 = 0;
				fr_can1_tx_msg(250, &tmp_cmd);
			}
			step = 0;
			break;
		default:
	
			step = 0;
			break;
	}
}


/******************************
**
**	鍑芥暟鍚嶇О:
**	鍔熻兘鎻忚堪:
**	
********************************/

void init_request_heart(void)
{
	 if(HeartRequest == NULL)
	 {
		 HeartRequest = xTimerCreate
                   //
                   (NULL,
                   //
                   100,   
                   //
                   pdTRUE,
                   //
                  NULL,
                   //
                  (TimerCallbackFunction_t)send_heart_request);

     if(HeartRequest != NULL ) 
		 {
        // 
        xTimerStart(HeartRequest, 0);     //
		 }
	 }
}



/**********************
** 	CAN 锁车任务
**	使用锁车命令队列
**	根据命令，执行相关锁车
**	优先级：握手链接最高
**	激活命令次之
**	锁车命令最低
***********************/
void task_can_lock(void *data)
{
	BaseType_t xStatus;
	struct fr_event event;
	unsigned char acc_back;
	unsigned char state;
	unsigned char buf[50];   //
	
	data = data;
	
	vTaskDelay(100);
	read_lock_state();            //读取锁车状态
	
	lock_cmd_queue = xQueueCreate(3, sizeof(struct fr_event));  //创建一个队列（能够缓冲5级命令）
	init_request_heart();
	acc_back = 0;
	
	for(;;)
	{
		//
		xStatus = xQueueReceive(lock_cmd_queue, &event,100);    //接收锁车命令
 
		if(xStatus == pdPASS)
		{
			switch(event.cmd)
			{
				case 0:                     //解锁车
					lock_expect.lock_expect_state = event.arg1;
					save_lock_state();
					state = send_lock_cmd(event.arg1);		
					xStatus = build_general_response(buf,sizeof(buf),event.arg2,state,event.arg3);
					if(xStatus > 0)
					{
						WriteCmdDataBuf(buf,xStatus); 
					}
					break;
				case 1: 									//激活锁车功能
					lock_expect.mon_expect_state = event.arg1;
					save_lock_state();
					state = send_activate_cmd(event.arg1);  
					xStatus = build_general_response(buf,sizeof(buf),event.arg2,state,event.arg3);
					if(xStatus > 0)
					{
						WriteCmdDataBuf(buf,xStatus); 
					}				
					break;
				case 2:
					can_checkout_handle(&event);    //注意这个问题
					break;
				default:
					
					break;
			}
		}
		
		if(acc_back != read_acc_state())    //ACC变化-ACC打开
		{
			if(acc_back == 0)
			{
				if(lock_expect.lock_expect_state == 1)
					send_lock_cmd(1);
				else if(lock_expect.lock_expect_state == 2)
					send_lock_cmd(2);
			}
			acc_back = read_acc_state(); 
		}
	}
}


