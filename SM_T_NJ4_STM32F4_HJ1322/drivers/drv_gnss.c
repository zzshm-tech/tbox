

/*******************
**	FileName:
**	Time:
*************************/
#include <stdio.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "pro_data.h"
#include "data_type.h"

#include "drv_rtc.h"
#include "drv_ticks.h"
#include "drv_uart.h"

#include "drv_gnss.h"


#define endof(array)	(array + sizeof(array))


/****************本地全局变量****************/

static unsigned char 		GnssDataBuf[1000];    				//GNSS数据包缓冲区

static GNSSSTR 					TempGnssData;           			//

static unsigned char    TempBuf[20];            			//



/************************************
**	
************************************/

static unsigned char GnssDataVerfy(unsigned char *source)	
{
	unsigned char i,len,data_sum;
  	unsigned char *p;

  	p = source;

	for(i = 0;i < 90;i++)
	{
		if(*(p + i) == '*')
			break;
	}
	if(i > 90)
		return 0;
	
  	len = i;
  	data_sum = 0;
	
	for(i = 0;i < len;i++)
	{
		data_sum = data_sum ^ *p;
		p++;
	}
	p++;
	i =	AsciiToHex(*p,*(p + 1));

	if(data_sum	== i)
		return len;
	else
		return 0;
}




/***********************************
**
**********************************/

unsigned char GetComma(unsigned char num,unsigned char *str,unsigned char str_len)
{
	unsigned char i,j = 0;

	for(i = 0;i < str_len;i ++)
	{
		if(str[i] == ',')
			j++;
		if(j == num)
			return i + 1;	
	}

	return 0;	
}




/********************************************************************************
**	????:
** 	????:????double?
********************************************************************************/

double StrToDouble(unsigned char *buf)
{
	double rev = 0;
	double dat = 0;
	int integer = 1;
	unsigned char *str = buf;
	
	int i;
	
	i = 0;
	while(*str != '\0')
	{
		switch(*str)
		{
			case '0':
				dat = 0;
				break;
			case '1':
				dat = 1;
				break;
			case '2':
				dat = 2;
				break;		
			case '3':
				dat = 3;
				break;
			case '4':
				dat = 4;
				break;
			case '5':
				dat = 5;
				break;
			case '6':
				dat = 6;
				break;
			case '7':
				dat = 7;
				break;
			case '8':
				dat = 8;
				break;
			case '9':
				dat = 9;
				break;
			case '.':
				dat = '.';
				break;
		}
		if(dat == '.')
		{
			integer = 0;
			i = 1;
			str ++;
			continue;
		}
		if( integer == 1 )
		{
			rev = rev * 10 + dat;
		}
		else
		{
			rev = rev + dat / (10 * i);
			i = i * 10 ;
		}
		str ++;
	}
	return rev;
}




/***********************************
**
***********************************/
unsigned char GetDataStr(unsigned char n,unsigned char m,unsigned char *str,unsigned char *source,unsigned char str_len)
{
	unsigned char a,b;
	unsigned char *p;
	
	a = GetComma(n,str,str_len);
	b = GetComma(m,str,str_len);
	
	if(a == 0 || b == 0)
		return 0;
	
	p = str + a;
	a = b - a - 1;
	if(a > 50)
		return 0;
	
	memcpy(source,p,a);
	
	return a;
}





/*********************************
**	把解析到的数据存放在
**	获取定位模块的原始数据
*******************************/

