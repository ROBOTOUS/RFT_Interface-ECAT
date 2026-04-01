/*
	Robotous FT-Sensor EtherCAT Interface 
	using SOEM Open Source Lib.
	
	Version 1 
	- Unify Interface for External ECAT board(EC02) and Embedded Type   
*/



#include "afxwin.h"
#include "RFTS_IF_ECAT_v1.h"

// PROCESSING DATA MAPPING FOR RFT EC02
// RFT data field index in RFT EC02 EtherCAT
#define IDX_D1 (0) 
#define IDX_D2 (1)
#define IDX_D3 (2)
#define IDX_D4 (3)
#define IDX_D5 (4)
#define IDX_D6 (5)
#define IDX_D7 (6)
#define IDX_D8 (7)
#define IDX_D9 (8) 
#define IDX_D10 (9)
#define IDX_D11 (10)
#define IDX_D12 (11)
#define IDX_D13 (12)
#define IDX_D14 (13)
#define IDX_D15 (14)
#define IDX_D16 (15)

#define IDX_RAW_FT1 (16) // 2 BYTES
#define IDX_RAW_FT2 (18) // 2 BYTES
#define IDX_RAW_FT3 (20) // 2 BYTES
#define IDX_RAW_FT4 (22) // 2 BYTES
#define IDX_RAW_FT5 (24) // 2 BYTES
#define IDX_RAW_FT6 (26) // 2 BYTES

#define IDX_OVERLOAD 	(28)
#define IDX_ERRLR_FLAG  (29)


// RFT-ECAT-V1 SDO
// sensor info
#define SDO_IDX_SENSOR_INFO			(0x2000)
#define SDO_SUB_IDX_PRODUCT_NAME	(1)
#define SDO_SUB_IDX_SERIAL_NUM		(2)
#define SDO_SUB_IDX_FW_VER			(3)
// sensor setup
#define SDO_IDX_SENSOR_SETUP		(0x2001)
#define SDO_SUB_IDX_BIAS			(1)
#define SDO_SUB_IDX_LPF_SETUP		(2)




// constructor and destructor
CRFTS_ECAT_IF::CRFTS_ECAT_IF()
{
	initialize_variables();

	ec_adapter *adapter = NULL;
	ec_adapter *adapter_old = NULL;

	adapter = ec_find_adapters();
	while (adapter != NULL)
	{
		string desc = adapter->desc;
		string name = adapter->name;
		m_vecNIC_Desc.push_back(desc);
		m_vecNic_Name.push_back(name);

		adapter_old = adapter;
		adapter = adapter->next;
		delete adapter_old;
	}
	delete adapter;

	//mutex_PDO_output.unlock();
	//mutex_PDO_input.unlock();
}

CRFTS_ECAT_IF::~CRFTS_ECAT_IF()
{
	stop();
}

bool CRFTS_ECAT_IF::stop(void)
{
	m_bIsEnabled_Callback = false;

	if (m_bEcatcheckThreadFlag)
	{
		m_bEcatcheckThreadFlag = false;
		m_hEcheckThread = NULL;
	}

	if (m_bOpCheckheckThreadFlag)
	{
		m_bOpCheckheckThreadFlag = false;
		m_hOpCheckThread = NULL;
	}

	/* stop RT thread */
	
	if (m_mmResult)
	{
		timeKillEvent(m_mmResult);
	}

	if (m_bInOP)
	{
		m_bInOP = false;

		ec_slave[0].state = EC_STATE_INIT; // 0 is master

		/* request INIT state for all slaves */
		ec_writestate(0);

		/* stop SOEM, close socket */
		ec_close();
	}

	return true;
}

