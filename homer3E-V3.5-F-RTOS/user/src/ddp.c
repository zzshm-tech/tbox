

/********************
**
**
***********************/

#include "FreeRTOS.h"

#include <stdio.h>
#include <string.h>
#include "pro_data.h"
#include "data_type.h"
#include "version.h"
#include "mon.h"
#include "rtc.h"
#include "config.h"
#include "gnss.h"
#include "can.h"


static unsigned char 						TmpBuf[512];					//

static unsigned char 						MsgLine;   						//

static unsigned char 						MsgNum;							//

static unsigned short int 			MsgSerial;						//

static unsigned char 						TerMsgBig = 0;					//

static MsgGnssStr   						*pMulGnss;						//

/********************
**
***********************/

unsigned short int build_position_packet(unsigned char *source,unsigned char big)
{
	static unsigned short int DataLen = 0;
	unsigned short int tmp = 0;
	
	MsgHeadStr 				TmpMsg;
	unsigned char 		buf[100];
	
	MsgHeadStr 	 		*pMsgHead;
	MsgGnssStr   		*pMsgGnss;
	MsgDeviceStr 		*pMsgDevice;
	MsgCanMaizeStr 		*pMsgMaize;       //
	
	struct can_info_str  			pCanData;
	struct time_str 		 now;         //时间
	
	if(MsgLine <= 1)
	{
		TmpMsg.frame_start[0] = 0xF1;
		TmpMsg.frame_start[1] = 0xF2;
		TmpMsg.frame_start[2] = 0xFF;
		TmpMsg.msg_id = 0x45;
		
		memcpy(&TmpMsg.device_id,buf,16);

		TmpMsg.DataPackFlag = 0x00;
		TmpMsg.msg_body_num = 0x00;
		TmpMsg.msg_len = 0x00;
		memcpy(TmpBuf,(unsigned char *)&TmpMsg,25);           			//
		DataLen = 25;

		pMsgDevice = (MsgDeviceStr *)buf;               				//
		pMsgDevice->MsgDeviceType = 0x0001;              	 			// 
		pMsgDevice->MsgDeviceLen = sizeof(MsgDeviceStr) - 4;  			//

		pMsgDevice->MsgManuNum = 0x01;            						//   		
		pMsgDevice->MsgTerminalType = 0x01;        						//
		pMsgDevice->MsgUserNum = 32;                     				//??????
		pMsgDevice->MsgAppVer1 = read_fir_ver();      //单片机软件版
		pMsgDevice->MsgAppVer2 = read_fir_ver();           				//ST86软件版本
		pMsgDevice->MsgHardwareVer = 41;             					//硬件版本
		memcpy(TmpBuf + DataLen,buf,sizeof(MsgDeviceStr));			// 
		DataLen += sizeof(MsgDeviceStr); 
		MsgNum = 1;
		
		//
		pMsgMaize = (MsgCanMaizeStr *)buf;    //
		
		read_can_info(&pCanData);
		switch(read_car_type())
		{ 
			case 0x0A:						  //
				pMsgMaize->MsgMaizeMechType = 0x000C;										//
				pMsgMaize->MsgMaizeMechLen = 78;        									//

				pMsgMaize->MsgSysVol = pCanData.sys_vol;           	 						//
				
				pMsgMaize->MsgTempWater = pCanData.temp_water;           		//
				pMsgMaize->MsgEngineRotate = pCanData.engine_actual_rotate;		 		//发动机转速
				pMsgMaize->MsgEngineRotateSet = pCanData.engine_set_rotate;     		//
				pMsgMaize->MsgEngineTorque = pCanData.engine_torque;				//
				pMsgMaize->MsgFuelTemp = pCanData.fuel_temp;					//
				pMsgMaize->MsgOilTemp = pCanData.oil_temp;         			//
				pMsgMaize->MsgAirPressure = pCanData.air_pressure;         		//
				pMsgMaize->MsgEngineNacelleTemp = pCanData.engine_nacelle_temp;			//
				pMsgMaize->MsgAirTemp = pCanData.air_temp;					//
				pMsgMaize->MsgEnteredAirTemp = pCanData.entere_air_temp;				//
				pMsgMaize->MsgEngineWorkTime = pCanData.engine_work_time;				//发动机工作时长
				pMsgMaize->MsgTravelSpeed = pCanData.travel_speed;				//行驶速度
				pMsgMaize->MsgOnceTravel = pCanData.once_travel;					//
				pMsgMaize->MsgTotalTravel = pCanData.total_travel;  				//
				pMsgMaize->MsgOnceFuel = pCanData.once_fuel;      				//
				pMsgMaize->MsgTotalFuel = pCanData.total_fuel;  				//
				pMsgMaize->MsgRelativeOilPressure = pCanData.relative_oil_pressure;		//
				pMsgMaize->MsgAbsoluteOilPressure = pCanData.absolute_oil_pressure;		//
				pMsgMaize->MsgRelativeAddPressure = pCanData.relative_add_pressure;		//
				pMsgMaize->MsgAbsoluteAddPressure = pCanData.absolute_add_pressure;		//
				pMsgMaize->MsgFuelNum = pCanData.fuel_position;					//燃油位置		
				pMsgMaize->MsgFuelPercent = pCanData.fuel_percent;				//
				pMsgMaize->MsgOilPosition = pCanData.oil_position;				//机油位置
				pMsgMaize->MsgCrankcasePressure = pCanData.crankcase_pressure;   		//
				pMsgMaize->MsgCoolPressure = pCanData.cool_pressure;  				//
				pMsgMaize->MsgCoolPosition = pCanData.cool_position;				//
				pMsgMaize->MsgLockCarState = pCanData.lock_car_state;				//
				pMsgMaize->MsgActivateStatus = pCanData.activate_status;   			//
				pMsgMaize->MsgKeyStatus = pCanData.key_status;     				//
				pMsgMaize->MsgTerIDStatus = pCanData.ter_id_status;				//
				pMsgMaize->MsgWorkFlag = pCanData.work_flag;   						//
				pMsgMaize->MsgCarWarnValue1 =pCanData.car_warn_value1;				//
				pMsgMaize->MsgCarWarnValue2 = pCanData.car_warn_value2;				//
				pMsgMaize->MsgFoodFullWarn = pCanData.food_full_warn;				//
				pMsgMaize->MsgClutchState = pCanData.clutch_state;  				//
				pMsgMaize->MsgStripRotate = pCanData.strip_rotate;				//
				pMsgMaize->MsgLiftRotate = pCanData.lift_rotate;					//
				pMsgMaize->MsgCutTableHigh = pCanData.cut_table_high;				//割台高度
				pMsgMaize->MsgNC1 = 0;
				pMsgMaize->MsgNC2 = 0;
				pMsgMaize->MsgNC3 = 0;
				pMsgMaize->MsgNC4 = 0;     //
				memcpy(TmpBuf + DataLen,buf,sizeof(MsgCanMaizeStr)); 		//
				DataLen += sizeof(MsgCanMaizeStr);
				MsgNum++;
			break;
			default:
				break;
		}

			//
		pMsgGnss = (MsgGnssStr *)buf;
		read_time(&now);
		
		pMsgGnss->MsgGnssType = 0x0021;								//
		pMsgGnss->MsgGnssLen = 0;				//
			
		pMsgGnss->MsgGnssLatitude = read_gnss_latitude() * 1000000;		//	
		pMsgGnss->MsgGnssLongitude = read_gnss_longitude() * 1000000;	//
		pMsgGnss->MsgGnssSpeed = read_gnss_speed();		//行驶速度
		pMsgGnss->MsgGnssAzimuth = read_gnss_heading();					//
		pMsgGnss->MsgGnssAltitude = read_gnss_altitude();				//
		pMsgGnss->MsgGnssYear = now.year; 					//Gnss
		pMsgGnss->MsgGnssMon = now.mon;						//Gnss
		pMsgGnss->MsgGnssDay = now.day;						//Gnss
		pMsgGnss->MsgGnssHour = now.hour; 					//Gnss 
		pMsgGnss->MsgGnssMin = now.min;						//Gnss 
		pMsgGnss->MsgGnssSec = now.sec;						//Gnss
			
		pMsgGnss->MsgGnssSatelliteNum = read_satellite_num();		//
		pMsgGnss->MsgGnssViewNum = read_bd_sate_num() + read_gps_sate_num();		//	可视卫星数量(GPS + BD)
		pMsgGnss->MsgGhdopV = read_gnss_hdop() ;	
		
		if(read_gnss_state() == 'A')
			pMsgGnss->MsgGnssStatus = pMsgGnss->MsgGnssStatus | 0x02; 		//  
		else
			pMsgGnss->MsgGnssStatus = pMsgGnss->MsgGnssStatus & 0xFD;
	
		if(read_ant_state() > 0)    //判断天线状态
			pMsgGnss->MsgGnssStatus |= 0x01;
		else
			pMsgGnss->MsgGnssStatus &= 0xFE;

		pMulGnss = (MsgGnssStr *)(TmpBuf + DataLen);           //
		memcpy(TmpBuf + DataLen,buf,sizeof(MsgGnssStr)); 
		DataLen += sizeof(MsgGnssStr);
		TmpBuf[DataLen] = MsgLine;     //
		MsgLine++;
		DataLen++;
		MsgNum++;
	}

	pMsgGnss = (MsgGnssStr *)buf;
	read_time(&now);
	
	pMsgGnss->MsgGnssType = 0x0021;								//
	pMsgGnss->MsgGnssLen = 0;									//

		
	pMsgGnss->MsgGnssLatitude = read_gnss_latitude() * 1000000;		//	
	pMsgGnss->MsgGnssLongitude = read_gnss_longitude() * 1000000;	//
	pMsgGnss->MsgGnssSpeed = read_gnss_speed();		//行驶速度
	pMsgGnss->MsgGnssAzimuth = read_gnss_heading();					//
	pMsgGnss->MsgGnssAltitude = read_gnss_altitude();				//
	pMsgGnss->MsgGnssYear = now.year; 					//Gnss
	pMsgGnss->MsgGnssMon = now.mon;						//Gnss
	pMsgGnss->MsgGnssDay = now.day;						//Gnss
	pMsgGnss->MsgGnssHour = now.hour; 					//Gnss 
	pMsgGnss->MsgGnssMin = now.min;						//Gnss 
	pMsgGnss->MsgGnssSec = now.sec;						//Gnss
		
	pMsgGnss->MsgGnssSatelliteNum = read_satellite_num();		//
	pMsgGnss->MsgGnssViewNum = read_bd_sate_num() + read_gps_sate_num();		//	可视卫星数量(GPS + BD)
	pMsgGnss->MsgGhdopV = read_gnss_hdop() ;	
	//
	memcpy(TmpBuf + DataLen,(buf + 4),(sizeof(MsgGnssStr) - 4)); 
	DataLen = DataLen + (sizeof(MsgGnssStr) - 4);
	TmpBuf[DataLen] = MsgLine;     //
	DataLen++;
	MsgLine++;
	//SL_Print("The Gnss Msg :%d,%d,%d\r\n",MsgLine,DataLen,big);
	if(MsgLine >= 10 || big > 0)    								//                    
	{
		pMulGnss->MsgGnssLen = MsgLine * 26;
		pMsgHead = (MsgHeadStr *)TmpBuf; 
		pMsgHead->msg_body_num = MsgNum;
		pMsgHead->CarTypeNum = 0x02;  										//
		
		pMsgHead->msg_len = DataLen - 23; 						
		MsgSerial++;
		memcpy(TmpBuf + DataLen,(unsigned char *)&MsgSerial,2);
		DataLen += 2;
		*(TmpBuf + DataLen) = BccVerify(TmpBuf + 3,DataLen - 3);
		DataLen++;
		*(TmpBuf + DataLen) = 0x0D;
		DataLen++;
		memcpy(source,TmpBuf,DataLen);
		tmp = DataLen;
		
		MsgNum = 0;
		DataLen = 0;
		MsgLine = 0;
	}
	return tmp;    //
}



