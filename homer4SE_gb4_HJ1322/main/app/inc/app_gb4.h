



#ifndef _APP_GB4_H
#define _APP_GB4_H



#define SOFEWARE_VER    10  //软件版本号
#define TERIMNAL_TYPE ((rt_uint8_t)1)   //不同数值代表不同型号，如0x01表示HOMER3X 0x02表示HOMER3E

/* GB */
#define SM2   		    0X01

#define LOGIN   		0x01
#define LOGOUT  		0x04
#define REALDATA   		0X02
#define BLINDDATA  		0X03
#define NTP             0X05
#define RECORD   		0X07
#define RECORDRES   	0X08
#define HEART   		0X09
/* 命令单元 */
#define PARQUERY        0X80
#define PARSET          0X81
#define DEVCONTROL      0X82
/* 信息类型 */
#define UPDATE          0X01



#pragma pack(1)


/************************
**  下行控制命令
*************************/

struct down_cmd_con_str
{
	uint8_t 		time[6];		//
	uint32_t  		seriarl[2];     //流水号
	uint8_t 		cmd;			//
	uint16_t  		len;			//
};


struct serial_num_str
{
	uint32_t 		verfy;				 //
	uint32_t 		login_num;    		 //登入登出流水号
	uint32_t 		serial_num;   		 //企标流水号
	uint32_t 		serial_gb;    		 //国标流水号
	uint32_t 		unix_time; 	 		 //
	uint32_t	 	day;			 	 //更改
	uint32_t 		alarm_num;    		 //报警流水号
	uint32_t 		qb_login_num; 		 //登录流水号
};





struct encryption_chip_str
{
	uint8_t 	id_sn[16];            //来自加密芯片的128bit序列号（全球唯一）
	uint8_t 	id_lot[16];       	  //自定义加密芯片ID
	uint8_t 	public_key[64];    	  //公钥
	uint8_t 	state;                //
	uint32_t 	version;              //
};



struct gb4_str
{
	uint8_t 	socket_state;				//
	uint8_t 	login_out_state;		    //0:登出；1：登入
	uint8_t 	qb_step;
	uint8_t 	gb_step;
	uint32_t 	qb_cnt;
	uint32_t 	gb_cnt;
};



/*****************************
**	消息头
******************************/

struct start_str
{
	uint8_t 			head[2]; 			//帧头
	uint8_t 		  	cmd;      			//指令类型
	uint8_t 			vin[17];    		//机械环保代码
	uint8_t 			soft_ver;      		//软件版本
	uint8_t 			encrypt;        	//加密方式
	uint16_t  	  		len;    			//数据长度
};




/*****************************
**	企标登录消息体
******************************/
struct login_qb_str
{
	uint8_t 		vin[17];             //
	uint8_t 		dev_id[17];			 //
	uint16_t 		type;                //
	uint8_t 		icc_id[20];          //
	uint8_t 		imei[17];            //
};



struct serial_qb_str
{
	uint8_t time[6];
	uint32_t num[2];
};




struct obd_fc_t
{	
	uint8_t 				type;  	        //信息标识
	uint8_t 				rtc[6]; 			//采集时间
	uint8_t					protocol;		//OBD协议
	uint8_t					alarm_state;	//报警状态
	uint8_t 				num;
};




struct gb27145_dm_msg_t
{	
	uint8_t 				type;  	        //信息标识
	uint8_t					protocol;		//OBD协议
	uint8_t					alarm_state;	//报警状态
	uint8_t 				num;
};




struct down_cmd_res_str
{
	uint32_t 	ser_num[2];
	uint8_t 	res;
	uint16_t 	cmd_id;
	uint8_t 	msg_type;
};

/*****************************
**	国标登录登出消息体
******************************/

struct gb_login_out_str
{
	uint8_t 		time[6];   		// 数据采集时间 
  	uint16_t 	    serial_num; 	// 登入流水号 
	uint8_t		    iccid[20];  	// ICCID
};




/*****************************
**	位置消息体
******************************/
struct gnss_msg_str
{
	uint8_t      	status;          //GNSS定位状态
	uint32_t        longitude;       //经度
	uint32_t       	latitude;        //纬度      	
};



