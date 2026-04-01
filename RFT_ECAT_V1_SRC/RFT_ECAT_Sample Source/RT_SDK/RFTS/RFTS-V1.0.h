/*
	V1.0 - 2020.02.23
		- RFTS 인터페이스 통합
		- 1차로 이더켓 인터페이스 적용
		- SOEM 기반의 범용 EtherCAT Master 라이브러리를 코딩하여 적용
		- 통신 패킷 처리를 위한 'RFT_IF_PACKET_Rev1.2' 라이브러리를 통합
*/

#ifndef __RFTS_H__
#define __RFTS_H__

#include <vector>
#include <mutex>
using namespace std;

#include <stdio.h>
#include <string.h>

// Physic Defines
#include "RT_Physics_Definitions.h"
#include "RT_Buffer.h"


enum class RFTS_COMMAND:uint8_t{
	NONE = 0,
	GET_PRODUCT_NAME,
	GET_SERIAL_NUMBER,
	GET_FIRMWARE_VER,
	SET_CAN_ID,
	GET_CAN_ID,
	SET_UART_BAUDRATE,
	GET_UART_BAUDRATE,
	SET_FT_FILTER,			
	GET_FT_FILTER,
	FT_ONCE,
	FT_CONT,
	FT_CONT_STOP,
	RESERVED_1,
	RESERVED_2,
	SET_OUTPUT_RATE,
	GET_OUTPUT_RATE,
	SET_BIAS,
	GET_OVERLOAD_COUNT,			// 18

	// For Vendor
	SET_PRODUCT_NAME = 101,
	SET_SERIAL_NUMBER,
	CAP_CONT,
	CAP_CONT_STOP,
	SET_CAL_DATA,
	SET_TEMP_COMP_DATA = 109,	//109
	GET_TEMP_COMP_DATA,
	SET_FACTORY_BIAS,
	CLEAR_OVERLOAD_CNT,
	SET_SCALE_FACTOR			// 113
};


// For Setting Low Pass Filter
enum class RFTS_LPF_CUTOFF:uint8_t{
	None = 0,	
	_500Hz,		
	_300Hz,
	_200Hz,
	_150Hz,
	_100Hz,
	_50Hz,
	_40Hz,
	_30Hz,
	_20Hz,
	_10Hz,
	_5Hz,
	_3Hz,
	_2Hz,
	_1Hz,
	END
};
const string RFTS_LPF_cutoff_str[] = { 
	"No Filter",
	"500Hz", 
	"300Hz", 
	"200Hz", 
	"150Hz", 
	"100Hz", 
	"50Hz", 
	"40Hz", 
	"30Hz", 
	"20Hz", 
	"10Hz", 
	"5Hz", 
	"3Hz", 
	"2Hz", 
	"1Hz" };
const uint16_t RFTS_LPF_cutoff_value[] = { 
	0, 500, 300, 200, 150, 100, 50, 40, 30, 20, 10, 5, 3, 2, 1 };

// For Setting Output Data Rate
enum class RFTS_OUTPUT_RATE :uint8_t{
	_200Hz_Default = 0, 
	_10Hz,
	_20Hz,
	_50Hz,
	_100Hz,
	_200Hz,
	_333Hz,
	_500Hz,
	_1000Hz,	// 8
	END
};
const string RFTS_output_rate_str[] = {
	"200Hz(Default)", "10Hz", "20Hz", "50Hz", "100Hz", "200Hz", "333Hz", "500Hz", "1000Hz"};
const int RFTS_output_rate_value[] = { 
	200, 10, 20, 50, 100, 200, 333, 500, 1000 };

typedef union {
	uint8_t byte;
	struct{
		uint8_t  Fx : 1;
		uint8_t  Fy : 1;
		uint8_t  Fz : 1;
		uint8_t  Tx : 1;
		uint8_t  Ty : 1;
		uint8_t  Tz : 1;
	};
}RFTS_OVERLOAD_STATE;

typedef union {
	uint8_t byte[6];
	struct{
		uint8_t  Fx;
		uint8_t  Fy;
		uint8_t  Fz;
		uint8_t  Tx;
		uint8_t  Ty;
		uint8_t  Tz;
	};
}RFTS_OVERLOAD_COUNTER;

typedef struct{
	uint8_t rx;
	uint8_t tx1;
	uint8_t tx2;
}RFTS_CAN_ID;

