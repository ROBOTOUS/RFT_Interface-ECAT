/*
	EtherCAT, SOEM Open Source Lib. 
*/

/*
	Rev0.0, 2018.02.23
		- initial creation
*/

#ifndef __RT_RFT_CAN_IF_H__
#define __RT_RFT_CAN_IF_H__

#include "RFT_IF_PACKET_Rev1.2.h"	//
#include "RT_Utilities.h"

#include <vector>
#include <mutex>
using namespace std;

#include <stdio.h>
#include <string.h>
#include <Mmsystem.h>

#include "osal.h"
#include "ethercattype.h"
#include "nicdrv.h"
#include "ethercatbase.h"
#include "ethercatmain.h"
#include "ethercatdc.h"
#include "ethercatcoe.h"
#include "ethercatfoe.h"
#include "ethercatconfig.h"
#include "ethercatprint.h"

#define EC_TIMEOUTMON (500)


#define PRODUCT_NAME_LEN	(16)
#define SERIAL_NUM_LEN		(16)
#define FW_VER_LEN			(16)

#define MAX_NUM_OF_SLAVE	(4)

#define MAX_LOGGING_NUM (5000) // 50초

// logging status
#define LS_READY 	  0		// 로깅 시작가능
#define LS_MEASURING  1		// 측정중
#define LS_SAVING	  2		// 저장중
typedef struct {
	char ProductName[PRODUCT_NAME_LEN];
	float ForceDivider;
	float TorqueDivder;
	float MaxForce[3]; // x, y, z
	float MaxTorque[3];
}RFT_SENSOR_SPEC;



////////////////////////////////////////////////////////////////////////

typedef enum{
	RFTS_IF_CAN = 1,
	RFTS_IF_UART,
	RFTS_IF_RS485,
	RFTS_IF_EC02,
	RFTS_IF_ECAT_V1
}RFTS_IF_TYPE;

typedef enum{
	RFTS_MODE_IDLE,
	RFTS_MODE_FT_CONT
}RFTS_MODE;


// For RFTS HW interface callback.... 
typedef void(*RFTS_HW_IF_CALLBACK) (void *rfts, void *rfts_if);

class RFTS {
public:
	// sensor info - fixed by vendor
	RFTS_IF_TYPE IF_type;
	string product_name;
	string serial_number;
	string firmware_version;
	float force_divider;
	float torque_divider;
	float maxFT[6];
	
	// sensor info - user defined
	double LPF_cut_off_freq;

	// communicaiton info
	uint8_t CAN_rx_id, CAN_tx_id;
	uint8_t RS485_id;
	int slave_id;				// for ethercat

	// command and parameters
	uint8_t command;
	uint8_t cmd_parameter[3];
	// response 
	uint8_t response[16];
	//
	uint8_t mode;

	// measured sensor data
	float FT[6];				// mesaured force/torque values
	float temperature;			// sensor temperature

	// overload counter
	uint8_t overload_counter[6];	

private:
	mutex  *mutex_ft_data; 
	
	// callbacks for hardware interface
	void *m_RFTS_IF;
	RFTS_HW_IF_CALLBACK cb_update_product_name;
	RFTS_HW_IF_CALLBACK cb_update_serial_number;
	RFTS_HW_IF_CALLBACK cb_update_firmware_version;
	RFTS_HW_IF_CALLBACK cb_start_FT_out;
	RFTS_HW_IF_CALLBACK cb_stop_FT_out;
	RFTS_HW_IF_CALLBACK cb_set_bias;
	RFTS_HW_IF_CALLBACK cb_reset_bias; // return to factory default


	//void (*cb_update_sensor_info)(RFTS *rfts);		// update sensor info (fixed)
	//void (*cb_update_measured_data)(RFTS *rfts);	// update overlaod (fixed)
	//void (*cb_update_overload_counter)(RFTS *rfts);	// update overlaod (fixed)

public:
	RFTS()	
	{
		force_divider = 50;
		torque_divider = 2000;

		mode = RFTS_MODE_IDLE;

		mutex_ft_data = new mutex;

		// callback initialize
		cb_update_product_name = NULL;
		cb_update_serial_number = NULL;
		cb_update_firmware_version = NULL;
		cb_start_FT_out = NULL;
		cb_stop_FT_out = NULL;
		cb_set_bias = NULL;
		cb_reset_bias = NULL;

	}
	~RFTS() 
	{
	//		delete mutex_ft_data;
	};
	// get force torque values
	void getFT_float(float *ft)
	{
		mutex_ft_data->lock();
		ft[0] = FT[0];
		ft[1] = FT[1];
		ft[2] = FT[2];
		ft[3] = FT[3];
		ft[4] = FT[4];
		ft[5] = FT[5];
		mutex_ft_data->unlock();
	}
	void getFT_double(double *ft)
	{
		mutex_ft_data->lock();
		ft[0] = (double)FT[0];
		ft[1] = (double)FT[1];
		ft[2] = (double)FT[2];
		ft[3] = (double)FT[3];
		ft[4] = (double)FT[4];
		ft[5] = (double)FT[5];
		mutex_ft_data->unlock();
	}

