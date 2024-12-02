




#include "drv_i2c.h"




#define I2C_MASTER_NUM                  I2C_NUM_1       /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ              100000          /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE       0               /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE       0               /*!< I2C master doesn't need buffer */

#define I2C_WRITE_BIT                   0x00            /*!< I2C master write */
#define I2C_READ_BIT                    0x01            /*!< I2C master read */

#define I2C_ACK_CHECK_EN                0x01            /*!< I2C master will check ack from slave*/
#define I2C_ACK_CHECK_DIS               0x00            /*!< I2C master will not check ack from slave */
#define I2C_ACK_VAL                     0x00            /*!< I2C ack value */
#define I2C_NACK_VAL                    0x01            /*!< I2C nack value */



/****************************************************/

static SemaphoreHandle_t                i2c_mutex;


/*************************************
**
**************************************/

void i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t val)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if (ESP_OK != i2c_master_start(cmd))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_write_byte(cmd, (addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_write_byte(cmd, reg, I2C_ACK_CHECK_EN))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_write_byte(cmd, val, I2C_ACK_CHECK_EN))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_stop(cmd))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    i2c_cmd_link_delete(cmd);
}


/*************************************
**
**************************************/

uint8_t i2c_read_byte(uint8_t addr, uint8_t reg)
{
    uint8_t data = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if (ESP_OK != i2c_master_start(cmd))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_write_byte(cmd, (addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_write_byte(cmd, reg, I2C_ACK_CHECK_EN))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }
    // i2c_master_stop(cmd);

    if (ESP_OK != i2c_master_start(cmd))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_write_byte(cmd, (addr << 1) | I2C_READ_BIT, I2C_ACK_CHECK_EN))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_read_byte(cmd, &data, I2C_NACK_VAL))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_stop(cmd))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    if (ESP_OK != i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS))
    {
        printf("ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }

    i2c_cmd_link_delete(cmd);

    return data;
}


/*************************************
**
**************************************/

uint8_t  rt_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t val)
{
    if (pdTRUE == xSemaphoreTake(i2c_mutex, 100))
    {
        i2c_write_byte(addr, reg, val);
        xSemaphoreGive(i2c_mutex);     
        return 1;
    }

    return 0;
}


/*************************************
**
**************************************/

uint8_t rt_i2c_read_byte(uint8_t addr, uint8_t reg)
{
    if (pdTRUE == xSemaphoreTake(i2c_mutex, 100))
    {
        uint8_t byte = i2c_read_byte(addr, reg);
        xSemaphoreGive(i2c_mutex);     
        return byte;
    }    

    return 0;
}


/*****************************
** 读取数据
******************************/

void i2c_write_data(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t size)
{

    i2c_cmd_handle_t cmd = i2c_cmd_link_create(); // create a command link,command data will be added to this link and then sent at once
    i2c_master_start(cmd);                        // I2C logic has been packaged in these functions
    i2c_master_write_byte(cmd, (addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write(cmd, &reg, 1, 1);
    for (uint16_t i = 0; i < size; i++)
    {
        i2c_master_write(cmd, (data + i), 1, 1);
    }

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}




/*****************************
** 读取数据
******************************/

void i2c_read_data(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write(cmd, &reg, 1, 1);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_READ_BIT, I2C_ACK_CHECK_EN);
    if(size > 1)
    {
        i2c_master_read(cmd, buf, size - 1, 0);
    }
    i2c_master_read_byte(cmd, buf + size - 1, I2C_NACK_VAL); //the lastest byte will not give a ASK
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_1, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}



/*****************************
** 写数据
******************************/

uint8_t rt_i2c_write_data(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t size)
{
    if (pdTRUE == xSemaphoreTake(i2c_mutex, 100))
    {
        i2c_write_data(addr, reg, data, size);
        xSemaphoreGive(i2c_mutex);     
    }

    return 0;
}



/*****************************
** 读取数据
******************************/

uint8_t  rt_i2c_read_data(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t size)
{
    if (pdTRUE == xSemaphoreTake(i2c_mutex, 100))
    {
        i2c_read_data(addr, reg, buf, size);
        xSemaphoreGive(i2c_mutex); 
        return 1;    
    }   

    return 0;
}




/*****************************
** 读取数据
******************************/

void i2c_mutex_create(void)
{
    /* 创建互斥量 */
    i2c_mutex = xSemaphoreCreateMutex();
    if(i2c_mutex == NULL)
    {
        //log_printf(LOG_ERROR, "ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        return;
    }
}



/************************************
 * 初始化CAN
 ************************************/

uint8_t rt_hw_init_i2c(void)
{
    esp_err_t err;

    i2c_mutex_create();

    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = GPIO_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };

    err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK)
    {
        return 1;
    }

    err = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if(err != ESP_OK)
    {
        return 1;
    }

    return 0;
}





