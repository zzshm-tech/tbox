/**********************************************************************
**	File Name:data_process.c
**	Time:2016-02-20
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>


#define EARTH_RADIUS  						6378.137


/**********************************************************************
**	函数名称:
** 	功能描述:
***********************************************************************/

unsigned char DecToHex(unsigned char dec)
{
	unsigned char rv;
	
	if(dec > 99)
		return 0;
	
	rv = ((dec / 10) << 4) + (dec % 10);
	
	return rv;
}


/**********************************************************************
**	函数名称:
** 	功能描述:
***********************************************************************/

unsigned char HexToDec(unsigned char bcd)
{
	unsigned char rv ;
	
	if(bcd > 0x99)
		return 0;
	
	rv = ((bcd >> 4) * 10) + (bcd & 0x0f);
	
	return rv;
}


/*******************************************
**	unsigned   int   		0～4294967295
** 	int      						-2147483648～2147483647 
**	
*******************************************/

unsigned char int_to_str(unsigned int num,char *str,unsigned char buf_size)
{
	int i = 0;//指示填充str 
  int j = 0;
	
	if(buf_size < 12)
		return 0;
	
//	if(num<0)//如果num为负数，将num变正 
//  {
//		num = -num;
//    str[i++] = '-';
//  } 
    //转换 
  do
  {
    str[i++] = num % 10 + 48;//取num最低位 字符0~9的ASCII码是48~57；简单来说数字0+48=48，ASCII码对应字符'0' 
    num /= 10;//去掉最低位    
  }while(num);//num不为0继续循环
    
  str[i] = '\0';
    
  //确定开始调整的位置 
  
  if(str[0]=='-')//如果有负号，负号不用调整 
  {
     j = 1;//从第二位开始调整 
     ++i;//由于有负号，所以交换的对称轴也要后移1位 
  }
    //对称交换 
  for(;j < i/2;j++)
  {
        //对称交换两端的值 其实就是省下中间变量交换a+b的值：a=a+b;b=a-b;a=a-b; 
		str[j] = str[j] + str[i-1-j];
    str[i-1-j] = str[j] - str[i-1-j];
    str[j] = str[j] - str[i-1-j];
  } 
	return i;
}


/************************************************************************
**	函数名称:
** 	功能描述:
*************************************************************************/

void AsciiToData(char *des, char *src, int size)
{
	unsigned short int i = 0;
	unsigned short int j = 0;
	unsigned char high_data;
	unsigned char low_data;
	
	while (i < size) 
	{
		if(src[i] <= '9' && src[i] >= '0')
			
			high_data = src[i] - '0';
		else
			high_data = src[i] - 'A' + 0x0A;
		i++;
		
		if (src[i] <= '9' && src[i] >= '0')
			low_data = src[i] - '0';
		else
			low_data = src[i] - 'A' + 0x0A;
			
		des[j] = low_data + (high_data << 4);
		
		j++;
		i++;
	}
}



/*********************************************************************
**	函数名称:
** 	功能描述:
**********************************************************************/

unsigned char CalcCrc8(unsigned char *start, int size)  
{
	unsigned char	 		i;
	unsigned char	 		crc8 = 0x00;
  unsigned char   	m_tmp;

	while(size > 0)
	{
		crc8 ^= *start;
		m_tmp = 0;
		
		for (i = 0; i < 8; i++)
		{
			if((m_tmp ^ crc8) & 0x01)
			{
				m_tmp ^= 0x18;
        m_tmp >>= 1;
        m_tmp |= 0x80;
      }
      else
			{
				m_tmp >>= 1;
      }
      
      crc8 >>= 1;
		}
		
		crc8 = m_tmp;
		start++;
		size--;
	}
	
	return crc8;
}






/**************************************************************************
**	函数名称:
** 	功能描述:
***************************************************************************/

unsigned char BccVerify(unsigned char *buf,unsigned int len)
{
	unsigned char s = 0;
  unsigned int i = 0;

	for(i = 0;i < len;i++)
	{
		s = s ^ (*(buf + i));
	}
	
	return s;
}

/************************************************************************
**	函数名称:
** 	功能描述:
*************************************************************************/

