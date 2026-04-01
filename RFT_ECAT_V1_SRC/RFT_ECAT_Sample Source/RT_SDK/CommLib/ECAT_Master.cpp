#include "stdafx.h"
#ifndef delay_ms
#define delay_ms(x) SleepEx(x, FALSE)
#endif

#include "ECAT_Master.h" 
// For EtherCAT : SOEM
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

// Grobal Varialbles
// for soem
bool in_OP;					// for requesting initilize operation to slave
int expected_WKC;			// 초기화시 work counter
int WKC;					// 확인된 work counter
int pdo_rtx_cnt;			// read thread counter
char io_map[4096];
unsigned char current_group;
UINT timer_event_id = 0;	// timer event id

ECAT_Master::ECAT_Master()
{
	ecat_check_thread_flag = false;
	callback_enabled = false;

	in_OP = false;
	expected_WKC = 0;
	current_group = 0;

	WKC = 0;
	pdo_rtx_cnt = 0;
	find_usable_NIC();
}

ECAT_Master::~ECAT_Master()
{
	stop();
}

bool ECAT_Master::find_usable_NIC(void)
{
	ec_adapter *adapter = NULL;
	ec_adapter *adapter_old = NULL;

	adapter = ec_find_adapters();
	usable_NICs.clear();

	while (adapter != NULL)
	{
		NIC_ENTRY nic;
		nic.description = adapter->desc;
		nic.name		= adapter->name;

		usable_NICs.push_back(nic);

		adapter_old = adapter;
		adapter = adapter->next;
		delete adapter_old;
	}
	delete adapter;
	
	return true;
}

bool ECAT_Master::start(string NIC_Name, UINT rtx_period)
{
	bool ret = false;
	int chk;

	// initialize slave stack & callback stack
	callback_enabled = false;
	ecat_slave_stack.clear(); // at the first, clear slave stack 
	clear_all_callbacks();

	if (ec_init((char *)NIC_Name.c_str()))  // setup NIC
	{
		if (ec_config_init(FALSE) > 0)
		{
			// ETHERCAT CHECK THREAD
			ecat_check_thread_flag = true;
			// Start thread for checking and recovering EtherCAT communication
			DWORD thread_id;
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ecat_check_thread, (LPVOID)this, 0, &thread_id);
			ec_config_map(&io_map);
			ec_configdc();

			/* wait for all slaves to reach SAFE_OP state */
			ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

			expected_WKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
			num_of_slave = ec_group[0].inputsWKC;

			ec_slave[0].state = EC_STATE_OPERATIONAL;
		 
			/* send one valid process data to make outputs in slaves happy*/
			ec_send_processdata();
			ec_receive_processdata(EC_TIMEOUTRET);

			/* request OP state for all slaves */
			ec_writestate(0);
			chk = 40;
			/* wait for all slaves to reach OP state */
			do
			{
				ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
			} while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

			if (ec_slave[0].state == EC_STATE_OPERATIONAL)  // Success to run master
			{
				// get master(slave_id = 0) and slave data pointer
				for (int slv_id = 0; slv_id <= num_of_slave; slv_id++)
				{
					// build slave and get data 
					ECAT_SLAVE slave;
					slave.inputs = ec_slave[slv_id].inputs;
					slave.outputs = ec_slave[slv_id].outputs;
					
					slave.name = ec_slave[slv_id].name;
					slave.vendor_id = ec_slave[slv_id].eep_man;
					slave.product_code = ec_slave[slv_id].eep_id;
					slave.revision = ec_slave[slv_id].eep_rev;

					// add an null callback entry for PDO processing
					slave.RxPDO_decode = { NULL, NULL };
					slave.TxPDO_encode = { NULL, NULL };
	
					// add slave to the stack;
					ecat_slave_stack.push_back(slave);
				}
	
				/* start RT thread as periodic MM timer */
				PDO_rtx_period = rtx_period;
				timer_event_id = timeSetEvent(PDO_rtx_period, 0, PDO_rtx_thread, (DWORD_PTR)this, TIME_PERIODIC);

				in_OP = true;
				ret = true;
			}
			else
			{
				last_err_msg = "Not all slaves reached operational state.";
				stop();
				ret = false;
			}
		}
		else
		{
			ec_close();
			last_err_msg = "There is no ECAT slave.";
			ret = false;
		}
	}
	else
	{
		last_err_msg = "No socket connection on " + NIC_Name;
		ret = false;
	}

	if (!ret)
		PDO_rtx_period = 0;
	return ret;
}

bool ECAT_Master::stop(void)
{
	// kill 'ecat_check_thread'
	enable_callbacks(false);
	clear_all_callbacks();
	ecat_check_thread_flag = false;
	// check thread가 종료될때까지 기다린다.
	delay_ms(100);

	// kill 'PDO_rtx_thread'
	if (timer_event_id)
	{
		timeKillEvent(timer_event_id);
		timer_event_id = 0;
	}

	if (in_OP)
	{
		in_OP = false;
		ec_slave[0].state = EC_STATE_INIT; // 0 is master
		ec_writestate(0);
		// wait for state: EC_STATE_INIT
		int chk = 40; // 40 x 50000us = 2 sec
		do
		{
			ec_statecheck(0, EC_STATE_INIT, 50000); // 50000 us
		} while (chk-- && (ec_slave[0].state != EC_STATE_INIT));
		if (chk == 0)
		{
			last_err_msg = "ECAT Master Stop Failed: Timeout Error";
			return false;
		}
		ec_close();
	}
	return true;
}

void ECAT_Master::ecat_check_thread(ECAT_Master *pThis)
{
	pThis->ecat_check_worker();
}

