#ifndef __ECAT_MASTER_H__
#define __ECAT_MASTER_H__

#include "afxwin.h"
#include <Mmsystem.h> 

#include <string>
#include <vector>
#include <mutex>
#include "stdint.h"

using namespace std;

typedef struct
{
	string name;
	string description;
}NIC_ENTRY;
typedef vector<NIC_ENTRY>	NIC_STACK;

typedef void(*ECAT_CALLBACK_FUNC) (void *object_callbacked);
typedef struct
{
	ECAT_CALLBACK_FUNC callback;		// callback function
	void *object_callbacked;			// object which is callbacked
}ECAT_CALLBACK_ENTRY;
typedef vector<ECAT_CALLBACK_ENTRY> ECAT_CALLBACK_STACK;

typedef struct
{
	/* Slave Information and Data -------------------------------*/
	string name;			// slave name
	uint32_t vendor_id;		
	uint32_t product_code;
	uint32_t revision;

	/* Slave PDO -----------------------------------------------*/
	uint8_t *inputs;	// pointer of input PDO
	uint8_t *outputs;	// pointer of ouput PDO

	/* Slave Callbacks -----------------------------------------*/
	ECAT_CALLBACK_ENTRY RxPDO_decode;
	ECAT_CALLBACK_ENTRY TxPDO_encode;

}ECAT_SLAVE; 
typedef vector <ECAT_SLAVE>  ECAT_SLAVE_STACK; // the first entry is for Master, next ones for Slave

class ECAT_Master
{
public:
	ECAT_Master();
	~ECAT_Master();

	bool find_usable_NIC(void);
	NIC_STACK usable_NICs;

	// ecat slave stack (vector) for using slave data outside of this class object like callbacks 
	// note - first entry(ecat_slave[0]) contains master data (i.e. first slave is not ecat_slave[0], but ecat_slave[1])
	ECAT_SLAVE_STACK ecat_slave_stack;

	// start ethercat master. rtx_period in mili-second (default = 1ms) 
	bool start(string NIC_Name, UINT rtx_period = 1);
	bool stop(void);

	// enable interface by enabling callback
	void enable(BOOL on = true);

	// set callback 
	void set_rx_decode_callback(int slv_id, ECAT_CALLBACK_FUNC cb_rx_decode, void *object_callbacked);  // set callback function for decoding rx data of the specified slave
	void set_tx_encode_callback(int slv_id, ECAT_CALLBACK_FUNC cb_tx_encode, void *object_callbacked);  // set callback function for encoding tx data of the specified slave

	void add_callback_before_PDO_decoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked);
	void add_callback_after_PDO_decoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked);
	void add_callback_before_PDO_encoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked);
	void add_callback_after_PDO_encoding(ECAT_CALLBACK_FUNC callback, void *object_callbacked);

	void clear_all_callbacks(void);  // clear all additional callbacks except en/decoding callbacks

	// wrapping functions of the native SOEM functions
	bool readSDO(uint16_t slave_id, uint16_t idx, uint8_t sub_idx, void *data);
	bool writeSDO(uint16_t slave_id, uint16_t idx, uint8_t sub_idx, int p_size, void *data);

	int num_of_slave; // 연결된 슬래이브 센서의 숫자
	string last_err_msg;

private:
	// for thread
	static void ecat_check_thread(ECAT_Master *pThis);
	void   ecat_check_worker(void);
	bool   ecat_check_thread_flag;  // thread를 죽이기 위해 사용

	static void CALLBACK PDO_rtx_thread(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
	void PDO_rtx_thread_worker(void);

	void initialize_slave_stack(void);
	UINT PDO_rtx_period;	// in mili-second;

protected: 
	virtual void interpret_input_PDO(void); //if not use virtual, the overridden function does not work, because these functions used is used in this class
	virtual void build_output_PDO(void);	//if not use virtual, the overridden function does not work, because these functions used is used in this class

// for callback
private:
	BOOL callback_enabled;
	mutex mutex_callback;

	ECAT_CALLBACK_STACK callbacks_before_PDO_decoding;		// callback stack that run before rxPDO decoding
	ECAT_CALLBACK_STACK callbacks_after_PDO_decoding;		// callback stack that run after rxPDO decoding
	ECAT_CALLBACK_STACK callbacks_before_PDO_encoding;		// callback stack that run before txPDO encoding
	ECAT_CALLBACK_STACK callbacks_after_PDO_encoding;		// callback stack that run after txPDO encoding
	void enable_callbacks(BOOL on = true);

public:
	
};

#endif // __ECAT_MASTER_H__