bool CRFTS_ECAT_IF::run(string NIC_Name)
{
	bool result = false;

	int chk;

//	m_RFT_IF_PACKET.setDivider(50,1000);

	m_bIsEnabled_Callback = false;

	if (ec_init((char *) NIC_Name.c_str()))  // setup NIC
	{
		if (ec_config_init(FALSE) > 0)
		{
			// ETHERCAT CHECK THREAD
			m_bEcatcheckThreadFlag = true;
			// Start thread for checking and recovering EtherCAT communication 
			m_hEcheckThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ecatcheckThread, (LPVOID)this, 0, &m_dwEcheckThreadId);
			ec_config_map(&IOmap);
			ec_configdc();
	
			/* wait for all slaves to reach SAFE_OP state */
			ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

			m_nExpectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
			mNumOfSlave = ec_group[0].inputsWKC;

			ec_slave[0].state = EC_STATE_OPERATIONAL;

			/* send one valid process data to make outputs in slaves happy*/
			ec_send_processdata();
			ec_receive_processdata(EC_TIMEOUTRET);

			/* start RT thread as periodic MM timer */
			m_mmResult = timeSetEvent(1, 0, RTthread, (DWORD_PTR)this, TIME_PERIODIC);

			/* request OP state for all slaves */
			ec_writestate(0);
			chk = 40;
			/* wait for all slaves to reach OP state */
			do
			{
				ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
			} while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

			if (ec_slave[0].state == EC_STATE_OPERATIONAL)
			{
				// Start thread checking sensor operation mode
				//m_bOpCheckheckThreadFlag = true;
				//m_hOpCheckThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)opCheckThread, (LPVOID)this, 0, &m_dwOpCheckThreadId);
				m_bInOP = true;
				
				// check the types of connected RFTS interface, the set sensor spec by reading product name
				InitializeSensor();
				m_bIsEnabled_Callback = true;

				// initialize variable
				isLogFileSaving = false;
				result = true;

			}
			else
			{
				ec_readstate();
				for (int i = 1; i <= ec_slavecount; i++)
				{
					if (ec_slave[i].state != EC_STATE_OPERATIONAL)
					{
					}
				}

				CString message;
				message = "Not all slaves reached operational state.";
				::AfxMessageBox(message);

				stop();
				result = false;
			}
		}
		else
		{
			ec_close();
			CString message;
			message = "There is no ECAT slave";
			::AfxMessageBox(message);
			result = false;
		}
	}
	else
	{
		CString message;
		message.Format("No socket connection on %s", NIC_Name);
		::AfxMessageBox(message);
	}

	return result;
}


void CRFTS_ECAT_IF::ecatcheckThread(CRFTS_ECAT_IF *pThis)
{
	pThis->ecatcheckWorker();
}

