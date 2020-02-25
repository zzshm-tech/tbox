


#ifndef _FR_DRV_I2C_H
#define _FR_DRV_I2C_H

void fr_init_i2c(void);
signed char fr_i2c_read(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short data_len);
unsigned char fr_i2c_write(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short int data_len);



#endif