/****************************
**	国四排放数据
*****************************/

struct gb4_data_msg_str
{
	uint8_t 			type;  	    //信息标识
	
	uint16_t 		    speed;			//车速
	uint8_t			    air_pressure;		//大气压力
	uint8_t 		 	engine_torque;		//发动机实际扭矩
	uint8_t            	friction_torque;		//摩擦扭矩
	uint16_t            engine_rotate; 		//发动机转速
	uint16_t            fuel_flow;		//发动机燃料流量
	uint16_t            scr_up_nox;		//SCR上游NOx传感器输出值
	uint16_t            scr_down_nox;	//SCR下游NOx传感器输出值
	uint8_t             reactant_allowance;  //反应剂余量
	uint16_t			enter_volume;		//进气量
	uint16_t			scr_in_temp;    //SCR入口温度
	uint16_t			scr_out_temp;	//SCR出口温度
	uint16_t			dpf_diffPressure;		//DPF压差
	uint8_t				cooling_temp;	//冷却液温度
	uint8_t             fuel_position;		//油箱液位
	uint16_t			egr_opening;			//EGR阀开度
	uint16_t			egr_setting;			//EGR设定值
};





/****************************
**	国四排放数据
*****************************/

struct dpf_scr_real_t
{
	uint8_t 			type;  	    //信息标识
	
	uint8_t 			rtc[6]; 			//采集时间

	uint16_t 		    speed;			//车速
	uint8_t			    air_pressure;		//大气压力
	uint8_t 		 	engine_torque;		//发动机实际扭矩
	uint8_t            	friction_torque;		//摩擦扭矩
	uint16_t            engine_rotate; 		//发动机转速
	uint16_t            fuel_flow;		//发动机燃料流量
	uint16_t            scr_up_nox;		//SCR上游NOx传感器输出值
	uint16_t            scr_down_nox;	//SCR下游NOx传感器输出值
	uint8_t             reactant_allowance;  //反应剂余量
	uint16_t			enter_volume;		//进气量
	uint16_t			scr_in_temp;    //SCR入口温度
	uint16_t			scr_out_temp;	//SCR出口温度
	uint16_t			dpf_diffPressure;		//DPF压差
	uint8_t				cooling_temp;	//冷却液温度
	uint8_t             fuel_position;		//油箱液位
	uint16_t			egr_opening;			//EGR阀开度
	uint16_t			egr_setting;			//EGR设定值
};



/************************************************
**	DPF/SCR周期内数据项统计值的数据格式及定义
*************************************************/

struct dpf_scr_average_t
{
	uint8_t 			type;  	    		//信息标识
	
	uint8_t 			rtc[6]; 			//采集时间

	uint16_t 			ref_torque;			// 参考扭矩
	uint16_t			engine_power;		// 发动机平均功率
	uint16_t			scr_up_nox;			// SCR上游NOx平均浓度
	uint16_t			scr_down_nox;		// SCR下游NOx平均浓度
	uint8_t 			scr_up_flow;		// SCR上游NOx平均质量流量
	uint8_t				scr_down_flow;     	// SCR下游NOx平均质量流量
	uint16_t			src_in_temp;		// SCR入口平均温度
	uint16_t			src_out_temp;  		// SCR出口平均温度
	uint16_t			fuel_flow;			// 发动机燃料流量平均值
	uint16_t			cycle_calculate;    // 统计周期时长
	uint8_t				cycle_pwm; 		  	// 统计周期内有效时间占比
};





/*************************
**	工程机械国四扩展数据  用户自定义
**	长度 
**	消息类型：0x80
**************************/

struct terminal_msg_str
{
	uint8_t 	 			msg_type;               //消息体类型
	uint16_t 				msg_len;                //消息体长度
	
