
#ifndef __MBRTU_H
#define __MBRTU_H

#define MBRTU_DEV_ADDR			 0
#define MBRTU_MEMORY_MW			 1






struct mbrtu_block 
{					
	unsigned char	 slave_addr;			
	unsigned char	 mode;				
	unsigned char	 type;				
	unsigned short int	 starting_addr;			
	unsigned short int	 num_obj;			
	void		*table;				
	unsigned short int	*status;			
};



struct com_task 
{
	unsigned char	 flag;
	void *block;	
};


void init_mbrtu_lib(unsigned char *arg_mw_do, unsigned short int arg_num_do,
               unsigned char *arg_mw_di, unsigned short int arg_num_di,
               unsigned short int *arg_mw_ao, unsigned short int arg_num_ao,
               unsigned short int *arg_mw_ai, unsigned short int arg_num_ai,
			   unsigned char arg_unit_id);



short int	 mbrtu_slave(unsigned char *query, short int querysize, unsigned char *rsp, short int max_rspsize);
short int	 mbrtu_master_build(struct mbrtu_block *block, unsigned char *pkt, short int max_pktsize);
void	 mbrtu_master_deal(struct mbrtu_block *block, unsigned char *rsp, short int rspsize);

#endif