	// update force torque values, this function is used at hardware routine
	void updateFT(float *ft_in)
	{
		mutex_ft_data->lock();
		FT[0] = ft_in[0];
		FT[1] = ft_in[1];
		FT[2] = ft_in[2];
		FT[3] = ft_in[3];
		FT[4] = ft_in[4];
		FT[5] = ft_in[5];
		mutex_ft_data->unlock();
	}

	void update_product_name(void)
	{
		string size;

		if (cb_update_product_name != NULL)
		{
			cb_update_product_name(this, m_RFTS_IF);
			
			if (product_name.size() >= 5)
			{
				size = product_name.substr(3, 2);
				if ( ( size == "80") || (size == "64") )
				{
					torque_divider = 1000;
				}
			}
		}
	}
	void update_serial_number(void)
	{
		if (cb_update_serial_number != NULL)
			cb_update_serial_number(this, m_RFTS_IF);
	}
	void update_firmware_version(void)
	{
		if (cb_update_firmware_version != NULL)
			cb_update_firmware_version(this, m_RFTS_IF);
	}
	
	void start_FT_out(void)
	{
		if (cb_start_FT_out != NULL)
			cb_start_FT_out(this, m_RFTS_IF);
	}
	void stop_FT_out(void)
	{
		if (cb_stop_FT_out != NULL)
			cb_stop_FT_out(this, m_RFTS_IF);
	}
	void set_bias(void)
	{
		if (cb_set_bias != NULL)
			cb_set_bias(this, m_RFTS_IF);
	}
	void reset_bias(void)
	{
		if (cb_reset_bias != NULL)
			cb_reset_bias(this, m_RFTS_IF);
	}


	void update_sensor_info(void)
	{
		stop_FT_out();
		delay_ms(50);

		update_product_name();
//		update_serial_number();
//		update_firmware_version();
	}
	
	// setup callbacks
	// set pointer of Hardware Interface Instance
	void set_ptr_RFTS_IF_Instance(void *rfts_if)
	{
		m_RFTS_IF = rfts_if;
	}
	void set_cb_function_update_product_name(RFTS_HW_IF_CALLBACK cb_func)
	{
		cb_update_product_name = cb_func;
	}
	void set_cb_function_update_serial_number(RFTS_HW_IF_CALLBACK cb_func)
	{
		cb_update_serial_number = cb_func;
	}
	void set_cb_function_update_firmware_version(RFTS_HW_IF_CALLBACK cb_func)
	{
		cb_update_firmware_version = cb_func;
	}

	void set_cb_function_start_FT_out(RFTS_HW_IF_CALLBACK cb_func)
	{
		cb_start_FT_out = cb_func;
	}
	void set_cb_function_stop_FT_out(RFTS_HW_IF_CALLBACK cb_func)
	{
		cb_stop_FT_out = cb_func;
	}


	void set_cb_function_set_bias(RFTS_HW_IF_CALLBACK cb_func)
	{
		cb_set_bias = cb_func;
	}
	void set_cb_function_reset_bias(RFTS_HW_IF_CALLBACK cb_func)
	{
		cb_reset_bias = cb_func;
	}
};

// for callback.... 
typedef void(*RFT_ECAT_IF_CALLBACK) (void *);

class CRFTS_ECAT_IF
{
public:
	// constructor and destructor
	CRFTS_ECAT_IF();
	~CRFTS_ECAT_IF();

public:
	
	bool run(string NIC_Name);
	bool stop(void);

	// for thread
	HANDLE m_hEcheckThread;
	DWORD  m_dwEcheckThreadId;
	bool   m_bEcatcheckThreadFlag;
	static void ecatcheckThread(CRFTS_ECAT_IF *pThis);
	void   ecatcheckWorker(void);

