#include "afxwin.h"

#include "RFTS-V1.0.h"
#include "RT_Delay.h"

// Number of F/T data
#define RFT_NUM_OF_FORCE			(6)

// For packet definition
//#define RESPONSE_CAN_PACKET_CNT			(2)
//#define COMMAND_PACKET_DATA_FIELD_SIZE	(8)
//#define RESPONSE_PACKET_DATA_FIELD_SIZE (16)

//// Filter set value definitions
//#define FILTER_NONE			(0) // NONE
//#define FILTER_1ST_ORDER_LP	(1) // 1st order low-pass filter

// Error code.
#define ERROR_NONE				(0)
#define NOT_SUPPORTED_CMD		(1)
#define SET_VALUE_RANGE_ERR		(2)
#define EEPROM_WRITING_ERR		(3)

// For uart packet
#define SOP (0x55)
#define EOP (0xAA)
#define UART_COMMAND_PACKET_SIZE	(11) // SOP(1) + DATA FIELD(8)  + CHECK_SUM(1) + EOP(1)
#define UART_RESPONSE_PACKET_SIZE	(19) // SOP(1) + DATA FIELD(16) + CHECK_SUM(1) + EOP(1)

// Processing data mapping for EC02
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
// overload counter
#define SDO_IDX_OVERLOAD_CNT		(0x2002)


// Internal functions
void cvtBufferToInt16(short *value, unsigned char *buff);


// Vendor ID and Product Code
#define ROBOTOUS_VENDER_ID			(0x08EE)
#define RFTS_EC02_PRODUCT_CODE		(0x0002)
#define RFTS_ECAT_V1_PRODUCT_CODE	(0x0003)

// Rx Buffer Size

#define	MIN_RX_BUFFER_SIZE	10
#define	MAX_RX_BUFFER_SIZE	500000

RFTS::RFTS()
{
	m_RFTS_IF = NULL; 

	force_divider = 50;
	torque_divider = 1000;

	op_mode = OP_MODE::IDLE;

	can_id.rx = 100;
	can_id.tx1 = 1;
	can_id.tx2 = 2;

	mutex_ft_data = new mutex;	
	mutex_command_send = new mutex;
	mutex_response = new mutex; 
}
RFTS::~RFTS()
{
	delete mutex_ft_data;
	delete mutex_command_send; 
	delete mutex_response; 
};
// get force torque values as float
void RFTS::get_FT_values(float *ft)
{
	mutex_ft_data->lock();
	ft[0] = (float)FT.entry[0];
	ft[1] = (float)FT.entry[1];
	ft[2] = (float)FT.entry[2];
	ft[3] = (float)FT.entry[3];
	ft[4] = (float)FT.entry[4];
	ft[5] = (float)FT.entry[5];
	mutex_ft_data->unlock();
}
void RFTS::get_FT_values(double *ft)
{
	mutex_ft_data->lock();
	ft[0] = FT.entry[0];
	ft[1] = FT.entry[1];
	ft[2] = FT.entry[2];
	ft[3] = FT.entry[3];
	ft[4] = FT.entry[4];
	ft[5] = FT.entry[5];
	mutex_ft_data->unlock();
}
void RFTS::get_FT_values(FT_double& FT_measured)
{
	mutex_ft_data->lock();
	FT_measured = FT;
	mutex_ft_data->unlock();
}
void RFTS::get_FT_values(FT_float& FT_measured)
{
	mutex_ft_data->lock();
	FT_measured.entry[0] = (float)FT.entry[0];
	FT_measured.entry[1] = (float)FT.entry[1];
	FT_measured.entry[2] = (float)FT.entry[2];
	FT_measured.entry[3] = (float)FT.entry[3];
	FT_measured.entry[4] = (float)FT.entry[4];
	FT_measured.entry[5] = (float)FT.entry[5];
	mutex_ft_data->unlock();
}


void RFTS::get_overload_state(RFTS_OVERLOAD_STATE& s)
{
	mutex_ft_data->lock();
	s = overload_state;
	mutex_ft_data->unlock();
}

void RFTS::get_temperaure(float& t)
{
	mutex_ft_data->lock();
	t = (float)temperature;
	mutex_ft_data->unlock();
}
void RFTS::get_temperaure(double& t)
{
	mutex_ft_data->lock();
	t = temperature;
	mutex_ft_data->unlock();
}

