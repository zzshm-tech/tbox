
/***********************
**	FileName:
**	Time:
*************************/

#include <string.h>

#include "DataType.h"
#include "CanProcess.h"
#include "SysProcess.h"

#include "bsp_can.h"         															//
#include "BspTick.h"
#include "bsp_mem.h"
#include "bsp_in.h"
#include "BspCfg.h"


/***********本地预定义宏***************/

#define CMD_LOCK1   						1
#define CMD_UNLOCK 							2

#define CMD_MONITOR_ON					1
#define CMD_MONITOR_OFF					2


/************数据本地常量**************/
//static  
Str_CanTxdData LockCarCmd 					=	{0x00};  //锁车命令
const Str_CanTxdData UpLockCarCmd 	= {0x00000501,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00};	 //解锁命令
const Str_CanTxdData MonOnCmd 			= {0x00000502,0x02,0x06,0x02,0x00,0x00,0x00,0x00,0x00};	 //监控开启
const Str_CanTxdData MonOffCmd 			= {0x00000502,0x01,0x04,0x01,0x00,0x00,0x00,0x00,0x00};	 //监控关闭
const Str_CanTxdData HeartBeat 			= {0x00000505,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00};	 //心跳包



/************本地全局变量*****************/

static CanLockCarStr								CanLockTcp;						//CAN锁车控制块 控制命令的发送、及发送状态

static CanLockStateStr              CanLockState;      //锁车状态

//static unsigned char 								AccStatusBack;
/******************************
**	锁车解锁命令
******************************/
void LockCarInfoHandle(unsigned char cmd,unsigned char n)
{
    switch (cmd)
    {
			case CMD_LOCK1:              //锁车   CMD_LOCK1:1,锁车
				CanLockTcp.LockCarCmdFlag = 1; 
				CanLockTcp.LockCarCmdStatus = 1;
				CanLockTcp.n = n;
				CanLockState.LockCurrentState = 1;    //锁车成功
				CanLockState.LockExpectState = 1;
				
				SaveLockCarInfo(&CanLockState);            //保存锁车信息
				//BasicPrintf(">锁车命令\r\n");
        break;
			case CMD_UNLOCK:             //解锁   CMD_UNLOCK:2,解锁   
				CanLockTcp.LockCarCmdFlag = 2;
				CanLockTcp.LockCarCmdStatus = 1;
				CanLockState.LockCurrentState = 0;    //锁车成功
				CanLockState.LockExpectState = 0;
				SaveLockCarInfo(&CanLockState);            //保存锁车信息
				//BasicPrintf(">解锁命令\r\n");
        break;
			case 3:
				CanLockTcp.LockCarCmdFlag = 1; 
				CanLockTcp.LockCarCmdStatus = 1;
				break;
			default :
				CanLockTcp.LockCarCmdFlag = 0;    //如果出现错误命令
				CanLockTcp.LockCarCmdStatus = 0;
				//BasicPrintf(">锁车命令无法识别\r\n");
        break;
    }
}



/********************************
**	监控开关；
*********************************/
 void MonitorInfoHandle(unsigned char cmd)
{
    switch (cmd)
    {
			case CMD_MONITOR_ON:              //
				CanLockTcp.MonCarCmdFlag = 1;
				CanLockTcp.MonCarCmdStatus = 1;
				//BasicPrintf(">监控打开命令\r\n");
        break;
			case CMD_MONITOR_OFF:              //
				CanLockTcp.MonCarCmdFlag = 2;
				CanLockTcp.MonCarCmdStatus = 1;
				//BasicPrintf(">监控关闭命令\r\n");
        break;
			default :
				CanLockTcp.MonCarCmdFlag = 0;
				CanLockTcp.MonCarCmdStatus = 0;
        //BasicPrintf(">监控命令无法识别\r\n");
        break;
    }
}


/*****************************
**
**	如果设备处于锁车状态，在没有接收到锁车命令的情况下，
**	每次ACC重启之后，都要发送锁车命令
*****************************/

void InitCanLockInfo(void)
{
	ReadLockCarInfo(&CanLockState);  			//读出保存的配置信息
		
	if(CanLockState.LockExpectState == 1)  //锁车
	{
			//LockCarInfoHandle(3);     //发送锁车
	}
}