/********************
**	完整数据包
**	天津勇猛机械协议：
**	终端信息消息体
**	终端状态信息消息体
**	高精度GNSS定位信息
**	玉米机CAN参数消息
**	车身当前故障码(尚未实现)
***********************/

unsigned short int build_complete_packet(unsigned char *source,unsigned char big)
{
	unsigned short int	DataLen = 0;

	MsgHeadStr 					TmpMsg;
	unsigned char				buf[100];          //
	MsgHeadStr 	 				*pMsgHead;         //
	
	MsgGnssStr					*pMsgGnss;         //
	MsgDeviceStr				*pMsgDevice;       //
	MsgInputStr 				*pMsgInput;        //
	MsgCanMaizeStr 				*pMsgMaize;       //
	
	
	struct can_info_str 							pCanData;         //需要修改
	struct time_str 		 now;         //时间
	//CurrentFaultCodeStr			*pFaultCode1;      //  故障码
	//
	MsgNum = 0;
	MsgLine = 0;
	
	TmpMsg.frame_start[0] = 0xF1;									//
	TmpMsg.frame_start[1] = 0xF2;									//
	TmpMsg.frame_start[2] = 0xFF;									//
	TmpMsg.msg_id = 0x45;											//
	if(read_terminal_id(buf,16))
	{
		memcpy(&TmpMsg.device_id,buf,16);         	//设备号
	}
	else   //(???)
	{
	}
	TmpMsg.CarTypeNum = 0x02;  										//车型编号
	TmpMsg.DataPackFlag = 0x01;   								 	//0：农机平台计亩；1：农机非平台计亩（或者不计亩）2：工程机械
	
	TmpMsg.msg_body_num = 0x00;                                     //
	TmpMsg.msg_len = 0x00;                                          //
	memcpy(TmpBuf,(unsigned char *)&TmpMsg,25);          		//
	DataLen = 25;													//

	if(TerMsgBig == 0)          //从新连接网络标志
	{
		pMsgDevice = (MsgDeviceStr *)buf;               				//
		pMsgDevice->MsgDeviceType = 0x0001;              	 			// 
		pMsgDevice->MsgDeviceLen = sizeof(MsgDeviceStr) - 4;  			//

		pMsgDevice->MsgManuNum = 0x01;            						// 厂家编码 0x01代表博创联动  		
		pMsgDevice->MsgTerminalType = 0x02;        						//终端型号：0x02代表Homer3E
		pMsgDevice->MsgUserNum = 30;                     				//使用方编号
		pMsgDevice->MsgAppVer1 = read_fir_ver();      		//单片机软件版
		pMsgDevice->MsgAppVer2 = read_fir_ver();          //
		pMsgDevice->MsgHardwareVer = read_config_info_hard_ware();             					//硬件版本
		memcpy(TmpBuf + DataLen,buf,sizeof(MsgDeviceStr));			// 
		DataLen += sizeof(MsgDeviceStr); 
		MsgNum++;
		TerMsgBig++;
	}
	
	//
	pMsgInput = (MsgInputStr *)buf;             //
	
	pMsgInput->MsgInputType = 0x0004;								//
	pMsgInput->MsgInputLen = sizeof(MsgInputStr) - 4;

	pMsgInput->MsgInputIo = read_acc_state();			//
	pMsgInput->MsgAcc = read_acc_state();    				// 

	pMsgInput->MsgMoto = 0;
	pMsgInput->MsgInputFrq1 = 0;								//
	pMsgInput->MsgInputFrq2 = 0;								//
	pMsgInput->MsgInputFrq3 = 0;								//
	pMsgInput->MsgInputFrq4 = 0;								//
	pMsgInput->MsgPowVol = read_power_vol();				//
	pMsgInput->MsgBatteryVol = read_batter_vol();			//
	pMsgInput->MsgInputVol1 = 0;								//
	pMsgInput->MsgInputVol2 = 0;								//
	pMsgInput->MsgWarnValue = 0;
	pMsgInput->MsgLine = 1;					//
 	 
	memcpy(TmpBuf + DataLen,buf,sizeof(MsgInputStr)); 
	DataLen += sizeof(MsgInputStr);
	MsgNum++;

	//
	pMsgGnss = (MsgGnssStr *)buf;
	
	read_time(&now);
	pMsgGnss->MsgGnssType = 0x0100;								//
	pMsgGnss->MsgGnssLen = sizeof(MsgGnssStr) - 4;				//
		
	pMsgGnss->MsgGnssLatitude = read_gnss_latitude() * 1000000;		//
	//fr_printf("the MsgGnssLatitude:%u\r\n",pMsgGnss->MsgGnssLatitude);
	pMsgGnss->MsgGnssLongitude = read_gnss_longitude() * 1000000;	//
	//fr_printf("the MsgGnssLongitude:%u\r\n",pMsgGnss->MsgGnssLongitude);
	pMsgGnss->MsgGnssSpeed = read_gnss_speed();		//行驶速度
	pMsgGnss->MsgGnssAzimuth = read_gnss_heading();					//
	pMsgGnss->MsgGnssAltitude = read_gnss_altitude();				//
	pMsgGnss->MsgGnssYear = now.year; 					//Gnss
	pMsgGnss->MsgGnssMon = now.mon;						//Gnss
	pMsgGnss->MsgGnssDay = now.day;						//Gnss
	pMsgGnss->MsgGnssHour = now.hour; 					//Gnss 
	pMsgGnss->MsgGnssMin = now.min;						//Gnss 
	pMsgGnss->MsgGnssSec = now.sec;						//Gnss
		
	pMsgGnss->MsgGnssSatelliteNum = read_satellite_num();		//
	pMsgGnss->MsgGnssViewNum = read_bd_sate_num() + read_gps_sate_num();		//	可视卫星数量(GPS + BD)
	pMsgGnss->MsgGhdopV = read_gnss_hdop() ;	
	//
	if(read_gnss_state() == 'A')
		pMsgGnss->MsgGnssStatus = pMsgGnss->MsgGnssStatus | 0x02; 		//  
	else
		pMsgGnss->MsgGnssStatus = pMsgGnss->MsgGnssStatus & 0xFD;
	
	if(read_ant_state() > 0)    //判断天线状态
		pMsgGnss->MsgGnssStatus |= 0x01;
	else
		pMsgGnss->MsgGnssStatus &= 0xFE;

	memcpy(TmpBuf + DataLen,buf,sizeof(MsgGnssStr)); 		//
	DataLen += sizeof(MsgGnssStr);
	MsgNum++;

	
	//
	pMsgMaize = (MsgCanMaizeStr *)buf;    //
	read_can_info(&pCanData);
	//
	switch(read_car_type())
	{ 
		case 0x0A:						 						 						//
			pMsgMaize->MsgMaizeMechType = 0x000C;										//
			pMsgMaize->MsgMaizeMechLen = 78;        									//
			
			pMsgMaize->MsgSysVol = pCanData.sys_vol;           	 						//
			//fr_printf("the sys _vol:%d\r\n",pMsgMaize->MsgSysVol);
			pMsgMaize->MsgTempWater = pCanData.temp_water;           		//
			pMsgMaize->MsgEngineRotate = pCanData.engine_actual_rotate;		 		//发动机转速
			pMsgMaize->MsgEngineRotateSet = pCanData.engine_set_rotate;     		//
			pMsgMaize->MsgEngineTorque = pCanData.engine_torque;				//
			pMsgMaize->MsgFuelTemp = pCanData.fuel_temp;					//
			pMsgMaize->MsgOilTemp = pCanData.oil_temp;         			//
			pMsgMaize->MsgAirPressure = pCanData.air_pressure;         		//
			pMsgMaize->MsgEngineNacelleTemp = pCanData.engine_nacelle_temp;			//
			pMsgMaize->MsgAirTemp = pCanData.air_temp;					//
			pMsgMaize->MsgEnteredAirTemp = pCanData.entere_air_temp;				//
			pMsgMaize->MsgEngineWorkTime = pCanData.engine_work_time;				//发动机工作时长
			pMsgMaize->MsgTravelSpeed = pCanData.travel_speed;				//行驶速度
			pMsgMaize->MsgOnceTravel = pCanData.once_travel;					//
			pMsgMaize->MsgTotalTravel = pCanData.total_travel;  				//
			pMsgMaize->MsgOnceFuel = pCanData.once_fuel;      				//
			pMsgMaize->MsgTotalFuel = pCanData.total_fuel;  				//
			pMsgMaize->MsgRelativeOilPressure = pCanData.relative_oil_pressure;		//
			pMsgMaize->MsgAbsoluteOilPressure = pCanData.absolute_oil_pressure;		//
			pMsgMaize->MsgRelativeAddPressure = pCanData.relative_add_pressure;		//
			pMsgMaize->MsgAbsoluteAddPressure = pCanData.absolute_add_pressure;		//
			pMsgMaize->MsgFuelNum = pCanData.fuel_position;					//燃油位置		
			pMsgMaize->MsgFuelPercent = pCanData.fuel_percent;				//
			pMsgMaize->MsgOilPosition = pCanData.oil_position;				//机油位置
			pMsgMaize->MsgCrankcasePressure = pCanData.crankcase_pressure;   		//
			pMsgMaize->MsgCoolPressure = pCanData.cool_pressure;  				//
			pMsgMaize->MsgCoolPosition = pCanData.cool_position;				//
			pMsgMaize->MsgLockCarState = pCanData.lock_car_state;				//
			pMsgMaize->MsgActivateStatus = pCanData.activate_status;   			//
			pMsgMaize->MsgKeyStatus = pCanData.key_status;     				//
			pMsgMaize->MsgTerIDStatus = pCanData.ter_id_status;				//
			pMsgMaize->MsgWorkFlag = pCanData.work_flag;   						//
			pMsgMaize->MsgCarWarnValue1 =pCanData.car_warn_value1;				//
			pMsgMaize->MsgCarWarnValue2 = pCanData.car_warn_value2;				//
			pMsgMaize->MsgFoodFullWarn = pCanData.food_full_warn;				//
			pMsgMaize->MsgClutchState = pCanData.clutch_state;  				//
			pMsgMaize->MsgStripRotate = pCanData.strip_rotate;				//
			pMsgMaize->MsgLiftRotate = pCanData.lift_rotate;					//
			pMsgMaize->MsgCutTableHigh = pCanData.cut_table_high;				//割台高度
			pMsgMaize->MsgNC1 = 0;
			pMsgMaize->MsgNC2 = 0;
			pMsgMaize->MsgNC3 = 0;
			pMsgMaize->MsgNC4 = 0;     //
			memcpy(TmpBuf + DataLen,buf,sizeof(MsgCanMaizeStr)); 		//
			DataLen += sizeof(MsgCanMaizeStr);
			MsgNum++;
			break;
		default:
			break;
	}


//	pFaultCode1 = GetCurrentFaultCodeSpace();	  //
//	//SL_Print("Run This Is :%d\r\n",pFaultCode1->FaultCodeNum);   //
//	if(pFaultCode1->FaultCodeNum > 0 && pFaultCode1->FaultCodeNum <= 160)	  //
//	{
//		IntToChar TmpInt16;
//	
//		TmpInt16.IntII = 0x0011;					  //
//		buf[0] = TmpInt16.TTbyte[0];
//		buf[1] = TmpInt16.TTbyte[1];
//	
//		TmpInt16.IntII = pFaultCode1->FaultCodeNum * 4 + 2;   //
//				
//		buf[2] = TmpInt16.TTbyte[0];
//		buf[3] = TmpInt16.TTbyte[1];
//					
//		SL_Memcpy(TmpBuf + DataLen,buf,4);			 // 
//		DataLen += 4;
//				
//		*(TmpBuf + DataLen) = pFaultCode1->FaultCodeNum;   //
//		DataLen++;
//		SL_Memcpy(TmpBuf + DataLen,pFaultCode1->FaultCode,pFaultCode1->FaultCodeNum * 4);
//		DataLen += pFaultCode1->FaultCodeNum*4;
//	
//		*(TmpBuf + DataLen) = 0x01;
//		DataLen += 1;
//		MsgNum++;
//					
//	}

	pMsgHead = (MsgHeadStr *)TmpBuf;                   										//
	pMsgHead->msg_len = DataLen - 23;          //
	pMsgHead->msg_body_num = MsgNum;  
	MsgSerial++;
	memcpy(TmpBuf + DataLen,(unsigned char *)&MsgSerial,2); 
	
	DataLen += 2;
	
	*(TmpBuf + DataLen) = BccVerify(TmpBuf + 3,DataLen - 3);
	DataLen++;
	*(TmpBuf + DataLen) = 0x0D;
	DataLen++;
	memcpy(source,TmpBuf,DataLen);
	MsgNum = 0;
	return DataLen;
}




