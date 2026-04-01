
// RFT_IF_ECATDlg.h : 헤더 파일
//

#pragma once

#include "afxwin.h"

////////////////////////////////////////////////////////////////////////////////
#include "RFTS-V1.0.h"


// for graph
#include <vector>
#include <mutex>
using namespace std;
#include "RT_Graph.h"

// 
#include "RT_UserInterface.h"

// for hardware interface
#include "ECAT_Master.h"
#include "RT_UserInterface.h"


// CRFT_IF_ECATDlg 대화 상자
class CRFTS_InterfaceDlg : public CDialogEx
{
// 생성입니다.
public:
	CRFTS_InterfaceDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
	enum { IDD = IDD_RFTS_INTERFACE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

public:

	//////////////////////////////////////////////////////////////////////////
	// FOR GRAPH
	RT_GRAPH_ARRAY mGA;
	LINE_SERIES *mLS[6];
	float time_to_display = 5.0f;

	//////////////////////////////////////////////////////////////////////////
	// FOR RFT
	
	// Sensor Object
	RFTS_SET m_rfts_set;
	const static int buf_size = 5000;

	// Interface
	ECAT_Master m_ecat_interface;
	UINT run_cycle = 1; // ms : 시간으로 정의

	static void callback_RFT_Data_Receive(void *object_callbacked, double *ft);

	long callback_cnt;

	//////////////////////////////////////////////////////////////////////////
	// For FT Data Saving and Display
	typedef struct {
		double time ;
		double FT[6] ;
	} FTS_DATA;

	bool logging = false;
	vector <FTS_DATA> measured_data;
	mutex mtx_FTS_data;

// 구현입니다.
protected:
	RTUI_FONT_SET mFontSet;
	void setup_font(void);
	void disp_rfts_info(void);
	void disp_FT(void);
	void disp_setup_info(void);


	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
	int mRFTS_ID;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CComboBox m_nic_info;
	afx_msg void OnBnClickedCheckInterfaceOpen();
	afx_msg void OnBnClickedCheckBias();
	afx_msg void OnBnClickedCheckFtOutCont();
	afx_msg void OnBnClickedReadOverloadCount();
	CComboBox m_combo_Filter_Cutoff_Frq;
	afx_msg void OnBnClickedButtonFilterSetting();
	afx_msg void OnBnClickedCheckDataLogging();

	afx_msg void OnBnClickedOk();
	CComboBox m_combo_ecat_cycle;
	afx_msg void OnBnClickedButtonDispSavedData();
	RTUI_LIST m_list_rfts_info;
	double m_disp_F_min;
	double m_disp_F_max;
	double m_disp_T_min;
	double m_disp_T_max;
	afx_msg void OnClickedButtonSetFixedRangeF();
	afx_msg void OnClickedButtonSetFixedRangeT();
	BOOL m_auto_scale_F;
	BOOL m_auto_scale_T;
	afx_msg void OnClickedCheckAutoScaleF();
	afx_msg void OnClickedCheckAutoScaleT();
};