	static void CALLBACK RTthread(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
	void RTthreadWorker(void);

	HANDLE m_hOpCheckThread;
	DWORD  m_dwOpCheckThreadId;
	bool   m_bOpCheckheckThreadFlag;
	static void opCheckThread(CRFTS_ECAT_IF *pThis);
	void   opCheckWorker(void);

	// for callback function
	bool m_bIsEnabled_Callback;
	RFT_ECAT_IF_CALLBACK m_pCallbackFunc;
	void *m_pCallbackParam;
	void setCallback(RFT_ECAT_IF_CALLBACK pCallbackFunc, void *callbackParam);


	// RFTS Control Command
	CRT_RFT_IF_PACKET m_RFT_IF_PACKET;					// data field & uart packet handling class

	bool FTS_update_product_name(int rfts_id);			// read product name
	bool FTS_stop_output(int rfts_id);					// stop force/torque continuous output



	bool rqst_SerialNumber(int slv_id);					// read serial number
	bool rqst_Firmwareverion(int slv_id);				// read firmware version
	bool rqst_GetID(int slv_id);						// read CAN message ID
	bool rqst_FT_Filter_Type(int slv_id);				// read filter type
	bool rqst_FT(int slv_id);							// read force/torque (once)
	bool rqst_FT_Continuous(int slv_id);				// start force/torque continuous output
	bool rqst_FT_Cont_Interval(int slv_id);				// read force/torque output frq.
	bool rqst_FT_OverloadCnt(int slv_id);				// read overload count

	//bool set_ID(void);	// set CAN message ID - not supported for ECAT
	bool set_FT_Filter_Type(int slv_id, int filter_type, int sub_type);	// set filter
	bool set_FT_Cont_Interval(int slv_id, int interval);					// set force/torque output frq.
	bool set_FT_Bias(int slv_id, int is_on);								// set bias


	// 제어 전송 명령 형태, 0 이면 현재 제어 전송 명령이 없음....
	int  m_nCurrMode;								// current operation mode or command type
	bool m_bIsRcvd_Response_Pkt;					// receive flag for response packet of current command

	// 수신 데이터

	int mNumOfSlave; // 연결된 슬래이브 센서의 숫자
	int mNumOfRFTS;  // number of the connected RFTS

	bool InitializeSensor(void);

	// RT Thread Enable
	bool m_bRTThreadEnable;

	vector<RFTS> mRFTS;

	//////////////////////////////////////////////////////////////////////////////////
	// for log file
	
	CString m_logfileName;

	bool startLogging(CString logfileName = "");
	bool stopLogging(CString folder_to_save);
	
	double mLogData[MAX_LOGGING_NUM][MAX_NUM_OF_SLAVE][6]; // 스택 overflow 방지하기 위해 프로젝트 속성(링커/시스템/스택예약크기) 변경

	unsigned long m_nlogDataCount;

	int LoggingStatus(void);    // 


	// for SOEM Network Interface Card setting
	vector<string> m_vecNIC_Desc;
	vector<string> m_vecNic_Name;

protected:
	// 
	void initialize_variables(void);

	// for SOEM
	bool m_bInOP;  // for Requesting Initilize Operation to slave
	int m_nExpectedWKC; // 초기화시 Work Counter
	bool m_bNeedlf;
	volatile int m_nWKC;  // 확인된 Work Counter
	volatile int m_nRT_Cnt;  // Read Thread Counter
	char IOmap[4096];
	unsigned char m_ucCurrentGroup;
	UINT m_mmResult; // timer event id

//	unsigned char configCmdType;
//	unsigned char configParam[3];
//	unsigned char RFT_DATA_FIELD[17];
//	short RFT_FT_RAW[MAX_NUM_OF_SENSOR][6];
	void cvtBufferToInt16(short *value, unsigned char *buff);

	mutex mutex_PDO_output, mutex_PDO_input;

	void updateInputProcessData(void);
	void updateOutputProcessData(void);

	// RFTS hardware callbacks
	static void callback_update_product_name(void *ptr_rfts, void *ptr_rfts_if);
	static void callback_update_serial_number(void *ptr_rfts, void *ptr_rfts_if);
	static void callback_update_firmware_version(void *ptr_rfts, void *ptr_rfts_if);
	static void callback_start_FT_out(void *ptr_rfts, void *ptr_rfts_if);
	static void callback_stop_FT_out(void *ptr_rfts, void *ptr_rfts_if);
	static void callback_set_bias(void *ptr_rfts, void *ptr_rfts_if);
	static void callback_reset_bias(void *ptr_rfts, void *ptr_rfts_if);

	// for log file
	mutex m_logfile_mutex;
	bool m_bIsStartLogging;
	FILE *m_pLogFile;
	bool isLogFileSaving;
};

#endif//__RT_RFT_CAN_IF_H__

// END OF FILE