typedef struct{
	FT_double FT;
	double temperature;
	RFTS_OVERLOAD_STATE OL_state;
}RFTS_RX_FT_DATA;

typedef struct{
	double Cap[8];
}RFTS_RX_CAP_DATA;


enum class RFTS_BAUD_RATE {
	_115200_BPS_Default,
	_921600_BPS,
	_460800_BPS,
	_230400_BPS,
	_115200_BPS,
	_57600_BPS,
	END
};
const string RFTS_baud_rate_str[] = { 
	"115.2kbps", "921.6kbps", "460.8kbps", "230.4kbps", "115.2kbps", "57.6kbps" };
const int RFTS_baud_rate_value[] = { 
	115200, 921600, 460800, 230400, 115200, 57600 };

// String data size
#define PRODUCT_NAME_LENGTH			(15) 
#define SERIAL_NUMBER_LENGTH		(15)
#define FIRMWARE_VER_LENGTH			(15)

#define MUTEX_LOCK_GUARD(mtx)  std::lock_guard<std::mutex> guard(mtx)

class RFTS 
{
public:
	enum PROTOCOL{ // 어떤 통신 프로토콜을 사용할 것인가에 대한 정의
		CAN = 1,
		UART,
		RS485,
		EC02,
		ECAT_V1
	};
	enum RTX_TYPE{ // 통신에 사용되는 패킷의 형태 정의 
		PACKET,
		ECAT_PDO
	};
	enum OP_MODE{
		IDLE,
		FT_CONT,
		CAP_CONT,
		NunOfOpMode
	};

	// FT 값 수신된 후 변환이 마쳐지면 수행되는 콜백
	typedef void(*FT_RCVD_CALLBACK_FUNC) (void *object_callbacked, double *ft);
	// CAP 값 수신된 후 변환이 마쳐지면 수행되는 콜백
	typedef void(*CAP_RCVD_CALLBACK_FUNC) (void *object_callbacked, double *cap);

	// sensor info
	PROTOCOL protocol; 
	RTX_TYPE rtx_type;

	string product_name;
	string serial_number;
	string firmware_version;
	double force_divider;
	double torque_divider;
	FT_double maxFT;

	// current operating mode 
	OP_MODE op_mode;

	// Error Message
	string error_msg;

private:
	FT_double FT;							// mesaured force/torque values
	RFTS_OVERLOAD_STATE overload_state;		
	double temperature;						// sensor temperature
	RT_CircularBuffer<RFTS_RX_FT_DATA> FT_buffer;

	// user setup
	bool biased;							// bias state
	uint16_t LPF_cutoff;
	uint16_t output_rate;
	RFTS_OVERLOAD_COUNTER overload_counter;

	// communicaiton info and data
	RFTS_COMMAND command;
	uint8_t cmd_parameter[3]; // EC02 only

	// vendor variables
	double Cap[8];
	RT_CircularBuffer<RFTS_RX_CAP_DATA> Cap_buffer;

	float **CalData;

	// callbacks on receive
	FT_RCVD_CALLBACK_FUNC callback_on_FT_received = NULL;
	void *callbacked_object_on_FT_received = NULL;
	CAP_RCVD_CALLBACK_FUNC callback_on_Cap_received = NULL;
	void *callbacked_object_on_Cap_received = NULL;
	
	RFTS_CAN_ID can_id;
	RFTS_CAN_ID can_id_next_boot;

	RFTS_BAUD_RATE uart_baudrate;
	RFTS_BAUD_RATE uart_baudrate_next_boot;

	uint8_t RS485_id;

	// slave id of rfts
	int slave_id;				// for ethercat

	uint8_t response[24]; 		// response : received data for can, uart interface and ec02
	uint8_t response_result;
	uint8_t response_error_code;

	mutex  *mutex_ft_data;		 // mutex can not be copied or deleted  --> pointer 
	mutex  *mutex_command_send;  
	mutex  *mutex_response;		  

public:
	RFTS();
	~RFTS();

	void get_FT_values(float *ft);
	void get_FT_values(double *ft);
	void get_FT_values(FT_double& FT_measured);
	void get_FT_values(FT_float& FT_measured);
	void get_overload_state(RFTS_OVERLOAD_STATE& s);
	void get_temperaure(float& t);
	void get_temperaure(double& t);

