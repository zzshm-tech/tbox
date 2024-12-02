

#ifndef _APP_FILES_H
#define _APP_FILES_H



struct files_sys_t
{
    uint8_t     state;
    uint32_t    total_capacity;    //总容量  （MB）
    uint32_t    used_capacity;     //已经使用的用量（MB）
    uint32_t    surplus_capacity;  //可用容量
};




uint8_t  read_files_sys_state(void);
uint8_t mount_sys_files(void);
uint8_t unmount_sys_file(void);
uint8_t test_files(void);
void show_files_sys_info(void);




#endif