bool RFTS::update_product_name(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::GET_PRODUCT_NAME;
		uint8_t command_pkt[8] = { (uint8_t)command, };
		if (send_command(command_pkt))
		{
			product_name = (char *)&(response[1]);
			ret = true;
		}
		else // Time out
		{
			error_msg = "Response Time Out";
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		ret = read_ecat_SDO(slave_id, SDO_IDX_SENSOR_INFO, SDO_SUB_IDX_PRODUCT_NAME, response);
		response[15] = 0;
		product_name = (char *)response;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	// Set sensor information by the received product name (model name)
	string size;
	if (product_name.size() >= 8)
	{
		size = product_name.substr(3, 2);
		if ((size == "40") || (size == "44") || (size == "60") || (product_name.substr(0, 8) =="RFT64-SB") )
		{
			force_divider = 50;
			torque_divider = 2000;
		}
		else
		{
			force_divider = 50;
			torque_divider = 1000;
		}
	}
	return ret;
}
bool RFTS::update_serial_number(void)
{
	bool ret = false;


	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::GET_SERIAL_NUMBER;
		uint8_t command_pkt[8] = { (uint8_t)command, };
		if (send_command(command_pkt))
		{
			serial_number = (char *)&(response[1]);
			ret = true;
		}
		else // Time out
		{
			error_msg = "Response Time Out";
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		ret = read_ecat_SDO(slave_id, SDO_IDX_SENSOR_INFO, SDO_SUB_IDX_SERIAL_NUM, response);
		response[15] = 0;
		serial_number = (char *)response;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::update_firmware_version(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::GET_FIRMWARE_VER;
		uint8_t command_pkt[8] = { (uint8_t)command, };
		if (send_command(command_pkt))
		{
			firmware_version = (char *)&(response[1]);
			ret = true;
		}
		else // Time out
		{
			error_msg = "Response Time Out";
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		ret = read_ecat_SDO(slave_id, SDO_IDX_SENSOR_INFO, SDO_SUB_IDX_FW_VER, response);
		response[15] = 0;
		firmware_version = (char *)response;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}

bool RFTS::update_sensor_info(void)
{
	if (protocol != PROTOCOL::ECAT_V1)
	{
		stop_FT_out();
	}
	if (!update_product_name()) return false;
	if (!update_serial_number()) return false;
	if (!update_firmware_version()) return false;
	if (!update_filter_data()) return false;
	if (!update_output_rate_data()) return false;
	if (!update_overload_counter()) return false;

	return true;
}

bool RFTS::start_FT_out(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::FT_CONT;
		uint8_t command_pkt[8] = { (uint8_t)command, };

		send_command(command_pkt);
		delay_ms(100);
		ret = true;
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		ret = true;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}
	return ret;
}

bool RFTS::stop_FT_out(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::FT_CONT_STOP;
		uint8_t command_pkt[8] = { (uint8_t)command, };

		send_command(command_pkt);
		ret = true;
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		ret = true;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}

bool RFTS::set_bias(bool on) // return 'true' means suceess of command sending 
{
	bool ret = false;
	int para;
	
	if (on)
		para = 1;
	else
		para = 0;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::SET_BIAS;
		uint8_t command_pkt[8] = { (uint8_t)command, para, };
		if (send_command(command_pkt))
		{
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		ret = write_ecat_SDO(slave_id, SDO_IDX_SENSOR_SETUP, SDO_SUB_IDX_BIAS, 2, &para);
	}
	else
	{
		error_msg = "Invaild RTX Type";
		ret = false;
	}

	return ret;
}
bool RFTS::set_filter(RFTS_LPF_CUTOFF para)
{
	bool ret = false;

	if (para < RFTS_LPF_CUTOFF::END)
	{
		if (rtx_type == RTX_TYPE::PACKET)
		{
			command = RFTS_COMMAND::SET_FT_FILTER;
			uint8_t command_pkt[8] = { (uint8_t)command, 1, (uint8_t)para, };
			if (send_command(command_pkt))
			{
				if (response[1] == 1) // success
				{
					LPF_cutoff = RFTS_LPF_cutoff_value[(int)para]; //update data by command
					ret = true;
				}
				else
				{
					error_msg = "Error Responsed : Set Filter Failed";
				}
			}
			else // Time out
			{
				error_msg = "Response Time Out";
			}
		}
		else if (rtx_type == RTX_TYPE::ECAT_PDO)
		{
			uint16_t co = RFTS_LPF_cutoff_value[(int)para];
	
			ret = write_ecat_SDO(slave_id, SDO_IDX_SENSOR_SETUP, SDO_SUB_IDX_LPF_SETUP, 2, &co);
			if (ret)
				LPF_cutoff = RFTS_LPF_cutoff_value[(int)para];
		}
	}
	else
	{
		error_msg = "Invalid Cut-off Parameter";
		ret = false;
	}
	return ret;
}

bool RFTS::update_filter_data(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::GET_FT_FILTER;
		uint8_t command_pkt[8] = { (uint8_t)command, };
		if (send_command(command_pkt))
		{
			LPF_cutoff = RFTS_LPF_cutoff_value[response[2]]; //update data 
			ret = true;
		}
		else // Time out
		{
			error_msg = "Response Time Out";
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		uint16_t para_rcvd;
		ret = read_ecat_SDO(slave_id, SDO_IDX_SENSOR_SETUP, SDO_SUB_IDX_LPF_SETUP, &para_rcvd);
		if (ret)
		{
			LPF_cutoff = para_rcvd;  // update data
		}
	}

	return ret;
}
bool RFTS::get_filter(uint16_t& co_freq)
{
	bool ret = update_filter_data();

	if (ret) // if success
	{
		co_freq = LPF_cutoff;
	}

	return ret;
}
bool RFTS::set_output_rate(RFTS_OUTPUT_RATE para)
{
	bool ret = false;

	if (para < RFTS_OUTPUT_RATE::END)
	{
		if (rtx_type == RTX_TYPE::PACKET)
		{
			command = RFTS_COMMAND::SET_OUTPUT_RATE;
			uint8_t command_pkt[8] = { (uint8_t)command, (uint8_t)para, };
			if (send_command(command_pkt))
			{
				if (response[1] == 1) // success
				{
					output_rate = RFTS_output_rate_value[(int)para]; //update data by command
					ret = true;
				}
				else
				{
					error_msg = "Error Responsed : Set Output-rate Failed";
				}
			}
			else // Time out
			{
				error_msg = "Response Time Out";
			}
		}
		else if (rtx_type == RTX_TYPE::ECAT_PDO)
		{
			output_rate = 0;
			ret = true;
		}
	}
	else
	{
		error_msg = "Invalid Cut-off Parameter";
	}
	return ret;
}
bool RFTS::update_output_rate_data(void)
{
	bool ret = false;
	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::GET_OUTPUT_RATE;
		uint8_t command_pkt[8] = { (uint8_t)command, };
		if (send_command(command_pkt))
		{
			output_rate = RFTS_output_rate_value[response[1]]; //update data 
			ret = true;
		}
		else // Time out
		{
			error_msg = "Response Time Out";
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		output_rate = 0;
		ret = true;
	}

	return ret;
}
bool RFTS::get_output_rate(uint16_t& out_rate)
{
	bool ret = update_output_rate_data();

	if (ret) // if success
	{
		out_rate = output_rate;
	}
	return ret;
}

bool RFTS::update_overload_counter(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::GET_OVERLOAD_COUNT;
		uint8_t command_pkt[8] = { (uint8_t)command, };
		if (send_command(command_pkt))
		{
			memcpy(overload_counter.byte, &(response[1]), 6); //update data 
			ret = true;
		}
		else // Time out
		{
			error_msg = "Response Time Out";
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		uint8_t sub_idx = 1;
		for (auto& cnt: overload_counter.byte)
		{
			ret = read_ecat_SDO(slave_id, SDO_IDX_OVERLOAD_CNT, sub_idx++, &cnt);
		}
	}

	return ret;
}

bool RFTS::get_overload_counter(RFTS_OVERLOAD_COUNTER& oc)
{
	bool ret = update_overload_counter();

	if (ret) // if success
	{
		oc = overload_counter;
	}
	return ret;
}

#ifdef RFTS_USE_CAN
bool RFTS::set_can_id(uint8_t rx_id, uint8_t tx1_id, uint8_t tx2_id)
{
	bool ret = false;

	if (!is_value_in_range<uint8_t>(rx_id, 1, 255) ||
		!is_value_in_range<uint8_t>(tx1_id, 1, 255) ||
		!is_value_in_range<uint8_t>(tx2_id, 1, 255) ||
		(rx_id == tx1_id) || (rx_id == tx2_id) || (tx1_id == tx2_id))
	{
		error_msg = "Invalid CAN ID Set";
		return false;
	}
	command = RFTS_COMMAND::SET_CAN_ID;
	uint8_t command_pkt[8] = { (uint8_t)command, rx_id, tx1_id, tx2_id, };
	if (send_command(command_pkt))
	{
		if (response[1] == 1) // success
		{
			ret = true;
		}
		else
		{
			error_msg = "Error Responsed : Set CAN ID Failed";
		}
	}
	else // Time out
	{
		error_msg = "Response Time Out";
	}
	return ret;
}
bool RFTS::update_can_id(void)
{
	bool ret = false;
	command = RFTS_COMMAND::GET_CAN_ID;
	uint8_t command_pkt[8] = { (uint8_t)command, };
	if (send_command(command_pkt))
	{
		can_id.rx = response[1];
		can_id.tx1 = response[2];
		can_id.tx2 = response[3];
		can_id_next_boot.rx = response[4];
		can_id_next_boot.tx1 = response[5];
		can_id_next_boot.tx2 = response[6];
		ret = true;
	}
	else // Time out
	{
		error_msg = "Response Time Out";
	}
	return ret;

}
bool RFTS::get_can_id(RFTS_CAN_ID& id_current, RFTS_CAN_ID& id_next_boot)
{
	bool ret = update_can_id();

	if (ret) // if success
	{
		id_current = can_id;
		id_next_boot = can_id_next_boot;
	}
	return ret;
}
#endif

#ifdef RFTS_USE_UART
bool RFTS::set_uart_baudrate(RFTS_BAUD_RATE br)
{
	bool ret = false;

	command = RFTS_COMMAND::SET_UART_BAUDRATE;
	uint8_t command_pkt[8] = { (uint8_t)command, (uint8_t)br };
	if (send_command(command_pkt))
	{
		if (response[1] == 1) // success
		{
			ret = true;
		}
		else
		{
			error_msg = "Error Responsed : Set CAN ID Failed";
		}
	}
	else // Time out
	{
		error_msg = "Response Time Out";
	}
	return ret;
}
bool RFTS::update_uart_baudrate(void)
{
	bool ret = false;
	command = RFTS_COMMAND::GET_UART_BAUDRATE;
	uint8_t command_pkt[8] = { (uint8_t)command, };
	if (send_command(command_pkt))
	{
		uart_baudrate = (RFTS_BAUD_RATE)response[1];
		uart_baudrate_next_boot = (RFTS_BAUD_RATE)response[2];
		ret = true;
	}
	else // Time out
	{
		error_msg = "Response Time Out";
	}
	return ret;

}
bool RFTS::get_uart_baudrate(RFTS_BAUD_RATE& br_current, RFTS_BAUD_RATE& br_next_boot)
{
	bool ret = update_uart_baudrate();

	if (ret) // if success
	{
		br_current = uart_baudrate;
		br_next_boot = uart_baudrate_next_boot;
	}
	return ret;
}
#endif


RT_CircularBuffer<RFTS_RX_FT_DATA>* RFTS::get_FT_buffer()
{
	return &FT_buffer;
}
RT_CircularBuffer<RFTS_RX_CAP_DATA>* RFTS::get_Cap_buffer()
{
	return &Cap_buffer;
}

void RFTS::set_callback_on_FT_receive(FT_RCVD_CALLBACK_FUNC cb_func, void *callbacked_object)
{
	callback_on_FT_received = cb_func;
	callbacked_object_on_FT_received = callbacked_object;
}

void RFTS::set_callback_on_Cap_receive(CAP_RCVD_CALLBACK_FUNC cb_func, void *callbacked_object)
{
	callback_on_Cap_received = cb_func;
	callbacked_object_on_Cap_received = callbacked_object;
}

bool RFTS::send_command(uint8_t* cmd_pkt, uint8_t *ext_pkt, int ext_pkt_len)
{
	clear_response_data();

	// ecat의 경우에는 tx callback에서 알아서 보내지만 
	// can과 uart type일 경우에는 직접 보내줘야한다.
	if (m_RFTS_IF != NULL)
	{
		lock_guard<mutex> lock_guard(*mutex_command_send);

		// for EC02
		cmd_parameter[0] = cmd_pkt[1];
		cmd_parameter[1] = cmd_pkt[2];
		cmd_parameter[2] = cmd_pkt[3];

#ifdef RFTS_USE_CAN
		if (protocol == PROTOCOL::CAN)
		{
			if (((CAN_Interface *)m_RFTS_IF)->is_enabled())
			{
				if (!((CAN_Interface *)m_RFTS_IF)->send((uint32_t)can_id.rx, cmd_pkt, 8))
					return false;;
				if (!((CAN_Interface *)m_RFTS_IF)->send((uint32_t)can_id.rx, ext_pkt, ext_pkt_len))
					return false;
			} 
			else // not enabled
			{
				return false;
			}
		}
#endif
#ifdef RFTS_USE_UART
		// 8/25/2022 수정
		if (protocol == PROTOCOL::UART)
		{
			if (((UART_Interface *)m_RFTS_IF)->is_connected())
			{
				bool ret;
				int pkt_size ;

				uint8_t *pkt;

				if (ext_pkt_len == 0)
				{
					pkt_size = 11;
					pkt = new uint8_t[pkt_size];
					pkt[0] = 0x55u; // SOP
					for (int i = 0; i < 8; i++)
						pkt[1 + i] = cmd_pkt[i];
				}
				else
				{
					switch ((RFTS_COMMAND)cmd_pkt[0])
					{
					case RFTS_COMMAND::SET_PRODUCT_NAME:
					case RFTS_COMMAND::SET_SERIAL_NUMBER:
						pkt_size = 21;
						break;
					case RFTS_COMMAND::SET_CAL_DATA:
					case RFTS_COMMAND::SET_TEMP_COMP_DATA:
						pkt_size = 6 + ext_pkt_len;
						break;
					default:
						return false;
					}
					pkt = new uint8_t[pkt_size];
					pkt[0] = 0x55u; // SOP
					pkt[1] = cmd_pkt[0]; // command
					pkt[2] = cmd_pkt[1]; // MF Code "R"
					pkt[3] = cmd_pkt[2]; // MF Code "T"

					for (int i = 0; i < pkt_size - 6; i++)
						pkt[4 + i] = ext_pkt[i];
				}
				pkt[pkt_size - 2] = ((UART_Interface *)m_RFTS_IF)->get_check_sum(&pkt[1], pkt_size - 3); // except sop, checksum, eop
				pkt[pkt_size - 1] = 0xAAu; // EOP
				DWORD byte_written;
				ret = ((UART_Interface *)m_RFTS_IF)->write(pkt,(DWORD)pkt_size, &byte_written);

				delete[] pkt;
				if (!ret)
					return false;
			}
			else // not connected
			{
				return false;
			}
		}
#endif
	}
	else // no interface
	{
		return false;
	}

	// 다음의 명령일 때는 회신이 없다.
	if (//(cmd_pkt[0] == (uint8_t)RFTS_COMMAND::CAP_CONT) ||
		//(cmd_pkt[0] == (uint8_t)RFTS_COMMAND::FT_CONT) ||
		(cmd_pkt[0] == (uint8_t)RFTS_COMMAND::SET_BIAS) ||
		(cmd_pkt[0] == (uint8_t)RFTS_COMMAND::FT_CONT_STOP) ||
		(cmd_pkt[0] == (uint8_t)RFTS_COMMAND::CAP_CONT_STOP))
	{
		int additional_delay;  // 수신 데이터를 처리하기 위해
		switch (protocol)
		{
		case PROTOCOL::CAN:	additional_delay = 100; break;
		case PROTOCOL::RS485:
		case PROTOCOL::UART:additional_delay = 100; break;
		default: additional_delay = 100; break;
		}
		delay_ms(additional_delay);
		return true;
	}
	else
	{
		return wait_resonse();
	}
}

bool RFTS::wait_resonse()
{
#define WAIT_TIME_STEP 20

	int time_out_ms;
	switch (protocol)
	{
	case PROTOCOL::CAN:	time_out_ms = 500; break;
	case PROTOCOL::RS485:
	case PROTOCOL::UART:time_out_ms = 1000; break;
	default: time_out_ms = 1000; break;
	}

	int wait_cnt = 0;
	int wait_cnt_max = time_out_ms / WAIT_TIME_STEP;
	bool ret = false;

	do{
		delay_ms(WAIT_TIME_STEP);
		if ((wait_cnt++) >= wait_cnt_max)
		{
			error_msg = "Response Time-out";
			break;
		}
		if ((response[0] == (uint8_t)command))
		{
			response_result = response[1];
			response_error_code = response[2];
			ret = true;
		}
	} while (!ret);

	return ret;
}

void RFTS::clear_response_data(void)
{
	mutex_response->lock();
	memset(response, 0, sizeof(response));
	mutex_response->unlock();
}


#ifdef RFTS_USE_CAN
void RFTS::callback_can_rx(void *me, uint8_t *msg, uint32_t id)
{
	RFTS *rfts = (RFTS *)me;
	
	// 현재 명령에 따라 받을 메시지 카운터 결정 
	// 모든 회신은 캡정보를 제외하고 모두 16byte --> 2 MSG
	// 캡정보의 경우에도 6Cap version -> 16byte, 8Cap version ->24byte
	int msg_cnt_expected = 2;
	rfts->mutex_command_send->lock();
	if (rfts->command == RFTS_COMMAND::CAP_CONT)
	{
		msg_cnt_expected = 3;
	}
	rfts->mutex_command_send->unlock();

	// 회신 명령과 송신 명령이 일치하는 지 확인하고,
	// 순서대로 들어온 데이터만 유효하게 임시 버퍼에 저장
	static uint8_t response_buf[24];
	static int msg_cnt_received = 0;
	static bool reception_completed = false;

	if (id == rfts->can_id.tx1) 
	// rfts 송신아이디가 첫번째 id 이면
	{
		memcpy(response_buf, msg, 8);
		msg_cnt_received = 1;
	}

	if (id == rfts->can_id.tx2)
	// rfts 송신아이디가 두번째 id 이면 
	{
		if (msg_cnt_received == 1) // 첫번째가 받아 졌으면
		{
			memcpy(&response_buf[8], msg, 8);
			msg_cnt_received = 2;
		}
		else // 첫번째가 없으면 수신 카운터 초기화
		{
			msg_cnt_received = 0;
		}
	}
	if (id == (rfts->can_id.tx2 + 1))
	// rfts 송신아이디가 세번째 id 이면 
	{
		if (msg_cnt_received == 2) // 두번째가 순차적으로 받아 졌으면
		{
			memcpy(&response_buf[16], msg, 8);
			msg_cnt_received = 3;
		}
		else // 두번째가 없으면 수신 카운터 초기화
		{
			msg_cnt_received = 0;
		}
	}

	typedef union{
		short int_val;
		struct{
			uint8_t lowerbyte;
			uint8_t upperbyte;
		};
	} uInt;

	typedef union{
		uint16_t int_val;
		struct{
			uint8_t lowerbyte;
			uint8_t upperbyte;
		};
	} uUint;


	if (msg_cnt_received == msg_cnt_expected) // 수신이 완료되면
	{
		if (response_buf[0] == (uint8_t)RFTS_COMMAND::FT_CONT)
		{
			rfts->op_mode = OP_MODE::FT_CONT;

			uInt ft_raw[6];
			for (int axis = 0; axis < 6; axis++)
			{
				ft_raw[axis].lowerbyte = response_buf[axis * 2 + 2];
				ft_raw[axis].upperbyte = response_buf[axis * 2 + 1];
			}

			rfts->mutex_ft_data->lock();
			rfts->FT.entry[0] = (double)ft_raw[0].int_val / rfts->force_divider; // fx
			rfts->FT.entry[1] = (double)ft_raw[1].int_val / rfts->force_divider; // fy
			rfts->FT.entry[2] = (double)ft_raw[2].int_val / rfts->force_divider; // fz
			rfts->FT.entry[3] = (double)ft_raw[3].int_val / rfts->torque_divider; // tx
			rfts->FT.entry[4] = (double)ft_raw[4].int_val / rfts->torque_divider; // ty
			rfts->FT.entry[5] = (double)ft_raw[5].int_val / rfts->torque_divider; // tz

			rfts->overload_state.byte = rfts->response[13];
			rfts->temperature = (double)(rfts->response[14] < 125 ? rfts->response[14] : -(256 - rfts->response[14])); // 변환
			
			// save to buffer
			RFTS_RX_FT_DATA ft_data;
			ft_data.FT = rfts->FT;
			ft_data.OL_state = rfts->overload_state;
			ft_data.temperature = rfts->temperature;
			rfts->FT_buffer.write(ft_data);

			// callback on FT data received
			if (rfts->callback_on_FT_received)
			{
				rfts->callback_on_FT_received(rfts->callbacked_object_on_FT_received, rfts->FT.entry);
			}
			rfts->mutex_ft_data->unlock();
		}
		else if (rfts->response[0] == (uint8_t)RFTS_COMMAND::CAP_CONT)
		{
			rfts->op_mode = OP_MODE::CAP_CONT;

			uUint cap_raw[8];
			for (int ch = 0; ch < 8; ch++)
			{
				cap_raw[ch].lowerbyte = response_buf[ch * 2 + 2];
				cap_raw[ch].upperbyte = response_buf[ch * 2 + 1];
			}

			rfts->mutex_ft_data->lock();
			rfts->Cap[0] = (double)cap_raw[0].int_val;
			rfts->Cap[1] = (double)cap_raw[1].int_val;
			rfts->Cap[2] = (double)cap_raw[2].int_val;
			rfts->Cap[3] = (double)cap_raw[3].int_val;
			rfts->Cap[4] = (double)cap_raw[4].int_val;
			rfts->Cap[5] = (double)cap_raw[5].int_val;
			rfts->Cap[6] = (double)cap_raw[6].int_val;
			rfts->Cap[7] = (double)cap_raw[7].int_val;

			// save to buffer
			RFTS_RX_CAP_DATA cap_data;
			for (int ch = 0; ch < 8;ch++)
				cap_data.Cap[ch] = rfts->Cap[ch];
			rfts->Cap_buffer.write(cap_data);

			// callback on Cap data received
			if (rfts->callback_on_Cap_received)
			{
				rfts->callback_on_Cap_received(rfts->callbacked_object_on_Cap_received,rfts->Cap);
			}
			rfts->mutex_ft_data->unlock();
		}
		else // 나머지 명령에 대한 회신 처리는 각 명령함수에서 수행
		{
			rfts->op_mode = OP_MODE::IDLE;
		}

		rfts->mutex_response->lock();
		if (msg_cnt_expected == 2)
			memcpy(rfts->response, response_buf, 16);
		if (msg_cnt_expected == 3)
			memcpy(rfts->response, response_buf, 24);
		rfts->mutex_response->unlock();

		// clear msg_cnt_received 
		msg_cnt_received = 0;
	}
}
#endif

#ifdef RFTS_USE_UART
void RFTS::callback_uart_rx(void *me, DWORD bytes_received)
{
	RFTS *rfts = (RFTS *)me;
	UART_Interface *uart_if = (UART_Interface *)(rfts->m_RFTS_IF);

	// 현재 명령에 따라 받을 메시지 카운터 결정 
	// 모든 회신은 캡정보를 제외하고 모두 16bytes + 3bytes (sop, checksum, eop) 
	// 캡정보의 경우에도 6Cap version -> 16bytes, 8Cap version ->24byte
	DWORD bytes_expected = 19;
	bool clear_rx_buffer = false;
	rfts->mutex_command_send->lock();
	if (rfts->command == RFTS_COMMAND::CAP_CONT)
	{
		bytes_expected = 23; // sop(1) + cmd (1) + cap(8x2) + 1 + temperature(1) + 1 + checksum(1) + eop(1)
	}
	if (rfts->command == RFTS_COMMAND::CAP_CONT_STOP)
	{
		clear_rx_buffer = true;
	}
	if (rfts->command == RFTS_COMMAND::FT_CONT_STOP)
	{
		clear_rx_buffer = true;
	}
	rfts->mutex_command_send->unlock();

	if (clear_rx_buffer)
	{
		uart_if->clear_rx_buffer();
		return;
	}
	
	// 수신 바이트 수 확인
	if (bytes_received < bytes_expected)
		return;
	//	uint8_t *rx_buffer = new uint8_t[bytes_expected];
	uint8_t rx_buffer[1000];
	uart_if->read(rx_buffer, bytes_expected);

	// 회신 명령과 송신 명령이 일치하는 지 확인하고,
	// 순서대로 들어온 데이터만 유효하게 임시 버퍼에 저장

	static int lost_cnt = 0;

	if ((rx_buffer[0] == 0x55) &&  // sop
		(rx_buffer[bytes_expected - 2] == uart_if->get_check_sum(&rx_buffer[1], bytes_expected - 3)) &&
		(rx_buffer[bytes_expected - 1] == 0xAA))  //eop
	{
		rfts->mutex_response->lock();
		memcpy(rfts->response, &rx_buffer[1], bytes_expected - 3);
		rfts->mutex_response->unlock();
	}
	else
	{
		lost_cnt++;
		return;
	}
//	delete[] rx_buffer;
	typedef union{
		short int_val;
		struct{
			uint8_t lowerbyte;
			uint8_t upperbyte;
		};
	} uInt;

	typedef union{
		uint16_t int_val;
		struct{
			uint8_t lowerbyte;
			uint8_t upperbyte;
		};
	} uUint;

	if (rfts->response[0] == (uint8_t)RFTS_COMMAND::FT_CONT)
	{
		rfts->op_mode = OP_MODE::FT_CONT;

		uInt ft_raw[6];
		for (int axis = 0; axis < 6; axis++)
		{
			ft_raw[axis].lowerbyte = rfts->response[axis * 2 + 2];
			ft_raw[axis].upperbyte = rfts->response[axis * 2 + 1];
		}

		rfts->mutex_ft_data->lock();
		rfts->FT.entry[0] = (double)ft_raw[0].int_val / rfts->force_divider; // fx
		rfts->FT.entry[1] = (double)ft_raw[1].int_val / rfts->force_divider; // fy
		rfts->FT.entry[2] = (double)ft_raw[2].int_val / rfts->force_divider; // fz
		rfts->FT.entry[3] = (double)ft_raw[3].int_val / rfts->torque_divider; // tx
		rfts->FT.entry[4] = (double)ft_raw[4].int_val / rfts->torque_divider; // ty
		rfts->FT.entry[5] = (double)ft_raw[5].int_val / rfts->torque_divider; // tz

		rfts->overload_state.byte = rfts->response[13];
		rfts->temperature = (double)(rfts->response[14] < 125 ? rfts->response[14] : -(256 - rfts->response[14])); // 변환

		// save to buffer
		RFTS_RX_FT_DATA ft_data;
		ft_data.FT = rfts->FT;
		ft_data.OL_state = rfts->overload_state;
		ft_data.temperature = rfts->temperature;
		rfts->FT_buffer.write(ft_data);

		// callback on FT data received
		if (rfts->callback_on_FT_received)
		{
			rfts->callback_on_FT_received(rfts->callbacked_object_on_FT_received, rfts->FT.entry);
		}
		rfts->mutex_ft_data->unlock();
	}
	else if (rfts->response[0] == (uint8_t)RFTS_COMMAND::CAP_CONT)
	{
		rfts->op_mode = OP_MODE::CAP_CONT;

		uUint cap_raw[8];
		for (int ch = 0; ch < 8; ch++)
		{
			cap_raw[ch].lowerbyte = rfts->response[ch * 2 + 2];
			cap_raw[ch].upperbyte = rfts->response[ch * 2 + 1];
		}
		rfts->mutex_ft_data->lock();

		rfts->Cap[0] = (double)cap_raw[0].int_val;
		rfts->Cap[1] = (double)cap_raw[1].int_val;
		rfts->Cap[2] = (double)cap_raw[2].int_val;
		rfts->Cap[3] = (double)cap_raw[3].int_val;
		rfts->Cap[4] = (double)cap_raw[4].int_val;
		rfts->Cap[5] = (double)cap_raw[5].int_val;
		rfts->Cap[6] = (double)cap_raw[6].int_val;
		rfts->Cap[7] = (double)cap_raw[7].int_val;

		rfts->temperature = (double)(rfts->response[18] < 125 ? rfts->response[18] : -(256 - rfts->response[18])); // 변환

		// save to buffer
		RFTS_RX_CAP_DATA cap_data;
		for (int ch = 0; ch < 8; ch++)
			cap_data.Cap[ch] = rfts->Cap[ch];
		rfts->Cap_buffer.write(cap_data);

		// callback on Cap data received
		if (rfts->callback_on_Cap_received)
		{
			rfts->callback_on_Cap_received(rfts->callbacked_object_on_Cap_received, rfts->Cap);
		}
		rfts->mutex_ft_data->unlock();
	}
	else // 나머지 명령에 대한 회신 처리는 각 명령함수에서 수행
	{
		rfts->op_mode = OP_MODE::IDLE;
	}
}
#endif

#ifdef RFTS_USE_ECAT
bool RFTS::set_slave_id(int slv_id)
{
	slave_id = slv_id;
	
	return true;
}

void RFTS::callback_ecat_rx(void *me)
{
	RFTS *rfts = (RFTS *)me;
	ECAT_Master *ecat_if = (ECAT_Master *)rfts->m_RFTS_IF;
	int slv_id = rfts->slave_id;
	uint8_t *inputs = ecat_if->ecat_slave_stack[slv_id].inputs;

	short ft_raw[6];

	switch (rfts->protocol)
	{
	case PROTOCOL::EC02:
		rfts->mutex_response->lock();
		memcpy(rfts->response, inputs, 16);
		rfts->mutex_response->unlock();

		if (rfts->response[0] == (uint8_t) RFTS_COMMAND::FT_CONT)
		{
			rfts->op_mode = OP_MODE::FT_CONT;
			for (int axis = 0; axis < 6; axis++)
			{
				cvtBufferToInt16(ft_raw + axis, inputs + IDX_RAW_FT1 + axis * 2);
			}
			rfts->mutex_ft_data->lock();
			rfts->FT.entry[0] = (double)ft_raw[0] / rfts->force_divider; // fx
			rfts->FT.entry[1] = (double)ft_raw[1] / rfts->force_divider; // fy
			rfts->FT.entry[2] = (double)ft_raw[2] / rfts->force_divider; // fz
			rfts->FT.entry[3] = (double)ft_raw[3] / rfts->torque_divider; // tx
			rfts->FT.entry[4] = (double)ft_raw[4] / rfts->torque_divider; // ty
			rfts->FT.entry[5] = (double)ft_raw[5] / rfts->torque_divider; // tz
			
			rfts->overload_state.byte = rfts->response[13];
			// EC02 does not provide temprature info.

			// save to buffer
			RFTS_RX_FT_DATA ft_buf;
			ft_buf.FT = rfts->FT;
			ft_buf.OL_state = rfts->overload_state;
			ft_buf.temperature = rfts->temperature;
			rfts->FT_buffer.write(ft_buf);

			if(rfts->callback_on_FT_received)
			{	
				rfts->callback_on_FT_received(rfts->callbacked_object_on_FT_received, rfts->FT.entry);
			}
			rfts->mutex_ft_data->unlock();

		}
		else if (rfts->response[0] == (uint8_t) RFTS_COMMAND::CAP_CONT)
		{
			rfts->op_mode = OP_MODE::CAP_CONT;
		}
		else // 나머지 명령에 대한 회신 처리는 각 명령함수에서 수행
		{
			rfts->op_mode = OP_MODE::IDLE;
		}
		break;
	case PROTOCOL::ECAT_V1:
		if (rfts->op_mode == OP_MODE::FT_CONT)
		{
			float ft[6];
			
			rfts->mutex_ft_data->lock();
			memcpy(ft, inputs, 24);
			rfts->FT.entry[0] = (double)ft[0];
			rfts->FT.entry[1] = (double)ft[1];
			rfts->FT.entry[2] = (double)ft[2];
			rfts->FT.entry[3] = (double)ft[3];
			rfts->FT.entry[4] = (double)ft[4];
			rfts->FT.entry[5] = (double)ft[5];
			rfts->overload_state.byte = inputs[24];
			rfts->temperature = (double)(*((float *)&inputs[28]));

			// save to buffer
			RFTS_RX_FT_DATA ft_buf;
			ft_buf.FT = rfts->FT;
			ft_buf.OL_state = rfts->overload_state;
			ft_buf.temperature = rfts->temperature;
			rfts->FT_buffer.write(ft_buf);

			//callback
			if(rfts->callback_on_FT_received)
			{	
				rfts->callback_on_FT_received(rfts->callbacked_object_on_FT_received, rfts->FT.entry);
			}
			rfts->mutex_ft_data->unlock(); 
		}
		else
		{
			float cap[8];
			rfts->mutex_ft_data->lock();
			memcpy(cap, inputs, 32);
			rfts->Cap[0] = cap[0];
			rfts->Cap[1] = cap[1];
			rfts->Cap[2] = cap[2];
			rfts->Cap[3] = cap[3];
			rfts->Cap[4] = cap[4];
			rfts->Cap[5] = cap[5];
			rfts->Cap[6] = (double)((float)(*((uint32_t *)&cap[6])));
			rfts->Cap[7] = cap[7];

			// save to buffer
			RFTS_RX_CAP_DATA cap_data;
			for (int ch = 0; ch < 8;ch++)
				cap_data.Cap[ch] = rfts->Cap[ch];
			rfts->Cap_buffer.write(cap_data);

			// callback
			if(rfts->callback_on_Cap_received)
			{	
				rfts->callback_on_Cap_received(rfts->callbacked_object_on_Cap_received, rfts->Cap);
			}
			rfts->mutex_ft_data->unlock();
		}
		break;
	default:
		break;
	}
}

void RFTS::callback_ecat_tx(void *me)
{
	RFTS *rfts = (RFTS *)me;
	if (rfts->protocol == PROTOCOL::EC02)
	{
		ECAT_Master *ecat_if = (ECAT_Master *)rfts->m_RFTS_IF;
		int slv_id = rfts->slave_id;
		uint8_t *outputs = ecat_if->ecat_slave_stack[slv_id].outputs;

		rfts->mutex_command_send->lock();
		outputs[0] = (uint8_t) rfts->command;
		outputs[1] = rfts->cmd_parameter[0];
		outputs[2] = rfts->cmd_parameter[1];
		outputs[3] = rfts->cmd_parameter[2];

		outputs[7] = (uint8_t) rfts->command;
		outputs[6] = rfts->cmd_parameter[0];
		outputs[5] = rfts->cmd_parameter[1];
		outputs[4] = rfts->cmd_parameter[2];
		rfts->mutex_command_send->unlock();
	}
}
#endif

#ifdef RFTS_USE_CAN
bool RFTS::add_can_interface(void *rfts_if, uint8_t tx_id, uint8_t rx_id1, uint8_t rx_id2)
{
	if (is_value_in_range<uint8_t>(tx_id, 1, 255) &&
		is_value_in_range<uint8_t>(rx_id1, 1, 255) &&
		is_value_in_range<uint8_t>(rx_id2, 1, 255) &&
		(tx_id != rx_id1) && (tx_id != rx_id2) && (rx_id1 != rx_id2))
	{
		can_id.rx = tx_id;
		can_id.tx1 = rx_id1;
		can_id.tx2 = rx_id2;
	}
	else
	{
		return false;
	}

	protocol = PROTOCOL::CAN;
	m_RFTS_IF = rfts_if;
	rtx_type = RTX_TYPE::PACKET;

	can_id.rx = tx_id;
	can_id.tx1 = rx_id1;
	can_id.tx2 = rx_id2;

	((CAN_Interface *)rfts_if)->set_rx_callback((uint32_t)can_id.tx1, callback_can_rx, this);
	((CAN_Interface *)rfts_if)->set_rx_callback((uint32_t)can_id.tx2, callback_can_rx, this);
	((CAN_Interface *)rfts_if)->set_rx_callback((uint32_t)can_id.tx2 + 1, callback_can_rx, this);

	connected_to_interface = true;
	return true;
}
#endif

#ifdef RFTS_USE_UART
bool RFTS::add_uart_interface(void *rfts_if, PROTOCOL p)
{
	protocol = p;
	m_RFTS_IF = rfts_if;
	rtx_type = RTX_TYPE::PACKET;
	((UART_Interface *)rfts_if)->clear_rx_buffer();
	((UART_Interface *)rfts_if)->set_rx_callback(callback_uart_rx, this);

	connected_to_interface = true;
	return true;
}
#endif

#ifdef RFTS_USE_ECAT
bool RFTS::add_ecat_interface(void  *rfts_if, int slv_id)
{
	m_RFTS_IF = rfts_if;

	if (((ECAT_Master *)rfts_if)->ecat_slave_stack[slv_id].product_code == RFTS_EC02_PRODUCT_CODE)
	{
		protocol = RFTS::PROTOCOL::EC02;
		rtx_type = RFTS::RTX_TYPE::PACKET;
	}
	else if (((ECAT_Master *)rfts_if)->ecat_slave_stack[slv_id].product_code == RFTS_ECAT_V1_PRODUCT_CODE)
	{
		protocol = RFTS::PROTOCOL::ECAT_V1;
		rtx_type = RFTS::RTX_TYPE::ECAT_PDO;
		op_mode = RFTS::OP_MODE::FT_CONT;
	}
	else
	{
		return false;
	}
	slave_id = slv_id;
	((ECAT_Master *)rfts_if)->set_rx_decode_callback(slave_id, callback_ecat_rx, this);
	((ECAT_Master *)rfts_if)->set_tx_encode_callback(slave_id, callback_ecat_tx, this);

	connected_to_interface = true;
	return true;
}
#endif

bool RFTS::is_connected()
{
	return connected_to_interface;
}


void RFTS::clear_interface()
{
	if (m_RFTS_IF != NULL)
	{
		switch (protocol)
		{
#ifdef RFTS_USE_UART
		case PROTOCOL::UART:
			break;
#endif
#ifdef RFTS_USE_CAN
		case PROTOCOL::CAN:
			((CAN_Interface *)m_RFTS_IF)->clear_rx_callback(can_id.tx1);
			((CAN_Interface *)m_RFTS_IF)->clear_rx_callback(can_id.tx2);
			((CAN_Interface *)m_RFTS_IF)->clear_rx_callback(can_id.tx2 + 1);
			break;
#endif
#ifdef RFTS_USE_ECAT
		case PROTOCOL::EC02:
		case PROTOCOL::ECAT_V1:
			break;
#endif
		}
		m_RFTS_IF = NULL;
	}

	connected_to_interface = false;
}

// --------------------------------------------------------------------------------
// functions for vendor
// --------------------------------------------------------------------------------

#define SDO_IDX_VODO			(0x2FF0)
#define SDO_SUB_IDX_VODO		(1)
bool RFTS::activate_vendor_mode(void)  // ECAT_V1 only
{
	bool ret = false;

	if (protocol == PROTOCOL::ECAT_V1)
	{
#ifdef RFTS_USE_ECAT 
		uint16_t VODO_enable_key = 0x2FF0;  // enable VODO (Vendor-Only Data Object) 
		((ECAT_Master *)m_RFTS_IF)->writeSDO(slave_id, SDO_IDX_SENSOR_SETUP, SDO_SUB_IDX_BIAS, 2, &VODO_enable_key);  // send first enable-key through bias command
		VODO_enable_key = 0xAB1E;
		((ECAT_Master *)m_RFTS_IF)->writeSDO(slave_id, SDO_IDX_SENSOR_SETUP, SDO_SUB_IDX_BIAS, 2, &VODO_enable_key);  // send second enable-key through bias command
#endif
	}
	return ret;
}

void RFTS::get_Cap_values(double *cap)
{
	mutex_ft_data->lock();
	cap[0] = Cap[0];
	cap[1] = Cap[1];
	cap[2] = Cap[2];
	cap[3] = Cap[3];
	cap[4] = Cap[4];
	cap[5] = Cap[5];
	cap[6] = Cap[6];
	cap[7] = Cap[7];
	mutex_ft_data->unlock();
}

bool RFTS::set_product_name(string& pn)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::SET_PRODUCT_NAME;
		uint8_t command_pkt[8] = { (uint8_t)command, 'R', 'T', };
		uint8_t ext_pkt[16];

		int len = pn.size() < 16 ? pn.size() : 16;
		for (int i = 0; i < 16; i++)
		{
			if (i < len)
				ext_pkt[i] = pn[i];
			else
				ext_pkt[i] = 0;
		}

		if (send_command(command_pkt,ext_pkt,16))
		{
			delay_ms(100);
			product_name = pn;
			ret = true;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		const int p_size = 16;
		char cmd_and_data[p_size + 2];
		cmd_and_data[0] = 2;
		cmd_and_data[1] = 0;

		for (int i = 0; i < p_size-1; i++)
		{
			if (i < (int)pn.size()) 
				cmd_and_data[i + 2] = pn[i];
			else
				cmd_and_data[i + 2] = 0;
		}
		cmd_and_data[p_size+1] = 0; // (2+p_size-1)
		if (write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 2 + p_size, cmd_and_data))
		{
			product_name = pn;
			ret = true;
		}
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::set_serial_number(string& sn)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::SET_SERIAL_NUMBER;
		uint8_t command_pkt[8] = { (uint8_t)command, 'R', 'T', };
		uint8_t ext_pkt[16];

		int len = sn.size() < 16 ? sn.size() : 16;
		for (int i = 0; i < 16; i++)
		{
			if (i < len)
				ext_pkt[i] = sn[i];
			else
				ext_pkt[i] = 0;
		}

		if (send_command(command_pkt, ext_pkt, 16))
		{
			delay_ms(100);
			serial_number = sn;
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		const int p_size = 16;
		char cmd_and_data[p_size + 2];
		cmd_and_data[0] = 3;
		cmd_and_data[1] = 0;

		for (int i = 0; i < p_size - 1; i++)
		{
			if (i < (int)sn.size())
				cmd_and_data[i + 2] = sn[i];
			else
				cmd_and_data[i + 2] = 0;
		}
		cmd_and_data[p_size + 1] = 0;  // (2+p_size-1)
		if (write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 2 + p_size, cmd_and_data))
		{
			serial_number = sn;
			ret = true;
		}
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::start_Cap_out()
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::CAP_CONT;
		uint8_t command_pkt[8] = { (uint8_t)command,'R','T', };
		if (send_command(command_pkt))
		{
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		uint16_t cmd_and_data[2];
		cmd_and_data[0] = 1;	// command for Cap/FT out control
		cmd_and_data[1] = 1;	// Cap (1)
		op_mode = OP_MODE::CAP_CONT;
		ret = write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 4, cmd_and_data);
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::stop_Cap_out(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::CAP_CONT_STOP;
		uint8_t command_pkt[8] = { (uint8_t)command, 'R', 'T', };
		if (send_command(command_pkt))
		{
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		uint16_t cmd_and_data[2];
		cmd_and_data[0] = 1;	// command for Cap/FT out control
		cmd_and_data[1] = 0;	// Force (1)
		op_mode = OP_MODE::FT_CONT;
		ret = write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 4, cmd_and_data);
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::download_cal_data(float *caldata)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::SET_CAL_DATA;
		uint8_t command_pkt[8] = { (uint8_t)command, 'R', 'T', };
		const int p_size = 6 * 24 * 4;
		uint8_t ext_pkt[p_size];

		memcpy(ext_pkt, caldata, p_size);
		if (send_command(command_pkt, ext_pkt, p_size))
		{
			delay_ms(100);
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		const int p_size = 6 * 24 * 4;
		char cmd_and_data[p_size + 2];

		cmd_and_data[0] = 5;
		cmd_and_data[1] = 0;

		memcpy(&(cmd_and_data[2]), caldata, p_size);
		write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 2 + p_size, cmd_and_data);
		ret = true;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::set_temperature_comp_data(short *comp_data)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::SET_TEMP_COMP_DATA;
		uint8_t command_pkt[8] = { (uint8_t)command, 'R', 'T', };
		const int p_size = 6 * (2 + 1) * 2;		//  6 * ( drift_slope(2)+ stiffness_slop(1) ) * 2 bytes = 36

		if (send_command(command_pkt, (uint8_t*)comp_data, p_size))
		{
			delay_ms(100);
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		const int p_size = 6 *( 2 + 1) * 2;		//  6 * ( drift_slope(2)+ stiffness_slop(1) ) * 2 bytes = 36
		char cmd_and_data[2 + p_size];

		cmd_and_data[0] = 6;		// command
		cmd_and_data[1] = 0;

		memcpy(&(cmd_and_data[2]), comp_data, p_size);
		write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 2 + p_size, cmd_and_data);
		ret = true;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::set_factory_bias(bool on)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::SET_FACTORY_BIAS;
		uint8_t para;
		if (on)
			para = 1;
		else
			para = 0;
		uint8_t command_pkt[8] = { (uint8_t)command, 'R', 'T', para, };
		if (send_command(command_pkt))
		{
			delay_ms(100);
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		const int p_size = 1;
		char cmd_and_data[p_size + 2];
		cmd_and_data[0] = 4;
		cmd_and_data[1] = 0;
		if (on)
			cmd_and_data[2] = 1;
		else
			cmd_and_data[2] = 0;

		write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 2 + p_size, cmd_and_data);
		ret = true;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::clear_overload_counter(void)
{
	bool ret = false;

	if (rtx_type == RTX_TYPE::PACKET)
	{
		command = RFTS_COMMAND::CLEAR_OVERLOAD_CNT;

		uint8_t command_pkt[8] = { (uint8_t)command, 'R', 'T', };
		if (send_command(command_pkt))
		{
			delay_ms(100);
			ret = true;
		}
		else
		{
			ret = false;
		}
	}
	else if (rtx_type == RTX_TYPE::ECAT_PDO)
	{
		const int p_size = 0;
		char cmd_and_data[2 + p_size];

		cmd_and_data[0] = 10;
		cmd_and_data[1] = 0;

		write_ecat_SDO(slave_id, SDO_IDX_VODO, SDO_SUB_IDX_VODO, 2 + p_size, cmd_and_data);
		ret = true;
	}
	else
	{
		error_msg = "Invaild RTX Type";
	}

	return ret;
}
bool RFTS::set_hysteresis_comp_rate(float h_rate)
{
	string sn_backup = serial_number;
	
	char buf[20];

	sprintf(buf, "hyst%6.4f", h_rate);
	string str(buf);
	bool ret = set_serial_number(str);

	serial_number = sn_backup;
	return ret;
}

bool RFTS::get_hysteresis_comp_rate(float& h_rate)
{
	string sn_backup = serial_number;

	string str("hyst_");
	if (set_serial_number(str))
	{
		h_rate = (float)response_error_code / 10000.0;
		serial_number = sn_backup;
		return true;
	}
	else
	{
		h_rate = 0.0;
		serial_number = sn_backup;
		return false;
	}
}


// For ECAT interface
bool RFTS::read_ecat_SDO(uint16_t slave_id, uint16_t idx, uint8_t sub_idx, void *data)
{
#ifdef RFTS_USE_ECAT
	return ((ECAT_Master *)m_RFTS_IF)->readSDO(slave_id, idx, sub_idx, data);
#else
	return false;
#endif
}

bool RFTS::write_ecat_SDO(uint16_t slave_id, uint16_t idx, uint8_t sub_idx, int p_size, void *data)
{
#ifdef RFTS_USE_ECAT
	return ((ECAT_Master *)m_RFTS_IF)->writeSDO(slave_id, idx, sub_idx, p_size, data);
#else
	return false;
#endif
}

void cvtBufferToInt16(short *value, unsigned char *buff)
{
	unsigned char *temp = (unsigned char *)value;
	temp[0] = buff[0];
	temp[1] = buff[1];
}

//-----------------------------------------------------------------------------------------
// RFTS SET - Mutiple Sensor on the Multiple Communication 
//-----------------------------------------------------------------------------------------

RFTS_SET::RFTS_SET()
{
}

RFTS_SET::~RFTS_SET()
{
	clear();
}
void RFTS_SET::clear()
{
	for (auto& rfts : rfts_stack)
		SafeDelete(&rfts);
	rfts_stack.clear();
}
int RFTS_SET::size()
{
	return (int)rfts_stack.size();
}
RFTS* RFTS_SET::at(int idx)
{
	if (is_value_in_range(idx, 0, size() - 1))
		return rfts_stack[idx];
	else
		return NULL;
}

#ifdef RFTS_USE_ECAT
void RFTS_SET::attach_all_rfts_on_ecat(ECAT_Master& ecat_if)
{
	// first, find all rfts on ecat
	for (int slave = 1; slave <= ecat_if.num_of_slave; slave++)
	{
		// check vendor-id
		if (ecat_if.ecat_slave_stack[slave].vendor_id != ROBOTOUS_VENDER_ID)
			continue;  // Not Robotous Product
		int slave_id;
		RFTS::PROTOCOL protocol;
		RFTS::RTX_TYPE rtx_type;
		RFTS::OP_MODE op_mode;
		// check product code
		if (ecat_if.ecat_slave_stack[slave].product_code == RFTS_EC02_PRODUCT_CODE)
		{
			protocol = RFTS::PROTOCOL::EC02;
			rtx_type = RFTS::RTX_TYPE::PACKET;
			slave_id = slave;
		}
		else if (ecat_if.ecat_slave_stack[slave].product_code == RFTS_ECAT_V1_PRODUCT_CODE)
		{
			protocol = RFTS::PROTOCOL::ECAT_V1;
			rtx_type = RFTS::RTX_TYPE::ECAT_PDO;
			slave_id = slave;
			op_mode = RFTS::OP_MODE::FT_CONT;
		}
		else
		{
			continue;
		}
		RFTS *null_rfts;
		rfts_stack.push_back(null_rfts);
		rfts_stack.back() = new RFTS;
		rfts_stack.back()->protocol = protocol;
		rfts_stack.back()->rtx_type = rtx_type;
		rfts_stack.back()->set_slave_id(slave_id);

		if (rfts_stack.back()->protocol == RFTS::PROTOCOL::ECAT_V1)
			rfts_stack.back()->op_mode = op_mode;
		rfts_stack.back()->add_ecat_interface(&ecat_if, slave_id);
	}
}
#endif
void RFTS_SET::update_info()
{
	for (auto& rfts : rfts_stack)
		rfts->update_sensor_info();
}



////-----------------------------------------------------------------------------------------
//// RFTS Interface utility functions
////-----------------------------------------------------------------------------------------
//
//
//#ifndef NO_RFTS_ECAT_IF
//
//
//
//void find_RFTS_on_ecat(ECAT_Master& ecat_if, RFTS_STACK& rfts_stack)
//{
//	for (int slave = 1; slave <= ecat_if.num_of_slave; slave++)
//	{
//		RFTS rfts;
//
//		// check vendor-id
//		if (ecat_if.ecat_slave_stack[slave].vendor_id != ROBOTOUS_VENDER_ID)
//			continue;  // Not Robotous Product
//		// check product code
//		if (ecat_if.ecat_slave_stack[slave].product_code == RFTS_EC02_PRODUCT_CODE)
//		{
//			rfts.protocol = RFTS::PROTOCOL::EC02;
//			rfts.rtx_type = RFTS::RTX_TYPE::PACKET;
//			rfts.slave_id = slave;
//		}
//		else if (ecat_if.ecat_slave_stack[slave].product_code == RFTS_ECAT_V1_PRODUCT_CODE)
//		{
//			rfts.protocol = RFTS::PROTOCOL::ECAT_V1;
//			rfts.rtx_type = RFTS::RTX_TYPE::ECAT_PDO;
//			rfts.slave_id = slave;
//
//			rfts.op_mode = RFTS::OP_MODE::FT_CONT;
//		}
//		else
//		{
//			continue;
//		}
//		rfts_stack.push_back(rfts);
//	}
//}
//// 사실, 인터페이스를 붙이는 작업은 센서를 찾으면서 수행하는 것이 좋지만,
//// 2개이상의 rfts를 스택에 추가할때, 
//// vector 스택에 추가 할때, 생성후 추가하기 때문에, 
//// 생성후 바로 인터페이스를 붙이고, 스택에 추가한 후, 
//// 생성시 만들어진 rfts 객체가 사라지기 때문에, 
//// 생성시 만들어진 객체의 콜백함수도 사라진다. 
//// 추후 마스터에서 콜백함수를 부르면 해당영역에 콜백함수가 없기 때문에 런타임에러가 발생
//// 이런 이유로 인터페이스 연결은 스택에 모든 rfts 객체가 담긴 후 수행해야 한다.
//
//void attach_ecat_interface(ECAT_Master& ecat_if, RFTS_STACK& rfts_stack)
//{
//	// enable callbacks of the interface
//
//	for (auto& rfts : rfts_stack)
//	{
//		if ((rfts.protocol == RFTS::PROTOCOL::EC02) || (rfts.protocol == RFTS::PROTOCOL::ECAT_V1))
//		{
//			rfts.add_interface(&ecat_if);
//		}
//	}
//}
//#endif
//
//// 당연한 이야기지만, 연결된 인터페이스 기동 후 수행해야 동작한다.
//void update_info_of_all_RFTS(RFTS_STACK& rfts_stack)
//{
//	for (auto& rfts : rfts_stack)
//		rfts.update_sensor_info();
//}
//



// END OF FILE

