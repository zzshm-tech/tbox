

#include <stdarg.h>
#include <string.h>

#include  "mbrtu.h"

#define MBRTU_DEV_ADDR			 0
#define MBRTU_MEMORY_MW			 1


#define bitset(var, bitno) ((var) |= 1UL << (bitno))
#define bitclr(var, bitno) ((var) &= ~(1UL << (bitno)))
			

static unsigned char      unit_id;

static unsigned char      *mw_do;
static unsigned short int num_do;

static unsigned char      *mw_di;
static unsigned short int num_di;

static unsigned short int *mw_ao;
static unsigned short int num_ao;

static unsigned short int *mw_ai;
static unsigned short int num_ai;

//************************************************************
//函数功能：无符号16位整数高低自己转换
//************************************************************

static unsigned short int swap_uint16(unsigned int short n)
{
	unsigned short int	 rv;
	
	*(unsigned char*)&rv = *((unsigned char*)&n + 1);
	*((unsigned char*)&rv + 1) = *(unsigned char*)&n;
	
	return rv;
}

//************************************************************
//函数功能：   钩子函数，
//************************************************************
void init_mbrtu_lib(unsigned char *arg_mw_do, unsigned short int arg_num_do,
               unsigned char *arg_mw_di, unsigned short int arg_num_di,
               unsigned short int *arg_mw_ao, unsigned short int arg_num_ao,
               unsigned short int *arg_mw_ai, unsigned short int arg_num_ai,
			   unsigned char arg_unit_id)
{
	mw_do = arg_mw_do;
	num_do = arg_num_do;

	mw_di = arg_mw_di;
	num_di = arg_num_di;

	mw_ao = arg_mw_ao;
	num_ao = arg_num_ao;

	mw_ai = arg_mw_ai;
	num_ai = arg_num_ai;

	unit_id = arg_unit_id;
}

//************************************************************
//函数功能：计算Modbus-RTU CRC16
//************************************************************

static unsigned short int calc_CRC16(unsigned char *start, int size)
{
	unsigned char	 i;
	unsigned char	 LSB;				
	unsigned short int	 CRC16 = 0xFFFF;
	
	while(size > 0) 
	{
		*(unsigned char*)&CRC16 ^= *start;
		for (i = 0; i < 8; i++) 
		{
			LSB = (unsigned char)CRC16 & 0x01 ? 1 : 0;
			CRC16 >>= 1;
			if (LSB) 
				CRC16 ^= 0xA001;
		}
		start++;
		size--;
	}
	return CRC16;
}

//************************************************************
//函数功能:读线圈状态   读数字量输出状态，  AO输出 ，
//************************************************************

static int slave_deal_01H(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	   *ptr_rsp;
	unsigned char	   *ptr_mw_do;
	unsigned char	   byte_count;
	unsigned char	   bit_addr;
	unsigned short int starting_addr;
	unsigned short int coil_num;
	
	ptr_rsp = rsp;
	if (querysize != 8)
		return 0;
	starting_addr = swap_uint16(*(unsigned short int*)(query + 2));
	coil_num = swap_uint16(*(unsigned short int*)(query + 4));

	if (coil_num < 1 || coil_num > 2000)                                 //线圈的数量，限定在2000。
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x81;
		*ptr_rsp++ = 0x03;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	if (num_do == 0 || starting_addr > num_do - 1 || coil_num > num_do - starting_addr) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x81;
		*ptr_rsp++ = 0x02;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	byte_count = (coil_num + 7) >> 3;
	if (max_rspsize < byte_count + 5)
		return 0;
	*ptr_rsp++ = unit_id;
	*ptr_rsp++ = 0x01;
	*ptr_rsp++ = byte_count;
	ptr_mw_do = mw_do + starting_addr;
	*ptr_rsp = 0;
	bit_addr = 0x01;
	while (coil_num > 0) 
	{
		if((*ptr_mw_do++ & 0x01) == 1)
			*ptr_rsp |=bit_addr;
		bit_addr <<=1;
		if (bit_addr == 0) 
		{
			*++ptr_rsp = 0;
			bit_addr = 0x01;
		}
		coil_num--;
	}
	ptr_rsp++;
	*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, byte_count + 3);
	return (byte_count + 5);
}

