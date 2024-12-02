
/***********************************************
** 	FileName:
**	Time:
************************************************/

#ifndef PRODATA_H
#define PRODATA_H

#include <stdint.h>

unsigned char DecToHex(unsigned char dec);
unsigned char HexToDec(unsigned char bcd);
int LookForStr(unsigned char *buf,unsigned char *str,int datasize);
unsigned char Int16ToStr(unsigned short int number,char *str);
void AsciiToData(char *des, char *src, int size);
unsigned char CalcCrc8(unsigned char *start, int size);
unsigned char verfy_sum(unsigned char *dest,unsigned short int len);
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
unsigned char str_compare(unsigned char *Str,unsigned char *Source,unsigned short int Len);
unsigned char ip_addr_to_str(unsigned char *source,unsigned char n1,unsigned char n2,unsigned char n3,unsigned char n4);
uint8_t get_file_verify(char *name, uint32_t *verify);
uint16_t hex_to_str(uint8_t* source, uint16_t len, uint8_t* buf, uint16_t buf_size);
unsigned short int calc_crc16(unsigned char *start, int size);
//uint16_t calc_crc16(uint16_t crc_u16,uint8_t *data_pu8, uint32_t length_u32);

uint8_t calc_xor_verify(uint8_t *buf,uint32_t len);
uint16_t swap_uint16_t(uint16_t n);
uint32_t swap_uint32_t(uint32_t n);
uint16_t get_buf_str(uint16_t n,uint16_t m,uint8_t deschar,uint8_t *str,uint8_t *source,uint16_t str_len);
uint16_t search_char(uint16_t num,uint8_t deschar,uint8_t *str,uint16_t str_len);
int list_for_str(uint8_t *buf,uint8_t *str,int datasize);


#endif




/***************************File End**************************/