void ECAT_Master::ecat_check_worker(void)
{
	int slave;

	while (ecat_check_thread_flag)
	{

		if (in_OP && ((WKC < expected_WKC) || ec_group[current_group].docheckstate))
		{

			/* one or more slaves are not responding */
			ec_group[current_group].docheckstate = FALSE;
			ec_readstate();

			for (slave = 1; slave <= ec_slavecount; slave++)
			{
				if ((ec_slave[slave].group == current_group) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
				{
					ec_group[current_group].docheckstate = TRUE;
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
		delay_ms(10000);
	}
}

void ECAT_Master::PDO_rtx_thread(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	ECAT_Master *pThis = (ECAT_Master*)dwUser;
	pThis->PDO_rtx_thread_worker();
}
void ECAT_Master::PDO_rtx_thread_worker(void)
{
	build_output_PDO();
	ec_send_processdata();
	WKC = ec_receive_processdata(EC_TIMEOUTRET);
	interpret_input_PDO();
	pdo_rtx_cnt++;
}

void ECAT_Master::interpret_input_PDO(void)
{
	if (callback_enabled)
	{
		mutex_callback.lock();
	
		// run post-decoding process
		for (auto& cb_entry : callbacks_before_PDO_decoding)
		{
			if (cb_entry.callback != NULL)
				cb_entry.callback(cb_entry.object_callbacked);
		}
		// run decoding process
		for (auto& slave : ecat_slave_stack)
		{
			if (slave.RxPDO_decode.callback != NULL)
				slave.RxPDO_decode.callback(slave.RxPDO_decode.object_callbacked);
		}
		// run post-decoding process
		for (auto& cb_entry : callbacks_after_PDO_decoding)
		{
			if (cb_entry.callback != NULL)
				cb_entry.callback(cb_entry.object_callbacked);
		}
		mutex_callback.unlock();
	}
}

void ECAT_Master::build_output_PDO(void)
{
	if (callback_enabled)
	{
		mutex_callback.lock();

		// run pre-encoding process
		for (auto& cb_entry : callbacks_before_PDO_encoding)
		{
			if (cb_entry.callback != NULL)
				cb_entry.callback(cb_entry.object_callbacked);
		}
		// run encoding process
		for (auto& slave : ecat_slave_stack)
		{
			if (slave.TxPDO_encode.callback != NULL)
				slave.TxPDO_encode.callback(slave.TxPDO_encode.object_callbacked);
		}
		// run post-encoding process
		for (auto& cb_entry : callbacks_after_PDO_encoding)
		{
			if (cb_entry.callback != NULL)
				cb_entry.callback(cb_entry.object_callbacked);
		}
		mutex_callback.unlock();
	}
}
void ECAT_Master::enable_callbacks(BOOL on)
{
	callback_enabled = on;
}
void ECAT_Master::set_rx_decode_callback(int slv_id, ECAT_CALLBACK_FUNC cb_rx_decode, void *object_callbacked)
{
	mutex_callback.lock();
	ecat_slave_stack[slv_id].RxPDO_decode.callback = cb_rx_decode;
	ecat_slave_stack[slv_id].RxPDO_decode.object_callbacked = object_callbacked;
	mutex_callback.unlock();
}
void ECAT_Master::set_tx_encode_callback(int slv_id, ECAT_CALLBACK_FUNC cb_tx_decode, void *object_callbacked)
{
	mutex_callback.lock();
	ecat_slave_stack[slv_id].TxPDO_encode.callback = cb_tx_decode;
	ecat_slave_stack[slv_id].TxPDO_encode.object_callbacked = object_callbacked;
	mutex_callback.unlock();
}
void ECAT_Master::add_callback_before_PDO_decoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked)
{
	ECAT_CALLBACK_ENTRY cb_entry = { callback, object_callbacked };

	mutex_callback.lock();
	callbacks_before_PDO_decoding.push_back(cb_entry);
	mutex_callback.unlock();
}
void ECAT_Master::add_callback_after_PDO_decoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked)
{
	ECAT_CALLBACK_ENTRY cb_entry = { callback, object_callbacked };

	mutex_callback.lock();
	callbacks_after_PDO_decoding.push_back(cb_entry);
	mutex_callback.unlock();
}
void ECAT_Master::add_callback_before_PDO_encoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked)
{
	ECAT_CALLBACK_ENTRY cb_entry = { callback, object_callbacked };

	mutex_callback.lock();
	callbacks_before_PDO_encoding.push_back(cb_entry);
	mutex_callback.unlock();
}
void ECAT_Master::add_callback_after_PDO_encoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked)
{
	ECAT_CALLBACK_ENTRY cb_entry = { callback, object_callbacked };

	mutex_callback.lock();
	callbacks_after_PDO_encoding.push_back(cb_entry);
	mutex_callback.unlock();
}

void ECAT_Master::enable(BOOL on)
{
	enable_callbacks(on);
}

void ECAT_Master::clear_all_callbacks(void)
{
	mutex_callback.lock();
	callbacks_before_PDO_encoding.clear();
	callbacks_after_PDO_encoding.clear();
	callbacks_before_PDO_decoding.clear();
	callbacks_after_PDO_decoding.clear();
	mutex_callback.unlock();
}

bool ECAT_Master::readSDO(uint16_t slave_id,uint16_t idx, uint8_t sub_idx, void *data)
{
	int rcvd_size = 20;
	ec_SDOread(slave_id, idx, sub_idx, FALSE, &rcvd_size, data, EC_TIMEOUTRXM);
	return true;
}

bool ECAT_Master::writeSDO(uint16_t slave_id, uint16_t idx, uint8_t sub_idx, int p_size, void *data)
{
	ec_SDOwrite(slave_id, idx, sub_idx, FALSE, p_size, data, EC_TIMEOUTRXM);
	return true;
}