//************************************************************
//函数功能：	多（开关）输入状态
//************************************************************

static int slave_deal_02H(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	   *ptr_rsp;
	unsigned char	   *ptr_mw_di;
	unsigned char	   byte_count;
	unsigned char	   bit_addr;
	unsigned short int starting_addr;
	unsigned short int discrete_num;
	
	ptr_rsp = rsp;
	if (querysize != 8)
		return 0;
	starting_addr = swap_uint16(*(unsigned short int*)(query + 2));
	discrete_num = swap_uint16(*(unsigned short int*)(query + 4));

	if (discrete_num < 1 || discrete_num > 2000) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x81;
		*ptr_rsp++ = 0x03;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	if (num_di == 0 || starting_addr > num_di - 1 || discrete_num > num_di - starting_addr) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x81;
		*ptr_rsp++ = 0x02;
		*(unsigned short int *)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	byte_count = (discrete_num + 7) >> 3;
	if (max_rspsize < byte_count + 5)
		return 0;
	*ptr_rsp++ = unit_id;
	*ptr_rsp++ = 0x02;
	*ptr_rsp++ = byte_count;
	ptr_mw_di = mw_di + starting_addr;
	*ptr_rsp = 0;
	bit_addr = 0x01;
	while (discrete_num > 0) 
	{
		if((*ptr_mw_di++ & 0x01) == 1)
			*ptr_rsp |=bit_addr;
		bit_addr <<=1;
		if (bit_addr == 0) 
		{
			*++ptr_rsp = 0;
			bit_addr = 0x01;
		}
		discrete_num--;
	}
	ptr_rsp++;
	*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, byte_count + 3);
	return (byte_count + 5);
}


//************************************************************
//函数功能：	读多保持寄存器  
//************************************************************

static int slave_deal_03H(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	    i;
	unsigned char	    *ptr_rsp;
	unsigned char	    byte_count;
	unsigned short int	*ptr_mw_ao;
	unsigned short int	 starting_addr;
	unsigned short int	 reg_num;
	
	ptr_rsp = rsp;
	if (querysize != 8)
		return 0;
	starting_addr = swap_uint16(*(unsigned short int*)(query + 2));  //开始地址
	reg_num = swap_uint16(*(unsigned short int*)(query + 4));        //数据长短，这个长度指的是16位数据。
	if (reg_num < 1 || reg_num > 127)                                //数据长度要在1-127之间。
	{
		if (max_rspsize < 5)                            
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x83;                                  //把命令码的最高位置1
		*ptr_rsp++ = 0x03; 
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	if (num_ao == 0 || starting_addr > num_ao - 1 || reg_num > num_ao - starting_addr)   //判断要读取数据空间的大小
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x83;
		*ptr_rsp++ = 0x02;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	byte_count = (unsigned char)(reg_num << 1);                      //返回的字节长度。
	if (max_rspsize < byte_count + 5)                                 
		return 0;                                    
	*ptr_rsp++ = unit_id;
	*ptr_rsp++ = 0x03;
	*ptr_rsp++ = byte_count;
	ptr_mw_ao = mw_ao + starting_addr;
	for (i = 0; i < (unsigned char)reg_num; i++) 
	{
		*(unsigned short int*)ptr_rsp = swap_uint16(*ptr_mw_ao++);
		ptr_rsp +=2;
	}
	*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, byte_count + 3);   //加上CRC校验
	return (byte_count + 5);
}

//************************************************************
//函数功能：	   读输入寄存器  就是度模拟量输入数据，
//************************************************************

