


#ifndef _DDP_H
#define _DDP_H


unsigned short int build_position_packet(unsigned char *source,unsigned char big);
unsigned short int build_complete_packet(unsigned char *source,unsigned char big);
unsigned char build_general_response(unsigned char *buf,unsigned short int buf_size,unsigned short int ser_num,unsigned char res,unsigned short cmd_id);

#endif