unsigned char get_gnss_data(GNSSSTR *p_g)
{           
	unsigned short int 						m_data_len;
	unsigned char 								m_str_len;
	unsigned char 								*p;
	int 													index;
	
	m_data_len =  read_com_pkt(3,GnssDataBuf,sizeof(GnssDataBuf));   

	if(m_data_len == 0) 
		return 0;           //说明没收到数据
	
	index = look_for_str(GnssDataBuf,(unsigned char *)"GNRMC",m_data_len);         //
	p = GnssDataBuf + index;
	if((index > 0) && (p + 100) < endof(GnssDataBuf))     //
	{
		m_str_len = GnssDataVerfy(GnssDataBuf + index);
			
		if(m_str_len > 0)                                 								//
		{
				if(GetDataStr(2,3,p,TempBuf,m_str_len) > 0)    							//
				{
					TempGnssData.Status = TempBuf[0];               					//定位状态
				}
				else
				{
					TempGnssData.Status = 0; 
				}
				if(GetDataStr(1,2,p,TempBuf,m_str_len) > 0)    					   //
				{
					TempGnssData.THour = (TempBuf[0] - '0') * 10 + (TempBuf[1] - '0');  //时间
					TempGnssData.TMin =(TempBuf[2] - '0') * 10 + (TempBuf[3] - '0');    //
					TempGnssData.TSec = (TempBuf[4] - '0') * 10 + (TempBuf[5] - '0');   //
				}
				else
				{
					
					TempGnssData.THour = 0;   //utc-hour
					TempGnssData.TMin = 0;    //utc-min
					TempGnssData.TSec = 0;    //utc-sec
				}
				if((index = GetDataStr(3,4,p,TempBuf,m_str_len)) > 0)    		//维度
				{
					TempBuf[index] = '\0';
					TempGnssData.Latitude = StrToDouble(TempBuf);             // 
				}
				else
				{
					TempGnssData.Latitude = 0;
				}
				if(GetDataStr(4,5,p,TempBuf,m_str_len) > 0)    							//维度标志
				{
					if(TempBuf[0] == 'N')
						TempGnssData.latitude_ns = NORTH_LATITUDE;                 //
					else if(TempBuf[0] == 'S')
						TempGnssData.latitude_ns = SOUTH_LATITUDE;  								//
				}
				
				if((index = GetDataStr(5,6,p,TempBuf,m_str_len)) > 0)    		//经度
				{
					TempBuf[index] = '\0';
					TempGnssData.Longitude = StrToDouble(TempBuf); 
				}
				else
				{
					TempGnssData.Longitude = 0;
				}
				
				if(GetDataStr(6,7,p,TempBuf,m_str_len) > 0)    							//经度标志
				{
					if(TempBuf[0] == 'E')
						TempGnssData.longitude_ew = EAST_LONGTITUDE;               //
					else if(TempBuf[0] == 'W')
						TempGnssData.longitude_ew = WEST_LONGTITUDE;  						//
				}
				
				if((index = GetDataStr(7,8,p,TempBuf,m_str_len)) > 0)    	  //速度
				{
					TempBuf[index] = '\0';
					TempGnssData.Speed = (float)StrToDouble(TempBuf);         //
				}
				else
				{
					
				}
				if((index = GetDataStr(8,9,p,TempBuf,m_str_len)) > 0)    		//
				{
					TempBuf[index] = '\0';
					TempGnssData.Azimuth = (unsigned short int)StrToDouble(TempBuf);
				}
				if(GetDataStr(9,10,p,TempBuf,m_str_len) > 0)    							//
				{
					TempGnssData.TYear = (TempBuf[4] - '0') * 10 + (TempBuf[5] - '0');  			//
					TempGnssData.TMon = (TempBuf[2] - '0') * 10 + (TempBuf[3] - '0');      //
					TempGnssData.TDay= (TempBuf[0] - '0') * 10 + (TempBuf[1] - '0');      //
				}
			}
		}
		
		index = look_for_str(GnssDataBuf,(unsigned char *)"GNGGA",m_data_len);
		
		p = GnssDataBuf + index;
		
		if((index > 0) && (p + 100) < endof(GnssDataBuf))     	//
		{
			m_str_len = GnssDataVerfy(p);
			if(m_str_len > 0)                                 					//
			{
				if((index = GetDataStr(7,8,p,TempBuf,m_str_len)) > 0)    //
				{
					TempBuf[index] = '\0';
					TempGnssData.Satellite_num = (unsigned char)StrToDouble(TempBuf);   //定位使用的卫星数量
				}
				else
				{
					TempGnssData.Satellite_num = 0;
				}
				
				if((index = GetDataStr(8,9,p,TempBuf,m_str_len)) > 0)    //
				{
					TempBuf[index] = '\0';
					TempGnssData.Ghdop_v = (float)StrToDouble(TempBuf);  //水平经度因子
				}
				else
				{
					TempGnssData.Ghdop_v = 0;
				}
				if((index = GetDataStr(9,10,p,TempBuf,m_str_len)) > 0)    //
				{
					TempBuf[index] = '\0';
					TempGnssData.Altitude = (unsigned short int)StrToDouble(TempBuf);     //海拔高度
				}
				else
				{
					TempGnssData.Altitude = 0;   //注意这个正负值
				}
			}
		}
		
		index = look_for_str(GnssDataBuf,(unsigned char *)"GPGSV",m_data_len);  
		
		p = GnssDataBuf + index;
		
		if((index > 0) && (p + 100) < endof(GnssDataBuf))     //
		{
			m_str_len = GnssDataVerfy(p);
			if(m_str_len > 0)                                 //
			{
				if((index = GetDataStr(3,4,p,TempBuf,m_str_len)) > 0)    //
				{
					TempBuf[index] = '\0';
					TempGnssData.Satellite_view_num = (unsigned char)StrToDouble(TempBuf);
				}
				else
				{
					TempGnssData.Satellite_view_num = 0;
				}
			}
		}
		
		index = look_for_str(GnssDataBuf,(unsigned char *)"BDGSV",m_data_len);
		p = GnssDataBuf + index;
		
		if((index > 0) && (p + 100) < endof(GnssDataBuf))     //
		{
			m_str_len = GnssDataVerfy(p);
			if(m_str_len > 0)                                 						//
			{
				if((index = GetDataStr(3,4,p,TempBuf,m_str_len)) > 0)   			 //
				{
					TempBuf[index] = '\0';
					TempGnssData.bd_view_num = (unsigned char)StrToDouble(TempBuf);     //北斗可视微信数量
				}
				else
				{
					TempGnssData.bd_view_num = 0;
				}
			}
		}
		memcmp(p_g,(char *)&TempGnssData,sizeof(TempGnssData));
		return 1;
}









/***************File End*************/