unsigned char VerfySum(unsigned char *dest,unsigned short int len)
{
	unsigned char sum = 0;
	
	while(len > 0)
	{
		sum += *dest;
		len--;
		dest++;
	}
	
	return sum;
}


/*************************************************************
**	函数名称:
** 	功能描述:
*************************************************************/

unsigned short int CheckSum16(unsigned char *buf,unsigned int len)					
{					
	unsigned int i=0;				
	unsigned int Sum=0;	
	
    unsigned short int CheckSum=0;					
					
	for(i = 0;i < len;i++)				
	{				
		Sum += *buf++;			
	}				
	 CheckSum = (Sum >>16) + (Sum & 0xffff);
	 
	return CheckSum;				
}



/*************************************************************
**	函数名称:
** 	功能描述:
*************************************************************/

unsigned short int CheckSum16B(unsigned char *buf,unsigned int len)					
{					
	unsigned int i=0;				
	unsigned int Sum=0;				
    unsigned short int CheckSum=0;					
					
	for(i = 0;i < len;i++)				
	{				
		Sum += *buf++;			
	}				
	 CheckSum = (Sum >>16) + (Sum & 0xffff);				
	return CheckSum;				
}





/*********************************************************************
**	函数名称:
** 	功能描述:
***********************************************************************/

unsigned char AsciiToHex(unsigned char a,unsigned char b)
{
    if(a >= '0' && a <= '9')
        a -= 48;
    else if(a >= 'A' && a <= 'F')				
        a -= 55;
    else if((a >= 'a')&&(a <= 'f'))				
        a -= 87;

    if(b >= '0' && b <= '9')
        b -= 48;
    else if((b >= 'A')&&(b <= 'F'))				
        b -= 55;
    else if((a >= 'a')&&(b <= 'f'))				
        b -= 87;

    return (a * 16 + b);
}





/*****************************************************************
**	函数名称:
** 	功能描述:
*****************************************************************/

double Rad(double d)
{
	return d * 3.1415926 / 180.0;
}




/****************************************************************
**	函数名称:
** 	功能描述:
*****************************************************************/

double PowTow(double x)
{

	return x * x;
}




/*****************************************************************************
**	函数名称:
** 	功能描述:
******************************************************************************/

double ComputeDistance(double lat1, double lng1, double lat2, double lng2)
{
	double s;
	double radLat1;
	double radLat2;
	double a;
	double b;
	
	radLat1 = Rad(lat1);
	radLat2 = Rad(lat2);

	a = radLat1 - radLat2;
	b = Rad(lng1) - Rad(lng2);

	s = 2 * sin(sqrt(PowTow(sin(a/2)) + cos(radLat1) * cos(radLat2) * PowTow(sin(b/2))));
	
	s = s * EARTH_RADIUS;
	
	return s;
}



/**********************************************************
**	函数名称:
** 	功能描述:
************************************************************/

void CharToStr(unsigned char n,unsigned char *buf,unsigned char len)
{
	if(len < 2)
		return;
	if(n > 99)
		return;
	buf[0] = (n / 10) + 0x30;
	buf[1] = (n % 10) + 0x30;
}


/************************************************************
**	函数名称:
** 	功能描述:
************************************************************/

int UInt32ToStr(unsigned int n,unsigned char *buf,unsigned char len)
{
    unsigned char tmp[12];
    unsigned int    a;

    if(len < 11)
        return -1;      //
    a = 0;
    if(n == 0)
    {
        buf[0] = '0';
        return 1;
    }
    while(n != 0)
    {
        tmp[a++] = n % 10 + 0x30;
        n /= 10;
    }
    for(n = 0;n < a;n++)
        buf[n] = tmp[a - n -1];
	
    return a;
}




/*********************************************************
**	函数名称:
** 	功能描述:
**********************************************************/