	bool update_product_name(void);
	bool update_serial_number(void);
	bool update_firmware_version(void);
	bool update_sensor_info(void);
	bool start_FT_out(void);
	bool stop_FT_out(void);
	bool set_bias(bool on = true);
	bool set_filter(RFTS_LPF_CUTOFF para);
	bool update_filter_data(void);
	bool get_filter(uint16_t& co_freq);
	bool set_output_rate(RFTS_OUTPUT_RATE para);
	bool update_output_rate_data(void);
	bool get_output_rate(uint16_t& out_rate);
	bool update_overload_counter(void);
	bool get_overload_counter(RFTS_OVERLOAD_COUNTER& oc);

	RT_CircularBuffer<RFTS_RX_FT_DATA> *get_FT_buffer();
	void set_callback_on_FT_receive(FT_RCVD_CALLBACK_FUNC cb_func, void *callbacked_object);

	// for vendor
	bool activate_vendor_mode(void);
	void get_Cap_values(double *cap);
	bool set_product_name(string& pn);
	bool set_serial_number(string& sn);
	bool start_Cap_out(void);
	bool stop_Cap_out(void);
	bool download_cal_data(float *caldata);
	bool set_temperature_comp_data(short *comp_data);
	bool set_factory_bias(bool on);
	bool clear_overload_counter(void);
	bool set_hysteresis_comp_rate(float h_rate);
	bool get_hysteresis_comp_rate(float& h_rate);

	RT_CircularBuffer<RFTS_RX_CAP_DATA> *get_Cap_buffer();
	void set_callback_on_Cap_receive(CAP_RCVD_CALLBACK_FUNC cb_func, void *callbacked_object);


// for H/W interface -------------------------------------------------------------------------

	bool set_can_id(uint8_t rx_id, uint8_t tx1_id, uint8_t tx2_id);
	bool update_can_id(void);
	bool get_can_id(RFTS_CAN_ID& id_current, RFTS_CAN_ID& id_next_boot);
	//bool search_can_id(RFTS_CAN_ID& id_current, RFTS_CAN_ID& id_next_boot, bool auto_update = true);

	bool set_uart_baudrate(RFTS_BAUD_RATE br);
	bool update_uart_baudrate(void);
	bool get_uart_baudrate(RFTS_BAUD_RATE& br_current, RFTS_BAUD_RATE& br_next_boot);

	bool set_slave_id(int slv_id);

	// add interface to this RFTS object
	bool add_uart_interface(void *rfts_if, PROTOCOL p = PROTOCOL::UART);
	bool add_can_interface(void *rfts_if, uint8_t tx_id, uint8_t rx_id1, uint8_t rx_id2);
	bool add_ecat_interface(void *rfts_if, int slv_id);

	// check rfts is connected
	bool is_connected();
	// clear the joined interface
	void clear_interface();

private:
	// Connected Interface
	void *m_RFTS_IF;
	bool connected_to_interface = false;

	// command 보낸다. - ecat 버전(EC02)일 경우 생성만 한다.
	bool send_command(uint8_t* cmd_pkt, uint8_t *ext_pkt = NULL, int ext_pkt_len = 0);
	bool wait_resonse();
	void clear_response_data(void);


	// callbacks for can interface
	static void callback_can_rx(void *me, uint8_t *msg, uint32_t id);

	// callbacks for uart interface
	static void callback_uart_rx(void *me, DWORD bytes_received);

	// callbacks for ecat interface
	static void callback_ecat_rx(void *me);
	static void callback_ecat_tx(void *me);

	// ecat functions for SDO
	bool read_ecat_SDO(uint16_t slave_id, uint16_t idx, uint8_t sub_idx, void *data);
	bool write_ecat_SDO(uint16_t slave_id, uint16_t idx, uint8_t sub_idx, int p_size, void *data);
};

#include "RT_CommonUtility.h"
#ifdef RFTS_USE_CAN
#include "CAN_Interface.h"
#endif

#ifdef RFTS_USE_UART
#include "UART_Interface.h"
#endif

#ifdef RFTS_USE_ECAT
#include "ECAT_Master.h"
#endif

class RFTS_SET
{
public:
	RFTS_SET();
	~RFTS_SET();

	void clear();
	int size();
	RFTS* at(int idx);

#ifdef RFTS_USE_ECAT
	// attach the ecat interface object to all of the connected ecat RFTSs, 
	void attach_all_rfts_on_ecat(ECAT_Master& ecat_if);
#endif
	// update sensor information.
	// Note: interface for RFTS should be enabled.
	void update_info();

private:
	vector <RFTS*> rfts_stack;
};

#endif // END OF FILE
