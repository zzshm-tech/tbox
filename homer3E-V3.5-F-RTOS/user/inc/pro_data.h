
/***********************************************
** 	FileName:
**	Time:
************************************************/

#ifndef PRODATA_H
#define PRODATA_H



unsigned char DecToHex(unsigned char dec);
unsigned char HexToDec(unsigned char bcd);
int LookForStr(unsigned char *buf,unsigned char *str,int datasize);
unsigned char Int16ToStr(unsigned short int number,char *str);
void AsciiToData(char *des, char *src, int size);
unsigned char CalcCrc8(unsigned char *start, int size);
unsigned char BccVerify(unsigned char *buf,unsigned int len);
unsigned char VerfySum(unsigned char *dest,unsigned short int len);
unsigned char AsciiToHex(unsigned char a,unsigned char b);
double ComputeDistance(double lat1, double lng1, double lat2, double lng2);
unsigned short int CheckSum16(unsigned char *buf,unsigned int len);
unsigned short int CheckSum16B(unsigned char *buf,unsigned int len)	;
double PowTow(double x);
void CharToStr(unsigned char n,unsigned char *buf,unsigned char len);
int UInt32ToStr(unsigned int n,unsigned char *buf,unsigned char len);
unsigned char CheckServerAddr(unsigned char *Data,unsigned int Len);
unsigned char IpAddrToStr(unsigned char *source,unsigned char n1,unsigned char n2,unsigned char n3,unsigned char n4);
unsigned char StrCompare(unsigned char *Str,unsigned char *Source,unsigned short int Len);
int look_for_str(unsigned char *buf,unsigned char *str,int datasize);
unsigned char int_to_str(int num,char *str,unsigned char buf_size);
unsigned char get_data_str(unsigned char n,unsigned char m,unsigned char *str,unsigned char *source,unsigned char str_len);
unsigned char nmea_verfy(unsigned char *source);
double fr_atof(const char* sptr);
unsigned int calc_CRC32(unsigned char *start, int size);
unsigned char get_comma_posi(unsigned char num,unsigned char *str,unsigned char str_len);
int fr_atoi(const char* sptr);

#endif




/***************************File End**************************/