unsigned char CheckServerAddr(unsigned char *Data,unsigned int Len)
{
    unsigned char *p;
    unsigned short int tmp;

    if(Len > 15)                    //
        return 0;

    p = Data;

    if(*(p + 1) == '.')
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        p += 2;
    }
    else if(*(p + 2) == '.')
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        if(*(p + 1) > '9' || *(p + 1) < '0')     //
            return 0;
        p += 3;
    }
    else if(*(p + 3) == '.')
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        if(*(p + 1) > '9' || *(p + 1) < '0')     //
            return 0;
        if(*(p + 2) > '9' || *(p + 2) < '0')
            return 0;
        tmp = (*p - 0x30) * 100 + (*(p + 1) - 0x30) * 10 + *(p + 2) - 0x30;
        if(tmp > 255)
            return 0;
        p += 4;
    }
    else
        return 0;                        //

    if(*(p + 1) == '.')                  //
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        p += 2;
    }
    else if(*(p + 2) == '.')
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        if(*(p + 1) > '9' || *(p + 1) < '0')     //
            return 0;
        p += 3;
    }
    else if(*(p + 3) == '.')
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        if(*(p + 1) > '9' || *(p + 1) < '0')     //
            return 0;
        if(*(p + 2) > '9' || *(p + 2) < '0')
            return 0;
        tmp = (*p - 0x30) * 100 + (*(p + 1) - 0x30) * 10 + *(p + 2) - 0x30;
        if(tmp > 255)
            return 0;
        p += 4;
    }
    else
        return 0;                        //

    if(*(p + 1) == '.')
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        p += 2;
    }
    else if(*(p + 2) == '.')
    {
        if(*p > '9' || *p < '0')       //
            return 0;
        if(*(p + 1) > '9' || *(p + 1) < '0')     //
            return 0;
        p += 3;
    }
    else if(*(p + 3) == '.')
    {
        if(*p > '9' || *p < '0')       
            return 0;
        if(*(p + 1) > '9' || *(p + 1) < '0')     //
            return 0;
        if(*(p + 2) > '9' || *(p + 2) < '0')
            return 0;
        tmp = (*p - 0x30) * 100 + (*(p + 1) - 0x30) * 10 + *(p + 2) - 0x30;
        if(tmp > 255)
            return 0;
        p += 4;
    }
    else
        return 0;                        //

    if(*p == '\0')
        return 0;

    if(*(p + 1) == '\0' || *(p + 2) == '\0')
        return 1;
    if(*(p + 3) == '\0')
    {
        tmp = (*p - 0x30) * 100 + (*(p + 1) - 0x30) * 10 + *(p + 2) - 0x30;
        if(tmp > 255)
            return 0;
    }
    return 1;
}



/*****************************************
**	函数名称:
** 	功能描述:
******************************************/

static unsigned char CharToStrB(unsigned char n,unsigned char * buf)
{
	if(n < 9)
  	{
    	buf[0] = n + 0x30;
		return 1;
  	}
  	else if(n < 99)
  	{
    	buf[0] = n / 10 + 0x30;
    	buf[1] = n % 10 + 0x30;
		return 2;
  	}
  	else if(n <= 255)
  	{
  		buf[0] = n / 100 + 0x30;
		buf[1] = n / 10 % 10 + 0x30;
		buf[2] = n % 10 + 0x30;
		return 3;
  	}
	return 0;
}


/*****************************************
**	函数名称:
** 	功能描述:
******************************************/

unsigned char IpAddrToStr(unsigned char *source,unsigned char n1,unsigned char n2,unsigned char n3,unsigned char n4)
{
	unsigned char buf[5];
	unsigned char tmp;
	unsigned char *p;

	p = source;
	tmp = CharToStrB(n1,buf);
	if(tmp > 0 && tmp < 4)
		memcpy(p,buf,tmp);
	else
		return 0;
	p += tmp;
	*p = '.';
	p += 1;
	tmp = CharToStrB(n2,buf);
	if(tmp > 0 && tmp < 4)
		memcpy(p,buf,tmp);
	else
		return 0;
	p += tmp;
	*p = '.';
	p += 1;
	tmp = CharToStrB(n3,buf);
	if(tmp > 0 && tmp < 4)
		memcpy(p,buf,tmp);
	else
		return 0;
	p += tmp;
	*p = '.';
	p += 1;
	tmp = CharToStrB(n4,buf);
	if(tmp > 0 && tmp < 4)
		memcpy(p,buf,tmp);
	else
		return 0;
	p += tmp;
	*p = '\0';
	
	return 1;
	
}	