/*******************************
**	
**	监控外部电源状态
**	如果外部电源掉电，执行锁车命令
*******************************/

void MonExternPower(void)
{
	static unsigned char counter;
	
	SysDataInfoStr 					mData;
	
	GetSysDataInfo(&mData); 
	
	if(mData.PowerVol < 60)   //
	{
		counter++;
		if(counter > 10)
		{
		}
			//LockCarInfoHandle(3);
	}
	else
	{
		counter = 0;
	}
}


/*****************************
**	函数名称:
**	功能描述:
*****************************/
void  SendLockCarCmd(unsigned short int n)
{
	IntToChar Tmp16;
	SysConfigStr	Tmp;
	
	
	LockCarCmd.id = 0x18FE0DEE;
	Tmp16.IntII = n * 8;
	
	LockCarCmd.data0 = Tmp16.TTbyte[0];
	LockCarCmd.data1 = Tmp16.TTbyte[1];   //限制转速的值
	LockCarCmd.data2 = 0xFF;
	LockCarCmd.data3 = 0xFF;
	LockCarCmd.data4 = 0xFF;
	ReadSysCfgInfo(&Tmp);
	LockCarCmd.data5 = Tmp.DevID[0];	 //
	LockCarCmd.data6 = Tmp.DevID[1];	 //
	LockCarCmd.data7 = Tmp.DevID[2];   //
	CAN_TransmitBeatCanMsg(1, 250, &LockCarCmd);
}


/******************************
**	
*******************************/

void  SendActivateCmd(unsigned char n)
{
	IntToChar Tmp16;
	SysConfigStr	Tmp;
	
	if(n == 0)
	{
		LockCarCmd.id = 0x18FE0BEE;
		Tmp16.IntII = 0x5176;
		LockCarCmd.data0 = Tmp16.TTbyte[0];    //激活密码
		LockCarCmd.data1 = Tmp16.TTbyte[1];
		ReadSysCfgInfo(&Tmp);
		LockCarCmd.data2 = Tmp.DevID[0];  //
		LockCarCmd.data3 = Tmp.DevID[1];	//
    LockCarCmd.data4 = Tmp.DevID[2];	//
		
		LockCarCmd.data5 = Tmp.DevSecret[0];  //GPS秘钥
		LockCarCmd.data6 = Tmp.DevSecret[1];
		LockCarCmd.data7 = Tmp.DevSecret[2]; 
		CAN_TransmitBeatCanMsg(1, 250, &LockCarCmd);
		return;
	}
	
	if(n == 1)
	{
		LockCarCmd.id = 0x18FE0BEE;
		Tmp16.IntII = 0x6715;
		LockCarCmd.data0 = Tmp16.TTbyte[0];    //激活密码
		LockCarCmd.data1 = Tmp16.TTbyte[1];
		ReadSysCfgInfo(&Tmp);
		LockCarCmd.data2 = Tmp.DevID[0];//Tmp.DevSecret[0];  //GPS ID,暂时使用设备秘钥
		LockCarCmd.data3 = Tmp.DevID[1];//Tmp.DevSecret[1];
		LockCarCmd.data4 = Tmp.DevID[2];//Tmp.DevSecret[2];  
		
		LockCarCmd.data5 = Tmp.DevSecret[0];  //GPS秘钥
		LockCarCmd.data6 = Tmp.DevSecret[1];
		LockCarCmd.data7 = Tmp.DevSecret[2]; 
		CAN_TransmitBeatCanMsg(1, 250, &LockCarCmd);
	}
}

/*******************************
**	处理CAN锁车
**	
*******************************/