void CRFTS_ECAT_IF::ecatcheckWorker(void)
{
	int slave;

	while (m_bEcatcheckThreadFlag)
	{

		if (m_bInOP && ((m_nWKC < m_nExpectedWKC) || ec_group[m_ucCurrentGroup].docheckstate))
		{
			
			/* one ore more slaves are not responding */
			ec_group[m_ucCurrentGroup].docheckstate = FALSE;
			ec_readstate();

			for (slave = 1; slave <= ec_slavecount; slave++)
			{
				if ((ec_slave[slave].group == m_ucCurrentGroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
				{
					ec_group[m_ucCurrentGroup].docheckstate = TRUE;
					if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
					{
						ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
						ec_writestate(slave);
					}
					else if (ec_slave[slave].state == EC_STATE_SAFE_OP)
					{
						ec_slave[slave].state = EC_STATE_OPERATIONAL;
						ec_writestate(slave);
					}
					else if (ec_slave[slave].state > 0)
					{
						if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
						{
							ec_slave[slave].islost = FALSE;
						}
					}
					else if (!ec_slave[slave].islost)
					{
						/* re-check state */
						ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
						if (!ec_slave[slave].state)
						{
							ec_slave[slave].islost = TRUE;
						}
					}
				}
				if (ec_slave[slave].islost)
				{
					if (!ec_slave[slave].state)
					{
						if (ec_recover_slave(slave, EC_TIMEOUTMON))
						{
							ec_slave[slave].islost = FALSE;
						}
					}
					else
					{
						ec_slave[slave].islost = FALSE;
					}
				}
			}
		}

		SleepEx(10, FALSE );
		//osal_usleep(10000); // [us]
	}
}


/* most basic RT thread for process data, just does IO transfer */
void CALLBACK CRFTS_ECAT_IF::RTthread(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CRFTS_ECAT_IF *pThis = (CRFTS_ECAT_IF*)dwUser;
	pThis->RTthreadWorker();
}

void CRFTS_ECAT_IF::RTthreadWorker(void)
{

	if (!m_bRTThreadEnable)
		return;


	updateOutputProcessData();
	ec_send_processdata();
	m_nWKC = ec_receive_processdata(EC_TIMEOUTRET);
	updateInputProcessData();

	m_nRT_Cnt++;

}

void CRFTS_ECAT_IF::setCallback(RFT_ECAT_IF_CALLBACK pCallbackFunc, void *callbackParam)
{
	m_pCallbackFunc = pCallbackFunc;
	m_pCallbackParam = callbackParam;

//	CONSOLE_S("SETTING.... RFT CAN I/F CALL BACK FUNCTION\n");
}



void CRFTS_ECAT_IF::initialize_variables(void)
{

	m_hEcheckThread = NULL;         // Handle of Thread
	m_dwEcheckThreadId = 0;
	m_bEcatcheckThreadFlag = false;
	
	m_hOpCheckThread = NULL;
	m_dwOpCheckThreadId = 0;
	m_bOpCheckheckThreadFlag = false;

	m_pCallbackFunc = NULL;
	m_pCallbackParam = NULL;

	m_bRTThreadEnable = true;

	// for logging
	m_pLogFile = NULL;
	m_nlogDataCount = 0;
	m_bIsStartLogging = false;

	m_bInOP = false;
	m_nExpectedWKC = 0;
	m_ucCurrentGroup = 0;

	m_nWKC = 0;
	m_nRT_Cnt = 0;

	m_mmResult = 0;
}

void CRFTS_ECAT_IF::cvtBufferToInt16(short *value, unsigned char *buff)
{
	unsigned char *temp = (unsigned char *)value;
	temp[0] = buff[0];
	temp[1] = buff[1];
}

void CRFTS_ECAT_IF::updateInputProcessData(void)
{
	int i,j;
	uint8_t *input_data;
	short ft_raw[6];
	float ft_val[6];

	for (int rfts_id = 0; rfts_id < mRFTS.size(); rfts_id++)
	{
		int slv_id = mRFTS[rfts_id].slave_id;
		
		switch (mRFTS[rfts_id].IF_type)
		{
		case RFTS_IF_ECAT_V1:
			memcpy(ft_val, ec_slave[slv_id].inputs, 24);
			break;
		case RFTS_IF_EC02:
			for (int i = 0; i < 16; i++)
				mRFTS[rfts_id].response[i] = ec_slave[slv_id].inputs[i];
			if (mRFTS[rfts_id].response[0] == CMD_FT_CONT)
				mRFTS[rfts_id].mode = RFTS_MODE_FT_CONT;
			else
				mRFTS[rfts_id].mode = RFTS_MODE_IDLE;

			for (int axis = 0; axis < 6; axis++)
			{
				cvtBufferToInt16( ft_raw + axis, ec_slave[slv_id].inputs + IDX_RAW_FT1 + axis * 2);
			}
			ft_val[0] = ft_raw[0] / mRFTS[rfts_id].force_divider; // fx
			ft_val[1] = ft_raw[1] / mRFTS[rfts_id].force_divider; // fy
			ft_val[2] = ft_raw[2] / mRFTS[rfts_id].force_divider; // fz
			ft_val[3] = ft_raw[3] / mRFTS[rfts_id].torque_divider; // tx
			ft_val[4] = ft_raw[4] / mRFTS[rfts_id].torque_divider; // ty
			ft_val[5] = ft_raw[5] / mRFTS[rfts_id].torque_divider; // tz

			break;
		default:
			break;
		}
		// update FT values
		mRFTS[rfts_id].updateFT(ft_val);
	}

	if (m_bIsEnabled_Callback)
	{
		// callback function...
		if (m_pCallbackFunc != NULL)
			m_pCallbackFunc(m_pCallbackParam); 
	}

	//if (RFT_DATA_FIELD[0] == CMD_FT_CONT) // 하나만 체크
	//{
	//	m_logfile_mutex.lock();
	//	// data logging
	//	if (m_bIsStartLogging)
	//	{

	//		for (int i = 0; i < mNumOfSlave; i++)
	//		for (int axis = 0; axis < 6; axis++)
	//		{
	//			mLogData[m_nlogDataCount][i][axis] = m_rcvdForce[i][axis];
	//		}
	//		m_nlogDataCount++;
	//		if (m_nlogDataCount >= MAX_LOGGING_NUM)
	//			m_nlogDataCount = MAX_LOGGING_NUM;

	//	}
	//	m_logfile_mutex.unlock();
	//}
}

void CRFTS_ECAT_IF::updateOutputProcessData(void)  
{
	int slv_id;
	for (RFTS rfts : mRFTS)
	{
		if (rfts.IF_type == RFTS_IF_EC02)
		{
			slv_id = rfts.slave_id;
			ec_slave[slv_id].outputs[0] = rfts.command;
			ec_slave[slv_id].outputs[1] = rfts.cmd_parameter[0];
			ec_slave[slv_id].outputs[2] = rfts.cmd_parameter[1];
			ec_slave[slv_id].outputs[3] = rfts.cmd_parameter[2];

			ec_slave[slv_id].outputs[7] = rfts.command;
			ec_slave[slv_id].outputs[6] = rfts.cmd_parameter[0];
			ec_slave[slv_id].outputs[4] = rfts.cmd_parameter[1];
			ec_slave[slv_id].outputs[3] = rfts.cmd_parameter[2];

		}
	}
}

void CRFTS_ECAT_IF::opCheckThread(CRFTS_ECAT_IF *pThis)
{
	pThis->opCheckWorker();
}

void CRFTS_ECAT_IF::opCheckWorker(void)
{
//	CONSOLE_S("Start... RFT Sensor Status Checking\n");

	//while (m_bOpCheckheckThreadFlag)
	//{
	//	switch (m_nCurrMode)
	//	{
	//	case CMD_GET_PRODUCT_NAME:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			for (int i = 0; i < 15; i++)
	//				mRFTS[m_nSlaveID-1].product_name[i] = RFT_DATA_FIELD[i + 1];
	//			mRFTS[m_nSlaveID - 1].product_name[PRODUCT_NAME_LEN] = 0;
	//			m_bIsRcvd_Response_Pkt = true;
	//		}

	//		break;

	//	case CMD_GET_SERIAL_NUMBER:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_GET_FIRMWARE_VER:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//		//case CMD_SET_ID: not supported at ethercat type
	//		//	break;

	//	case CMD_GET_ID:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_SET_FT_FILTER:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_RFT_IF_PACKET.m_response_result = RFT_DATA_FIELD[1];
	//			m_RFT_IF_PACKET.m_response_errcode = RFT_DATA_FIELD[2];
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_GET_FT_FILTER:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_FT_ONCE:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_FT_CONT:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_FT_CONT_STOP:
	//		// there is no response packet
	//		break;

	//	case CMD_SET_CONT_OUT_FRQ:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_GET_CONT_OUT_FRQ:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	case CMD_GET_OVERLOAD_COUNT:
	//		if (RFT_DATA_FIELD[0] == m_nCurrMode)
	//		{
	//			m_bIsRcvd_Response_Pkt = true;
	//		}
	//		break;

	//	default: break;
	//	}

	//	SleepEx(10, FALSE);
	//}

//	CONSOLE_S("Finish... RFT Sensor Status Checking\n");
}


// read product name


// read serial number
bool CRFTS_ECAT_IF::rqst_SerialNumber(int slv_id)
{
	//m_bIsRcvd_Response_Pkt = true;
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_GET_SERIAL_NUMBER;
	return true;
}

// read firmware version
bool CRFTS_ECAT_IF::rqst_Firmwareverion(int slv_id)
{
	//m_bIsRcvd_Response_Pkt = true;
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_GET_FIRMWARE_VER;
	return true;
}

// read CAN message ID
bool CRFTS_ECAT_IF::rqst_GetID(int slv_id)
{
	//m_bIsRcvd_Response_Pkt = true;
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_GET_ID;
	return true;
}

// read filter type
bool CRFTS_ECAT_IF::rqst_FT_Filter_Type(int slv_id)
{
	//m_bIsRcvd_Response_Pkt = true;
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_GET_FT_FILTER;
	return true;
}

// read force/torque (once)
// stop force/torque continuous output
bool CRFTS_ECAT_IF::FTS_stop_output(int rfts_id)
{
	//int slv_id = mRFTS[rfts_id].slave_id;

	//switch (mRFTS[rfts_id].IF_type)
	//{
	//case RFT_CAN:  // To-do
	//case RFT_UART:
	//case RFT_RS485:
	//	break;
	//case RFT_EC02:
	//	mutex_PDO_output.lock();
	//	ec_slave[slv_id].outputs[0] = CMD_FT_CONT_STOP;
	//	ec_slave[slv_id].outputs[1] = 0;
	//	ec_slave[slv_id].outputs[2] = 0;
	//	ec_slave[slv_id].outputs[3] = 0;

	//	ec_slave[slv_id].outputs[7] = CMD_FT_CONT_STOP;
	//	ec_slave[slv_id].outputs[6] = 0;
	//	ec_slave[slv_id].outputs[5] = 0;
	//	ec_slave[slv_id].outputs[4] = 0;
	//	mutex_PDO_output.unlock();
	//	delay_ms(50);

	//	break;

	//default:
	//	break;
	//}

	return true;
}
bool CRFTS_ECAT_IF::rqst_FT(int slv_id)
{
	//m_bIsRcvd_Response_Pkt = true;
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_FT_ONCE;
	return true;
}

// start force/torque continuous outpu
bool CRFTS_ECAT_IF::rqst_FT_Continuous(int slv_id)
{

	//m_nSlaveID = slv_id;

	//m_bIsRcvd_Response_Pkt = true; 
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_FT_CONT;
	//
	//Sleep(50);
	return true;
}


// read force/torque output frq
bool CRFTS_ECAT_IF::rqst_FT_Cont_Interval(int slv_id)
{
	//m_bIsRcvd_Response_Pkt = false;
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_GET_CONT_OUT_FRQ;
	return true;
}

// read overload count
bool CRFTS_ECAT_IF::rqst_FT_OverloadCnt(int slv_id)
{
	//m_bIsRcvd_Response_Pkt = false;
	//configParam[0] = 0;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_GET_OVERLOAD_COUNT;
	return true;
}

//bool set_ID(void);	// set CAN message ID - not supported for ECAT

// set filter
bool CRFTS_ECAT_IF::set_FT_Filter_Type(int slv_id, int filter_type, int sub_type)
{
	//m_nSlaveID = slv_id;

	//configParam[0] = filter_type;
	//configParam[1] = sub_type;
	//configParam[2] = 0;
	//configCmdType = CMD_SET_FT_FILTER;

	//int waitTimeOutCnt = 0;
	//do{
	//	Sleep(20);
	//	waitTimeOutCnt++;
	//	if (waitTimeOutCnt >= 50)
	//		break;
	//	if ((RFT_DATA_FIELD[0] == configCmdType))
	//	{
	//		return true;
	//	}
	//} while (1);

	//return false;
	
	return true;
}

// set force/torque output frq.
bool CRFTS_ECAT_IF::set_FT_Cont_Interval(int slv_id, int interval)
{
	//m_bIsRcvd_Response_Pkt = false;
	//configParam[0] = interval;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_SET_CONT_OUT_FRQ;
	return true;
}

// set bias
bool CRFTS_ECAT_IF::set_FT_Bias(int slv_id, int is_on)
{
	//int Mode_last = m_nCurrMode;

	//m_nSlaveID = slv_id;

	//configParam[0] = is_on;
	//configParam[1] = 0;
	//configParam[2] = 0;
	//configCmdType = CMD_SET_BIAS;
	//
	//Sleep(50);
	//configCmdType = CMD_NONE;
	return true;
}

bool CRFTS_ECAT_IF::startLogging(CString logfileName)
{
	bool result = true;

	if (isLogFileSaving)
	{
		::AfxMessageBox("잠시후 다시 시작할 것 - LogD Data 저장 중");
		return false;
	}

	m_logfile_mutex.lock();
	m_bIsStartLogging = true;
	m_nlogDataCount = 0;
	m_logfile_mutex.unlock();
	
	return true;
}

bool CRFTS_ECAT_IF::stopLogging(CString folder_to_save)
{
	isLogFileSaving = true;

	m_logfile_mutex.lock();
	m_bIsStartLogging = false;
	m_logfile_mutex.unlock();

	SYSTEMTIME	sysTime;
	::GetLocalTime(&sysTime);

	CString time;
	time.Format("_%04d%02d%02d_%02d%02d%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

	m_logfileName = "RFT_Data" + time + ".csv";
	CString path_filename;
	path_filename = folder_to_save + "\\" + m_logfileName;

	fopen_s(&m_pLogFile, path_filename.GetBuffer(), "w");
	
	if (m_pLogFile == NULL)
	{
		return false;
	}

	if (m_pLogFile != NULL)
	{
		for (int i = 0; i < m_nlogDataCount; i++)
		{
			double time = (float)i / 1000;
			fprintf(m_pLogFile, "%6.03f,", time);
			for (int slave = 1; slave <= mNumOfSlave; slave++)
			{
				fprintf(m_pLogFile, " %8.02f, %8.02f, %8.02f, %8.03f, %8.03f, %8.03f,",
					mLogData[i][slave - 1][0], mLogData[i][slave - 1][1], mLogData[i][slave - 1][2],
					mLogData[i][slave - 1][3], mLogData[i][slave - 1][4], mLogData[i][slave - 1][5]);
			}
			fprintf(m_pLogFile, "\n");
		}

		fclose(m_pLogFile);

	}
	isLogFileSaving = false;

	return true;
}
int CRFTS_ECAT_IF::LoggingStatus(void)
{
	if (m_bIsStartLogging)
		return LS_MEASURING;
	else if (isLogFileSaving)
		return LS_SAVING;
	else
		return LS_READY;
}

// check the types of connected RFTS interface, the set sensor spec by reading product name
bool CRFTS_ECAT_IF::InitializeSensor(void)
{
	RFTS rft_sensor;
	bool is_slave_rfts;

	// Delete All previous RFTS Object
	mRFTS.clear();

	// count connected RFTS and check type  
	for (int slave = 1; slave <= mNumOfSlave; slave++)
	{
		is_slave_rfts = true;

		string name = ec_slave[slave].name; // slave name
		if ((name == "RFT_EC02") || (name == "Robotous slave"))
		{
			rft_sensor.IF_type = RFTS_IF_EC02;
			rft_sensor.slave_id = slave;

		}
		else if (name == "RFT-ECAT-V1")
		{
			rft_sensor.IF_type = RFTS_IF_ECAT_V1;
			rft_sensor.slave_id = slave;
		}
		else
		{	
			is_slave_rfts = false;
		}

		if (is_slave_rfts)
		{
			// setup callback
			rft_sensor.set_ptr_RFTS_IF_Instance(this);

			rft_sensor.set_cb_function_update_product_name(callback_update_product_name);
			rft_sensor.set_cb_function_update_serial_number(callback_update_serial_number);
			rft_sensor.set_cb_function_update_firmware_version(callback_update_firmware_version);

			rft_sensor.set_cb_function_start_FT_out(callback_start_FT_out);
			rft_sensor.set_cb_function_stop_FT_out(callback_stop_FT_out);
			rft_sensor.set_cb_function_set_bias(callback_set_bias);
			rft_sensor.set_cb_function_reset_bias(callback_reset_bias);

			// save sensor object
			mRFTS.push_back(rft_sensor);


			// update sensor info
			int last_rfts_id = mRFTS.size() - 1;
			mRFTS[last_rfts_id].update_sensor_info();

			// check product name 
			if (mRFTS[last_rfts_id].product_name.substr(0, 3) != "RFT")
			{
				mRFTS.erase(mRFTS.end());
			}
		}
	}
	return true;
}

void CRFTS_ECAT_IF::callback_update_product_name(void *ptr_rfts, void *ptr_rfts_if)
{
	CRFTS_ECAT_IF *me = (CRFTS_ECAT_IF *)ptr_rfts_if;
	RFTS *rfts = (RFTS *)ptr_rfts;

	int slv_id = rfts->slave_id;

	int rcvd_size = 20;  // Desired data size to be received 
	char data[20];

	bool received = false;
	int waitTimeOutCnt = 0;

	switch (rfts->IF_type)
	{
	case RFTS_IF_CAN:  // To-do
		break;
	case RFTS_IF_UART:
		break;
	case RFTS_IF_RS485:
		break;
	case RFTS_IF_ECAT_V1:

		ec_SDOread(slv_id, SDO_IDX_SENSOR_INFO, SDO_SUB_IDX_PRODUCT_NAME, FALSE, &rcvd_size, data, EC_TIMEOUTRXM);
		data[PRODUCT_NAME_LEN] = 0;
		rfts->product_name = data;
		break;

	case RFTS_IF_EC02:
		rfts->command = CMD_GET_PRODUCT_NAME;
		rfts->cmd_parameter[0] = 0;
		rfts->cmd_parameter[1] = 0;
		rfts->cmd_parameter[2] = 0;

		do{
			delay_ms(50);
			if ((waitTimeOutCnt++) >= 50) // 2.5 sec 
				break;

			if ((rfts->response[IDX_D1] == CMD_GET_PRODUCT_NAME))
			{
				rfts->product_name = (char *)&(rfts->response[1]);
				received = true;
			}
		} while (!received);
		break;
	default:
		break;
	}
}

void CRFTS_ECAT_IF::callback_update_serial_number(void *ptr_rfts, void *ptr_rfts_if)
{
	CRFTS_ECAT_IF *me = (CRFTS_ECAT_IF *)ptr_rfts_if;
	RFTS *rfts = (RFTS *)ptr_rfts;

	int slv_id = rfts->slave_id;

	int rcvd_size = 20;  // Desired data size to be received 
	char data[20];

	bool received = false;
	int waitTimeOutCnt = 0;

	switch (rfts->IF_type)
	{
	case RFTS_IF_CAN:  // To-do
		break;
	case RFTS_IF_UART:
		break;
	case RFTS_IF_RS485:
		break;
	case RFTS_IF_ECAT_V1:

		ec_SDOread(slv_id, SDO_IDX_SENSOR_INFO, SDO_SUB_IDX_SERIAL_NUM, FALSE, &rcvd_size, data, EC_TIMEOUTRXM);
		data[SERIAL_NUM_LEN] = 0;
		rfts->serial_number = data;
		break;

	case RFTS_IF_EC02:

		rfts->command = CMD_GET_SERIAL_NUMBER;
		rfts->cmd_parameter[0] = 0;
		rfts->cmd_parameter[1] = 0;
		rfts->cmd_parameter[2] = 0;

		do{
			delay_ms(50);
			if ((waitTimeOutCnt++) >= 50) // 2.5 sec 
				break;

			if ((rfts->response[IDX_D1] == CMD_GET_SERIAL_NUMBER))
			{
				rfts->serial_number = (char *)&(rfts->response[1]);
				received = true;
			}
		} while (!received);

		break;
	default:
		break;
	}
}
void CRFTS_ECAT_IF::callback_update_firmware_version(void *ptr_rfts, void *ptr_rfts_if)
{
	CRFTS_ECAT_IF *me = (CRFTS_ECAT_IF *)ptr_rfts_if;
	RFTS *rfts = (RFTS *)ptr_rfts;

	int slv_id = rfts->slave_id;

	int rcvd_size = 20;  // Desired data size to be received 
	char data[20];

	bool received = false;
	int waitTimeOutCnt = 0;

	switch (rfts->IF_type)
	{
	case RFTS_IF_CAN:  // To-do
		break;
	case RFTS_IF_UART:
		break;
	case RFTS_IF_RS485:
		break;

	case RFTS_IF_ECAT_V1:

		ec_SDOread(slv_id, SDO_IDX_SENSOR_INFO, SDO_SUB_IDX_FW_VER, FALSE, &rcvd_size, data, EC_TIMEOUTRXM);
		data[FW_VER_LEN] = 0;
		rfts->firmware_version = data;
		break;

	case RFTS_IF_EC02:

		rfts->command = CMD_GET_FIRMWARE_VER;
		rfts->cmd_parameter[0] = 0;
		rfts->cmd_parameter[1] = 0;
		rfts->cmd_parameter[2] = 0;

		do{
			delay_ms(50);
			if ((waitTimeOutCnt++) >= 50) // 2.5 sec 
				break;

			if ((rfts->response[IDX_D1] == CMD_GET_FIRMWARE_VER))
			{
				rfts->firmware_version = (char *)&(rfts->response[1]);
				received = true;
			}
		} while (!received);

		break;
	default:
		break;
	}
}

void CRFTS_ECAT_IF::callback_start_FT_out(void *ptr_rfts, void *ptr_rfts_if)
{
	CRFTS_ECAT_IF *me = (CRFTS_ECAT_IF *)ptr_rfts_if;
	RFTS *rfts = (RFTS *)ptr_rfts;

	int slv_id = rfts->slave_id;

	switch (rfts->IF_type)
	{
	case RFTS_IF_CAN:  // To-do
		break;
	case RFTS_IF_UART:
		break;
	case RFTS_IF_RS485:
		break;
	case RFTS_IF_ECAT_V1:
		break;
	case RFTS_IF_EC02:

		rfts->command = CMD_FT_CONT;
		rfts->cmd_parameter[0] = 0;
		rfts->cmd_parameter[1] = 0;
		rfts->cmd_parameter[2] = 0;
		delay_ms(150);
		//rfts->mode = RFTS_MODE_FT_CONT;
		break;

	default:
		break;
	}

}

void CRFTS_ECAT_IF::callback_stop_FT_out(void *ptr_rfts, void *ptr_rfts_if)
{
	CRFTS_ECAT_IF *me = (CRFTS_ECAT_IF *)ptr_rfts_if;
	RFTS *rfts = (RFTS *)ptr_rfts;

	int slv_id = rfts->slave_id;

	switch (rfts->IF_type)
	{
	case RFTS_IF_CAN:  // To-do
		break;
	case RFTS_IF_UART:
		break;
	case RFTS_IF_RS485:
		break;
	case RFTS_IF_ECAT_V1:
		break;
	case RFTS_IF_EC02:

		rfts->command = CMD_FT_CONT_STOP;
		rfts->cmd_parameter[0] = 0;
		rfts->cmd_parameter[1] = 0;
		rfts->cmd_parameter[2] = 0;
		delay_ms(150);
		//rfts->mode = RFTS_MODE_IDLE;
		break;

	default:
		break;
	}

}
void CRFTS_ECAT_IF::callback_set_bias(void *ptr_rfts, void *ptr_rfts_if)
{
	CRFTS_ECAT_IF *me = (CRFTS_ECAT_IF *)ptr_rfts_if;
	RFTS *rfts = (RFTS *)ptr_rfts;
	uint16_t data;

	int slv_id = rfts->slave_id;

	switch (rfts->IF_type)
	{
	case RFTS_IF_CAN:  // To-do
		break;
	case RFTS_IF_UART:
		break;
	case RFTS_IF_RS485:
		break;
	case RFTS_IF_ECAT_V1:
		data = 1;
		ec_SDOwrite(slv_id, SDO_IDX_SENSOR_SETUP, SDO_SUB_IDX_BIAS, FALSE, 2, &data, EC_TIMEOUTRXM);
		break;
	case RFTS_IF_EC02:

		rfts->command = CMD_SET_BIAS;
		rfts->cmd_parameter[0] = 1;
		rfts->cmd_parameter[1] = 0;
		rfts->cmd_parameter[2] = 0;
		delay_ms(100);
		break;

	default:
		break;
	}
}
void CRFTS_ECAT_IF::callback_reset_bias(void *ptr_rfts, void *ptr_rfts_if)
{
	CRFTS_ECAT_IF *me = (CRFTS_ECAT_IF *)ptr_rfts_if;
	RFTS *rfts = (RFTS *)ptr_rfts;
	uint16_t data;

	int slv_id = rfts->slave_id;

	switch (rfts->IF_type)
	{
	case RFTS_IF_CAN:  // To-do
		break;
	case RFTS_IF_UART:
		break;
	case RFTS_IF_RS485:
		break;
	case RFTS_IF_ECAT_V1:
		data = 0;
		ec_SDOwrite(slv_id, SDO_IDX_SENSOR_SETUP, SDO_SUB_IDX_BIAS, FALSE, 2, &data, EC_TIMEOUTRXM);
		break;
	case RFTS_IF_EC02:

		rfts->command = CMD_SET_BIAS;
		rfts->cmd_parameter[0] = 0;
		rfts->cmd_parameter[1] = 0;
		rfts->cmd_parameter[2] = 0;
		delay_ms(100);
		break;
 
	default:
		break;
	}

}
// END OF FILE