unsigned int calc_CRC32(unsigned char *start, int size)
{
	unsigned char	 i;
	unsigned char	 LSB;				
	unsigned int	 CRC32 = 0xFFFFFFFF;
	
	while(size > 0) 
	{
		*(unsigned char*)&CRC32 ^= *start;
		for (i = 0; i < 8; i++) 
		{
			LSB = (unsigned char)CRC32 & 0x01 ? 1 : 0;
			CRC32 >>= 1;
			if (LSB) 
				CRC32 ^= 0xEDB88320;
		}
		start++;
		size--;
	}
	CRC32 = ~CRC32;
	
	return CRC32;
}




/******************************
**	函数名称:
** 	功能描述: 比较字符
*******************************/

unsigned char StrCompare(unsigned char *Str,unsigned char *Source,unsigned short int Len)
{
	unsigned short int i;
	
	for(i = 0;i < Len;i++)
	{
		if(*(Str + i) != *(Source + i))
			return 0;
	}
	return 1;
}



/*************************************************
**	函数名称：
**	功能描述：
**************************************************/

unsigned char get_comma_posi(unsigned char num,unsigned char *str,unsigned char str_len)
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



/*********************************************
**	函数名称:
**	功能描述:
*******************************************/
unsigned char get_data_str(unsigned char n,unsigned char m,unsigned char *str,unsigned char *source,unsigned char str_len)
{
	unsigned char a,b;
	unsigned char *p;
	
	a = get_comma_posi(n,str,str_len);     //获取逗号位置
	b = get_comma_posi(m,str,str_len);
	
	if(a == 0 || b == 0)
		return 0;
	
	p = str + a;
	a = b - a - 1;
	if(a > 50)
		return 0;
	
	memcpy(source,p,a);
	
	return a;
}


/*********************************************************
**	函数名称:
**	功能描述:
**********************************************************/

unsigned char nmea_verfy(unsigned char *source)	
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



/*************************************************
**	函数名称:
** 	功能描述:
**************************************************/

int look_for_str(unsigned char *buf,unsigned char *str,int datasize)
{
	unsigned char	 ch;
	unsigned char	*str_cpy;
	unsigned short int		 index = 0;
	unsigned short int		 index_cpy;
	
	while (index < datasize) 
	{
		str_cpy = str;
		index_cpy = index;
		
		while ((ch = *str_cpy++) != '\0' && index_cpy < datasize && ch == buf[index_cpy++]);
		
		if(ch == '\0')
			return (index);
		
		index++;
	}
	return -1;
}


/*********************
**
**********************/

double fr_atof(const char* sptr)
{
    double temp=10;
    unsigned char  ispnum = 1;
    double ans=0;
    if(*sptr=='-')//判断是否是负数
    {
        ispnum = 0;
        sptr++;
    }
    else if(*sptr=='+')//判断是否为正数
    {
        sptr++;
    }


    while(*sptr!='\0')//寻找小数点之前的数
    {
        if(*sptr=='.')
				{ 
					sptr++;
					break;
				}
        ans = ans * 10 + (*sptr - '0');
        sptr++;
    }
    while(*sptr != '\0')//寻找小数点之后的数
    {
        ans = ans + (*sptr - '0') / temp;
        temp *= 10;
        sptr++;
    }
    if(ispnum) 
			return ans;
    else 
			return ans*(-1);
}


/**********************
**
************************/

int fr_atoi(const char* sptr)
{
	unsigned char ispnum = 1;
  int ans=0;
    
	if(*sptr == '-')//判断是否是负数
  {
		ispnum = 0;
    sptr++;
  }
  else if(*sptr == '+')//判断是否为正数
  {
		sptr++;
  }


    while(*sptr!='\0')//类型转化
    {
        ans=ans*10+(*sptr-'0');
        sptr++;
    }


    if(ispnum) return ans;
    else return ans*(-1);
}



/********************************************File End*************************************************/












