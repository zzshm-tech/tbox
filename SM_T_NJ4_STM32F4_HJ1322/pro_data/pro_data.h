
/***********************************************
** 	FileName:
**	Time:
************************************************/

#ifndef PRODATA_H
#define PRODATA_H

#include <stdint.h>


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
int look_for_str(uint8_t *buf,uint8_t *str,uint32_t datasize);
unsigned char int_to_str(int num,char *str,unsigned char buf_size);
uint8_t get_data_str(uint8_t n,uint8_t m,uint8_t *str,uint8_t *source,uint16_t str_len);
unsigned char nmea_verfy(unsigned char *source);
double fr_atof(const char* sptr);
unsigned int calc_CRC32(unsigned char *start, int size);
unsigned char get_comma_posi(unsigned char num,unsigned char *str,unsigned char str_len);
int fr_atoi(const char* sptr);
uint8_t str_compare(uint8_t *str,uint8_t *source,uint16_t len);
unsigned char ip_addr_to_str(unsigned char *source,unsigned char n1,unsigned char n2,unsigned char n3,unsigned char n4);
unsigned char get_file_verify(char *name, unsigned short int *verify);
uint16_t hex_to_str(uint8_t* source, uint16_t len, uint8_t* buf, uint16_t buf_size);
unsigned short int calc_crc16(unsigned char *start, int size);
uint16_t search_char(uint8_t num,uint8_t deschar,uint8_t *str,uint16_t str_len);
uint16_t get_buf_str(uint16_t n,uint16_t m,uint8_t deschar,uint8_t *str,uint8_t *source,uint16_t str_len);
uint16_t get_given_string(uint8_t *src, uint8_t ch, uint8_t *des);
uint8_t get_given_number(uint8_t *src, uint8_t ch, uint16_t *des);

uint8_t calc_xor_verify(uint8_t *buf,uint32_t len);
uint16_t swap_uint16_t(uint16_t n);
uint32_t swap_uint32_t(uint32_t n);
double swap_double_t(double n);
float swap_flaot_t(float n);
int list_for_str(uint8_t *buf,uint8_t *str,int datasize);



#endif




/***************************File End**************************/