void ProcessCanLockCar(void)
{
	static struct stopwatch32 sw2;
	static unsigned char step2 = 0;
	static unsigned char Num;
	
	CanDataStr TmpCan;
	
	GetCanData(&TmpCan);
	
	if(ReadSysRunStatus() > 0)     //不在判断CAN标志
		return;
	
	switch(step2)
	{
		case 0:      																	//锁车
			if(CanLockTcp.LockCarCmdFlag == 1)   				//
			{
				if(CanLockTcp.n == 0)
				{
					Num = 0;
					step2 = 1;      //1级锁车
					break;
				}
				else if(CanLockTcp.n == 1)
				{
					if(TmpCan.LockCarState == 1)     //先判断锁车状态
					{
						Num = 0;
						step2 = 3;     //先去解锁
						break;
					}
					Num = 0;
					step2 = 9;     //2级锁车
					break;
				}
			}                                  				 //解锁
			if(CanLockTcp.LockCarCmdFlag == 2)   			 //解锁
			{
				Num = 0;
				step2 = 3;
				break;
			}
			if(CanLockTcp.MonCarCmdFlag == 1)        //打开监控
			{
				Num = 0;
				step2 = 5;
				break;
			}
			if(CanLockTcp.MonCarCmdFlag == 2)   			//关闭监控
			{
				Num = 0;
				step2 = 7;
				break;
			}
			break;
		case 1:
			SendLockCarCmd(1000);     //锁车
			InitStopwatch32(&sw2);
			step2++;
			break;
		case 2:
			if(ReadStopwatch32Value(&sw2) < 99)   		//
				break;
			Num++;
			if(Num > 3)    														//读取30
			{
				CanLockTcp.LockCarCmdStatus = 0;   				//
				CanLockTcp.LockCarCmdFlag = 0;   					//停止发送
				step2 = 0;
				
				break;
			}
			
			step2 = 1;
			break;
		case 3:
			SendLockCarCmd(3500);      //解锁
			InitStopwatch32(&sw2);
			step2++;
			break;
		case 4:
			if(ReadStopwatch32Value(&sw2) < 99)   			//
				break;
			Num++;
			if(Num > 3)                                 //发送三次
			{
				CanLockTcp.LockCarCmdStatus = 0;  					//
				
				CanLockTcp.LockCarCmdFlag = 0;   						//
				if(CanLockTcp.n == 1)
				{
					Num = 0;
					step2 = 9;     //2级锁车
					CanLockTcp.n = 0;
					break;
				}
				step2 = 0;
				break;
			}
			step2 = 3;
			break;
		case 5:
			SendActivateCmd(1);					//发送激活码
			InitStopwatch32(&sw2);
			step2++;
			break;
		case 6:
			if(ReadStopwatch32Value(&sw2) < 99)   													//
				break;
		
			Num++;
			if(Num > 3)
			{
				step2 = 0;
				CanLockTcp.MonCarCmdStatus = 0;
				CanLockTcp.MonCarCmdFlag = 0;
				break;
			}
			step2 = 5;
			break;
		case 7:
			SendActivateCmd(0);   				//取消激活
			InitStopwatch32(&sw2);
			step2++;
			break;
		case 8:
			if(ReadStopwatch32Value(&sw2) < 99)   //
				break;
			Num++;
			if(Num > 3)
			{
				step2 = 0;
				CanLockTcp.MonCarCmdStatus = 0;
				CanLockTcp.MonCarCmdFlag = 0;
				break;
			}
			step2 = 7;
			break;
		case 9:
			SendLockCarCmd(0);     //锁车
			InitStopwatch32(&sw2);
			step2++;
			break;
		case 10:
			if(ReadStopwatch32Value(&sw2) < 99)   		//
				break;
			Num++;
			if(Num > 3)    														//读取30
			{
				CanLockTcp.LockCarCmdStatus = 0;   				//
				CanLockTcp.LockCarCmdFlag = 0;   					//停止发送
				step2 = 0;
				CanLockTcp.n = 0;
				break;
			}
			
			step2 = 9;
			break;
		default:
			step2 = 0;
			break;
	}
	
	
}



/********************************
**	获取CAN锁车控制块
*********************************/

void GetCanLockCarTcp(CanLockCarStr *p)
{
	memcpy(p,(unsigned char *)&CanLockTcp,sizeof(CanLockCarStr));
}


/*****************************
**	清除锁车监控状态
******************************/

void ClearLockCarStatus(void)
{
	CanLockTcp.LockCarCmdStatus = 0;
}


/******************************
**	清除锁车控制状态
*******************************/

void ClearMonStatus(void)
{
	CanLockTcp.MonCarCmdStatus = 0;
}


/******************File End****************/