/********************************
**	ser_num:回应命令序列号
********************************/

unsigned char build_general_response(unsigned char *buf,unsigned short int buf_size,unsigned short int ser_num,unsigned char res,unsigned short cmd_id)
{
	unsigned short int len;
	SysCmdStr m_tmp;
	int16_to_char m;
	
	if(buf_size < 50)
		return 0;
	
	len = 0;
	
	m_tmp.FrameStart[0] = 0xF1;
	m_tmp.FrameStart[1] = 0xF2;
	m_tmp.FrameStart[2] = 0xFF;
	read_terminal_id((unsigned char *)&m_tmp.device_id,16);
	len += sizeof(SysCmdStr);
	
	switch(cmd_id)
	{
		case 0:
			return 0;
		case 0x0200:           //解锁车
		case 0x0201:					 //锁车
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			if(res == 1)
			{
				if(cmd_id == 0x0200)
						*(buf + len) = 0x00;     //
				else if(cmd_id == 0x0201)
						*(buf + len) = 0x08;
			}
			else 
			{
				*(buf + len) = 9;        //解锁或者锁车失败
			}
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 1;
			m_tmp.msg_body_num = 1;
			m_tmp.msg_len = 5;
			m_tmp.DataPackFlag = ser_num;
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			
			return len;
		case 0x0203: 
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			if(res == 1)
					*(buf + len) = 0x00;     //
			else 
					*(buf + len) = 0x01;
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 1;
			m_tmp.msg_body_num = 1;
			m_tmp.msg_len = 5;
			m_tmp.DataPackFlag = ser_num;
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			
			return len;
		case 0xFD00:     //
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			*(buf + len) = 0x00;     //
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);    // 
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 0x01;	//
			m_tmp.msg_body_num = 1;  //
			m_tmp.msg_len = 5;		//
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			
			return len;
		case 0xFE00:         //
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			*(buf + len) = 0x00;     //
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);    // 
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 0x01;	//
			m_tmp.msg_body_num = 1;  //
			m_tmp.msg_len = 5;		//
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			return len;
		case 0x0001:            //
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			*(buf + len) = 0x00;     //
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);    // 
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 0x01;	//
			m_tmp.msg_body_num = 1;  //
			m_tmp.msg_len = 5;		//
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			return len;
		case 0x0003:           //  
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			*(buf + len) = 0x00;     //
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);    // 
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 0x01;	//
			m_tmp.msg_body_num = 1;  //
			m_tmp.msg_len = 5;		//
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			return len;
		case 0x0005:
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			*(buf + len) = 0x00;     //
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);    // 
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 0x01;	//
			m_tmp.msg_body_num = 1;  //
			m_tmp.msg_len = 5;		//
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			return len;
		case 0x0009:
			m.value = cmd_id;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //消息体类型码
			len++;
			m.value = 1;
			*(buf + len) = m.byte[0];
			len++;
			*(buf + len) = m.byte[1];    //数据长度
			len++;
			*(buf + len) = 0x00;     //
			len++;
			*(buf + len) = BccVerify(buf + 3,len - 3);    // 
			len++;
			*(buf + len) = 0x0d;
			len++;
			m_tmp.msg_id = 0x01;	//
			m_tmp.msg_body_num = 1;  //
			m_tmp.msg_len = 5;		//
			memcpy(buf,(unsigned char *)&m_tmp,sizeof(m_tmp));
			return len;
		default:
			break;
	}
	
	return 0;
}