	uint8_t 				vin[17];           			//VIN号 （）
	uint8_t 				id[17];            			//设备编号
	uint8_t      			manu_num;          			//厂家编码
	uint8_t 				terminal_type;     			//终端型号
	uint8_t 	   			user_num;          			//使用方编号
	uint8_t      			car_type;     		 			//安装车型 
	uint8_t 				app_ver1;          			//终端上传协议版本
	uint8_t      			app_ver2;          			//应用程序版本号    
	uint8_t      			hd_ver;        		 			//硬件版本号
	uint8_t      			io_status;							//IO状态
	uint8_t 				acc_status;     				//ACC状态
	uint8_t 				moto_status;						//MOTO状态
	uint16_t 				input_frq1;							//外部输入频率
	uint16_t 				input_frq2;							//外输输入频率
	uint16_t 				output_frq3;						//PWM1输出频率
	uint16_t 				output_frq4;						//PWM2 输出频率
	uint16_t 				input_vol1;							//外部输入电压1
	uint16_t 				input_vol2;							//外部输入电压2
	uint16_t 				power_vol;							//外部输入电压
	uint16_t 				batter_vol;							//内部电池输入电压
	uint32_t				warn_value;							//设备报警值
	uint16_t 				gnss_speed;   					//GNSS 速度
	uint16_t 				gnss_heading;   				//GNSS 方向
	uint16_t 				gnss_altitude;          //GNSS 海拔高度
	uint8_t 				gnss_used_satellite;    //GNSS 使用卫星数
	uint8_t 				gnss_view_satellite;    //GNSS 可视卫星数
	uint16_t 				gnss_hdop;             	//GNSS 水平经度因子
	uint8_t 				gnss_mondel_status;     //GNSS 模块状态
	uint8_t 				nj_mon_state;  										//1：关闭三合一功能，2：开启三合一功能  预留
	uint8_t 				nj_socket_state;    								//0：三和一断开；1：已经链接Token服务器；2：已链接IP服务器；3：已经链接数据服务器；4：轨迹同步成功。
	uint8_t         		data_model;   					//0:只传单包模式；1：单包和多包模式；
	uint8_t 				csq;       							//4G网络信号值
	uint8_t 				arch_state; 						//防篡改备案状态
	uint8_t					ver_security;   //安全芯片应用程序版本号
	uint8_t 				id_security[16];
	uint32_t 				nj_send_num;
};





/*************************
**	英轩农机扩展，消息ID是：0x11
**	长度 
**************************/
struct vehicle_msg_0x11_t
{
	uint8_t 			msg_type;
	uint16_t 			msg_len;
	
	uint16_t 			accumulator_vol;          			//电源电压
	uint16_t 			reference_torque;		  			//发动机参考扭矩
	uint8_t 			cold_boot_status;     	  			//冷启动热速状态
	uint8_t     		lock_status;			  			//ECU锁车状态
	uint8_t				mon_status;   			  			//锁车功能状态
	uint8_t 			key_status;     		  			//KEY码状态
	uint8_t 			id_status;				  			//TBOX ID状态
	uint8_t 			fuel_temp;				  			//燃油温度
	uint16_t 			oil_temp;         		  			//机油温度
	uint8_t				fuel_pressure;            			//燃油压力
	uint8_t 			oil_position;			  			//机油液位
	uint8_t 			cool_position;			  			//冷却液位置
	uint8_t				engine_torque_model;	  			//发动机扭矩模式   （没有）
	uint8_t 			driver_cmd_torque_percent;			//驾驶员指令扭矩百分比                 
	uint8_t  			engine_need_torque_percent;			//发动机需求扭矩百分比 
	uint8_t 			engine_affiliated_torque_percent;	//发动机附件扭矩百分比
	uint16_t 			ave_fuel_consume_rate;				//平均燃油消耗率
	uint16_t			instant_fuel_consume_rate;			//瞬时燃油消耗率
	uint32_t 			once_fuel_consume_rate;      		//单次油耗
	uint32_t 	   		total_fuel_consume_rate;  			//累计油耗
	uint8_t 			dfp_inlet_pressure;					//颗粒捕捉器入口压力
	uint8_t 			boost_pressure;						//增压压力
	uint8_t 			entered_air_intake_temp;			//发动机进气歧管温度（）
	uint8_t				enter_pressure;           			//进气压力
	uint16_t			nc;									//排气温度  (发动机排气温度)
	uint16_t 			air_temp;							//大气温度
	uint8_t 			entered_air_temp;					//发动机进气温度
	uint8_t        		carbamide_temp;						//尿素温度
	uint8_t 		 	carbamide_position;					//尿素液位
	uint8_t 		 	carbamide_concentration;			//尿素浓度
	uint8_t 			dpf_dirt_retention;					//DPF积尘量	实际值=传送值	4453	
	uint8_t 			dpf_reproduce_indicate_light;		//DPF再生提醒灯	实际值=传送值 4418	
	uint8_t 			dpf_reproduce_active_light;			//DPF主动再生状态指示灯	实际值=传送值4419	
	uint8_t 			dpf_reproduce_forbid_light;			//DPF的再生禁止指示灯	实际值=传送值4420	
	uint8_t 			dpf_reproduce_fault_light;			//DPF再生故障指示灯	实际值=传送值
	uint8_t				oil_water_pilot;    				//油中有水指示
	uint8_t				energy_saving_state;				//能量节约状态  
	uint8_t 			auxiliary_flameout_state;			//辅助熄火状态;
	uint16_t 			absolute_oil_pressure;				//绝对机油压力
	uint16_t			relative_oil_pressure;				//相对机油压力
	uint8_t 			accelerator_percent;				//加速踏板行程值
	uint8_t				engine_load_percent;  				//发动机负荷
	uint32_t			engine_work_time;					//发动机工作时间
	uint32_t			engine_total_speed;					//发动机总转速
	uint8_t				buffer_torque;                		//缓冲区扭矩
	uint32_t 			once_travel;						//单次行驶距离
	uint32_t 			total_travel;  						//总里程 
};