static int slave_deal_04H(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	    i;
	unsigned char	    *ptr_rsp;
	unsigned char	    byte_count;
	unsigned short int	*ptr_mw_ai;
	unsigned short int	starting_addr;
	unsigned short int	reg_num;
	
	ptr_rsp = rsp;
	if (querysize != 8)
		return 0;
	starting_addr = swap_uint16(*(unsigned short int*)(query + 2));
	reg_num = swap_uint16(*(unsigned short int*)(query + 4));
	if (reg_num < 1 || reg_num > 127) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x84;
		*ptr_rsp++ = 0x03;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	if (num_ai == 0 || starting_addr > num_ai - 1 || reg_num > num_ai - starting_addr) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x84;
		*ptr_rsp++ = 0x02;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	byte_count = (unsigned char)(reg_num << 1);
	if (max_rspsize < byte_count + 5)
		return 0;
	*ptr_rsp++ = unit_id;
	*ptr_rsp++ = 0x04;
	*ptr_rsp++ = byte_count;
	ptr_mw_ai = mw_ai + starting_addr;
	for (i = 0; i < (unsigned char)reg_num; i++) 
	{
		*(unsigned short int*)ptr_rsp = swap_uint16(*ptr_mw_ai++);
		ptr_rsp +=2;
	}
	*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, byte_count + 3);
	return (byte_count + 5);
}

//************************************************************
//函数功能：   写单个线圈
//************************************************************

static int slave_deal_05H(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	   *ptr_rsp;
	unsigned char	   *ptr_mw_do;
	unsigned short int coil_addr;
	unsigned short int output_value;
	
	ptr_rsp = rsp;
	if (querysize != 8)
		return 0;

	coil_addr = swap_uint16(*(unsigned short int*)(query + 2));
	output_value = swap_uint16(*(unsigned short int*)(query + 4));

	if (output_value !=0x0000 && output_value != 0xff00) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x85;
		*ptr_rsp++ = 0x03;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	if (num_do == 0 || coil_addr >num_do - 1) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x85;
		*ptr_rsp++ = 0x02;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	ptr_mw_do = mw_do + coil_addr;
	if (output_value == 0x0000)
		*ptr_mw_do = 0x00;
	else
		*ptr_mw_do = 0x01;
	if (max_rspsize < 8)
		return 0;
	memcpy(rsp, query, 8);
	return 8;
}

//************************************************************
//函数功能：   写单个保持寄存器
//************************************************************

static short int slave_deal_06H(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	*ptr_rsp;
	unsigned short int	 reg_addr;
	
	ptr_rsp = rsp;
	if (querysize != 8)
		return 0;
	reg_addr = swap_uint16(*(unsigned short int*)(query + 2));

	if (reg_addr >= num_ao) 
	{
		if(max_rspsize < 5)
			return 0;
	 	*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x86;
		*ptr_rsp++ = 0x02;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	mw_ao[reg_addr] = swap_uint16(*(unsigned short int*)(query + 4));
	if (max_rspsize < 8)
		return 0;
	memcpy(rsp, query, 8);
	return 8;
}

//************************************************************
//函数功能：写多个线圈
//************************************************************

static int slave_deal_0fH(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	*ptr_query;
	unsigned char	*ptr_rsp;
	unsigned char	*ptr_mw_do;
	unsigned char	 bit_addr;
	unsigned char	 byte_count;
	unsigned int	 starting_addr;
	unsigned int	 coil_num;
	
	ptr_rsp = rsp;
	if (querysize < 9)
		return 0;
	starting_addr = swap_uint16(*(unsigned short int*)(query + 2));
	coil_num = swap_uint16(*(unsigned short int*)(query + 4));
	byte_count = *(query + 6);
	if (querysize != byte_count + 9)
		return 0;
	if (coil_num < 1 || coil_num > 2000 || byte_count != (unsigned char)((coil_num + 7) >> 3)) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x8f;
		*ptr_rsp++ = 0x03;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	if (num_do == 0 || starting_addr > num_do - 1 || coil_num > num_do - starting_addr) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x8f;
		*ptr_rsp++ = 0x02;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	ptr_mw_do = mw_do + starting_addr;
	ptr_query = query + 7;
	bit_addr = 0x01;
	while (coil_num > 0) 
	{
		if ((*ptr_query & bit_addr) == 0)
			*ptr_mw_do = 0x00;
		else
			*ptr_mw_do = 0x01;
		ptr_mw_do++;
		bit_addr <<=1;
		if (bit_addr == 0) 
		{
			ptr_query++;
			bit_addr = 0x01;
		}
		coil_num--;
	}
	if (max_rspsize < 8)
		return 0;
	memcpy(rsp, query, 6);
	*(unsigned short int*)(rsp + 6) = calc_CRC16(rsp, 6);
	return 8;
}

//************************************************************
//函数功能： 对主机命令错误处理
//************************************************************

static short int slave_deal_10H(unsigned char *query, short int querysize, unsigned char *rsp, short int max_rspsize)
{
	unsigned char	 i;
	unsigned char	 n;
	unsigned char	*ptr_query;
	unsigned char	*ptr_rsp;
	unsigned char	 nbyte;
	unsigned short int	*ptr_mw_ao;
	unsigned short int	 starting_addr;
	unsigned short int	 num_reg;
	
	ptr_rsp = rsp;
	
	if (querysize < 9)
		return 0;
		
	starting_addr = swap_uint16(*(unsigned short int*)(query + 2));
	num_reg = swap_uint16(*(unsigned short int*)(query + 4));
	nbyte = *(query + 6);
	
	if (num_reg == 0 || num_reg > 123 || nbyte != *(unsigned char*)&num_reg << 1) 
	{
		if (max_rspsize < 5)
			return 0;
		
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x90;
		*ptr_rsp++ = 0x03;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	
	if (querysize != (int)((num_reg << 1) + 9))
		return 0;
	
	if (starting_addr >= num_ao || num_reg > num_ao - starting_addr) 
	{
		if (max_rspsize < 5)
			return 0;
		*ptr_rsp++ = unit_id;
		*ptr_rsp++ = 0x90;
		*ptr_rsp++ = 0x02;
		*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
		return 5;
	}
	
	ptr_query = query + 7;
	ptr_mw_ao = &mw_ao[starting_addr];
	
	n = *(unsigned char*)&num_reg;
	for (i = 0; i < n; i++) 
	{
		*ptr_mw_ao = swap_uint16(*(unsigned short int*)ptr_query);
		ptr_mw_ao++;
		ptr_query +=sizeof(unsigned short int);
	}
	
	if (max_rspsize < 8)
		return 0;
	
	memcpy(rsp, query, 6);
	*(unsigned short int*)(ptr_rsp + 6) = calc_CRC16(rsp, 6);
	return 8;
}

//************************************************************
//函数功能： 对主机命令错误处理
//************************************************************

static int slave_deal_unsup(unsigned char *query, int querysize, unsigned char *rsp, int max_rspsize)
{
	unsigned char	*ptr_rsp;

	ptr_rsp = rsp;
	if(max_rspsize < 5)
		return 0;
	*ptr_rsp++ = unit_id;
	*ptr_rsp++ = *(query + 1) | 0x80;
	*ptr_rsp++ = 0x01;
	*(unsigned short int*)ptr_rsp = calc_CRC16(rsp, 3);
	return 5;
}

//**********************************************************************
//* 函数功能：主机命令处理
//**********************************************************************

short int mbrtu_slave(unsigned char *query, short int querysize, unsigned char *rsp,short int max_rspsize)
{

	if (*query != unit_id || querysize < 4)
		return 0;
	if (calc_CRC16(query, querysize - 2) != *(unsigned short int*)(query + querysize - 2))
		return 0;
	switch (*(query + 1)) 
	{
		case 0x01:
			return slave_deal_01H(query, querysize, rsp, max_rspsize);
		case 0x02:
			return slave_deal_02H(query, querysize, rsp, max_rspsize);
		case 0x03:
			return slave_deal_03H(query, querysize, rsp, max_rspsize);
		case 0x04:
			return slave_deal_04H(query, querysize, rsp, max_rspsize);
		case 0x05:
			return slave_deal_05H(query, querysize, rsp, max_rspsize);
		case 0x06:
			return slave_deal_06H(query, querysize, rsp, max_rspsize);
		case 0x0f:
			return slave_deal_0fH(query, querysize, rsp, max_rspsize);
		case 0x10:
			return slave_deal_10H(query, querysize, rsp, max_rspsize);
		default:
			return slave_deal_unsup(query, querysize, rsp, max_rspsize);
	}
}

//**********************************************************************
//* 函数功能：
//**********************************************************************

static short int master_build_03H(struct mbrtu_block *block, unsigned char *pkt, short int max_pktsize)
{
	if(max_pktsize < 8) 
		return -1;

	*pkt = block->slave_addr;
	*(pkt + 1) = 0x03;
	*(unsigned short int*)(pkt + 2) = swap_uint16(block->starting_addr);
	*(unsigned short int*)(pkt + 4) = swap_uint16(block->num_obj);
	*(unsigned short int*)(pkt + 6) = calc_CRC16(pkt, 6);

	return 8;
}

//**********************************************************************
//* 函数功能：	   主机命令10H
//**********************************************************************

static short int master_build_10H(struct mbrtu_block *block, unsigned char *pkt, short int max_pktsize)
{
	unsigned char	 i;
	unsigned char	 n;
	unsigned char	*ptr_pkt;
	unsigned short int	*ptr_table;
	
	ptr_pkt = pkt;
	ptr_table = (unsigned short int*)block->table;

	if(block->num_obj == 0 || block->num_obj > 123) 
		return -1;
	
	if(max_pktsize < (short int)((block->num_obj << 1) + 9)) 
		return 0;

	*ptr_pkt++ = block->slave_addr;
	*ptr_pkt++ = 0x10;
	*(unsigned short int*)ptr_pkt = swap_uint16(block->starting_addr);
	ptr_pkt +=sizeof(short int);
	*(unsigned int*)ptr_pkt = swap_uint16(block->num_obj);
	ptr_pkt +=sizeof(short int);
	*ptr_pkt++ = (unsigned char)(block->num_obj << 1);
	
	n = *(unsigned char*)&block->num_obj;
	for (i = 0; i < n; i++) 
	{
		*(unsigned short int*)ptr_pkt = swap_uint16(*ptr_table);
		ptr_pkt +=sizeof(short int);
		ptr_table++;
	}
	
	*(unsigned short int*)ptr_pkt = calc_CRC16(pkt, (block->num_obj << 1) + 7);
	return (short int)((block->num_obj << 1) + 9);
}


//**********************************************************************
//* 函数功能：	 多保持寄存器
//**********************************************************************

static void master_deal_03H(struct mbrtu_block *block, unsigned char *rsp, short int rspsize)
{
	unsigned char	 	i;
	unsigned char		*p_rsp;
	unsigned short int	*p_table;
	
	p_table = (unsigned short int*)block->table;
	
	if (*rsp != block->slave_addr) 
	{
		bitset(*block->status,2);
		return;
	 }
	if (*(rsp + 1) == 0x83) 
	{
		bitset(*block->status, 2);
		return;
	}
	if (*(rsp + 1) != 0x03) 
	{
		bitset(*block->status, 2);
		return;
	}
	if (rspsize != (int)((block->num_obj << 1) + 5)) 
	{
		bitset(*block->status, 2);
		return;
	}
	if (*(rsp + 2) != (unsigned char)(block->num_obj << 1)) 
	{
		bitset(*block->status, 2);
		return;
	}
	p_rsp = rsp + 3;	
	for (i = 0; i < (unsigned char)block->num_obj; i++) 
	{
		*p_table = swap_uint16(*(unsigned short int*)p_rsp);
		p_table++;
		p_rsp += 2;
	}
}

//**********************************************************************
//* 函数功能：	 写单个保持寄存器-处理从机数据帧
//**********************************************************************



//**********************************************************************
//* 函数功能：  写多保持寄存器
//**********************************************************************

static void master_deal_10H(struct mbrtu_block *block, unsigned char *rsp, short int rspsize)
{
	if (rspsize != 8) 
	{
		bitset(*block->status, 2);
		return;
	}
	
	if (*rsp != block->slave_addr) 
	{
		bitset(*block->status, 2);
		return;
	}
	
	if (*(rsp + 1) == 0x90) 
	{
		bitset(*block->status, 2);
		return;
	}
	
	if (*(rsp + 1) != 0x10) 
	{
		bitset(*block->status, 2);
		return;
	}
	
	if (block->starting_addr != swap_uint16(*(unsigned short int*)(rsp + 2))) 
	{
		bitset(*block->status, 2);
		return;
	}
	
	if (block->num_obj != swap_uint16(*(unsigned short int*)(rsp + 4))) 
	{
		bitset(*block->status, 2);
		return;
	}
}






//**********************************************************************
//* 函数功能：
//**********************************************************************

short int mbrtu_master_build(struct mbrtu_block *block, unsigned char *pkt, short int max_pktsize)
{
	bitclr(*block->status, 1);

	switch (block->type) 
	{
		case '0':
			//if(block->mode == 'r')
				//return master_build_01H(block,pkt,max_pktsize);
			  //if(block->mode = 'w')
			  	// return master_build_0FH(block,pkt,max_pktsize);
			//bitset(*block->status,1);
			return 0;
		case '1':
			//if(block->mode == 'r')
				//return master_build_02H(block,pkt,max_pktsize);
			//bitset(*block->status,1);
			return 0;
		case '3':
			//if(block->mode == 'r')
				//return master_build_04H(block,pkt,max_pktsize);
			//bitset(*block->status,1);
			return 0;
		case '4':
			if (block->mode == 'r')
				return master_build_03H(block, pkt, max_pktsize);
			if (block->mode == 'w')
				return master_build_10H(block, pkt, max_pktsize);
			bitset(*block->status, 1);
			return 0;
		default:
			bitset(*block->status, 1);
			return 0;
	}
}

//**********************************************************************
//* 函数功能：
//**********************************************************************

void mbrtu_master_deal(struct mbrtu_block *block, unsigned char *rsp, short int rspsize)
{
	unsigned short int CRC16;
	
	bitclr(*block->status, 0);
	bitclr(*block->status, 2);
	if (rspsize == 0) 
	{
		bitset(*block->status, 0);
		return;
	}
	if (rspsize < 4) 
	{
		bitset(*block->status, 2);
		return;
	}
	CRC16 = calc_CRC16(rsp, rspsize - 2);
	if (CRC16 != *(unsigned short int*)(rsp + rspsize - 2)) 
	{
		bitset(*block->status, 2);
		return;
	}
	switch (block->type) 
	{
		case '0':
			if(block->mode == 'r')
			{
				return;
			}
			if(block->mode == 'w')
			{
				return;
			}
			return;
		case '1':
			if(block->mode == 'r')
			{
				return;
			}
			if(block->mode == 'w')
			{
				return;
			}
			return;
		case '2':
			if(block->mode == 'r')
			{
				return;
			}
			if(block->mode == 'w')
			{
				return;
			}
			return;
		case '3':
			if(block->mode == 'r')
			{
				return;
			}
			if(block->mode == 'w')
			{
				return;
			}
			return;
		case '4':
			if(block->mode == 'r') 
			{
				master_deal_03H(block, rsp, rspsize);
				return;
			}
			if(block->mode == 'w') 
			{
				master_deal_10H(block, rsp, rspsize);
				return;
			}
			bitset(*block->status, 2);
			return;
		default:
			bitset(*block->status, 2);
			return;
	}
}