/**************  国四数据包  *****************/
struct gb4_packets_t
{
    uint16_t len;
    uint16_t crc;
    uint8_t  flag;
	uint8_t	buf[251];
};



struct qb4_packets_t
{
    uint16_t len;
    uint16_t crc;
    uint8_t  flag;
	uint8_t	buf[251];
};




//针对设备报警值
struct gb4_alarm_str
{
	//struct at_mutex_str      at_mutex;
    uint32_t 				 value;
};





/********* 英轩企标故障上传 ************/

struct qb4_fc_t
{	
	uint8_t 				type;  	        //信息标识
	uint16_t				len;		//OBD协议
	uint8_t					num;	//报警状态
};
  





struct general_fc_t
{	
	uint8_t 				type;  	        //信息标识
	uint16_t				total_len;			//OBD协议
	//uint8_t					group_len;			//
};





/****************************
**	农机作业数据
*****************************/

struct farm_work_msg_t
{
	uint8_t 			type;  	    //信息标识
	uint16_t 			len;                //消息体长度
	
	uint8_t 		    flag;		//作业标志 1：正在作业，2：未作业
	uint16_t 			speed;   					//GNSS 速度
	uint16_t 			heading;   				//GNSS 方向
	uint16_t 			altitude;          //GNSS 海拔高度
	uint8_t 			use_satellite;    //GNSS 使用卫星数
	uint8_t 			view_satellite;    //GNSS 可视卫星数
	uint16_t 			hdop;             	//GNSS 水平经度因子
};




#pragma pack()




uint8_t read_encryption_chip_id(uint8_t *source,uint8_t size);
uint8_t read_acl16_work_state(void);
uint8_t read_gb4_socket_state(void);

uint16_t build_gb4_platform_data(uint8_t *buf,uint16_t size);
uint16_t build_qb4_platform_data(uint8_t *buf,uint16_t size);
uint8_t read_acl16_work_state(void);
uint8_t read_acl16_app_version(void);
uint16_t write_gb4_fifo_buff(uint8_t *data,uint16_t len);
uint16_t write_qb4_fifo_buff(uint8_t *data,uint16_t len);
void build_rtc_time(uint8_t *buf,uint16_t size);
uint16_t vehicle_dismantle_alarm(uint8_t *buf,uint16_t size);
uint32_t read_dismantle_state(void);
void delete_login_out_num(void);

void thread_entry_gb4(void *parameter);

#endif




