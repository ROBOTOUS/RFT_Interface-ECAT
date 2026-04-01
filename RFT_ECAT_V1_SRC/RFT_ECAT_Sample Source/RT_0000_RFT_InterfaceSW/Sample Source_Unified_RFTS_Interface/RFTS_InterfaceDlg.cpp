
// RFT_IF_ECATDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "RFTS_Interface.h"
#include "RFTS_InterfaceDlg.h"
#include "afxdialogex.h"

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

LARGE_INTEGER g_st, g_ed, g_freq;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_PERIOD 100

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRFT_IF_ECATDlg 대화 상자



CRFTS_InterfaceDlg::CRFTS_InterfaceDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CRFTS_InterfaceDlg::IDD, pParent)
	, mRFTS_ID(0) // 멤버 변수 초기화
	, m_disp_F_min(0)
	, m_disp_F_max(0)
	, m_disp_T_min(0)
	, m_disp_T_max(0)
	, m_auto_scale_F(FALSE)
	, m_auto_scale_T(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRFTS_InterfaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_NIC_INFO, m_nic_info);
	DDX_Control(pDX, IDC_FT_GRAPH, mGA);
	//  DDX_Control(pDX, IDC_CUSTOM_CHART_MOMENT, m_ChartCtrl_Torque); //
	DDX_Control(pDX, IDC_COMBO_FILTER_TYPE, m_combo_Filter_Cutoff_Frq);
	DDX_Control(pDX, IDC_COMBO_ECAT_CYCLE, m_combo_ecat_cycle);
	DDX_Control(pDX, IDC_RFTS_INFO, m_list_rfts_info);
	DDX_Text(pDX, IDC_EDIT_DISP_F_MIN, m_disp_F_min);
	DDX_Text(pDX, IDC_EDIT_DISP_F_MAX, m_disp_F_max);
	DDX_Text(pDX, IDC_EDIT_DISP_T_MIN, m_disp_T_min);
	DDX_Text(pDX, IDC_EDIT_DISP_T_MAX, m_disp_T_max);
	DDX_Check(pDX, IDC_CHECK_AUTO_SCALE_F, m_auto_scale_F);
	DDX_Check(pDX, IDC_CHECK_AUTO_SCALE_T, m_auto_scale_T);
}

BEGIN_MESSAGE_MAP(CRFTS_InterfaceDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK_INTERFACE_OPEN, &CRFTS_InterfaceDlg::OnBnClickedCheckInterfaceOpen)
	ON_BN_CLICKED(IDC_CHECK_BIAS, &CRFTS_InterfaceDlg::OnBnClickedCheckBias)
	ON_BN_CLICKED(IDC_CHECK_FT_OUT_CONT, &CRFTS_InterfaceDlg::OnBnClickedCheckFtOutCont)
	ON_BN_CLICKED(IDC_READ_OVERLOAD_COUNT, &CRFTS_InterfaceDlg::OnBnClickedReadOverloadCount)
	ON_BN_CLICKED(IDC_BUTTON_FILTER_SETTING, &CRFTS_InterfaceDlg::OnBnClickedButtonFilterSetting)
	ON_BN_CLICKED(IDC_CHECK_DATA_LOGGING, &CRFTS_InterfaceDlg::OnBnClickedCheckDataLogging)
	ON_BN_CLICKED(IDOK, &CRFTS_InterfaceDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_DISP_SAVED_DATA, &CRFTS_InterfaceDlg::OnBnClickedButtonDispSavedData)

	ON_BN_CLICKED(IDC_BUTTON_SET_F_RANGE, &CRFTS_InterfaceDlg::OnClickedButtonSetFixedRangeF)
	ON_BN_CLICKED(IDC_BUTTON_SET_T_RANGE, &CRFTS_InterfaceDlg::OnClickedButtonSetFixedRangeT)

	ON_BN_CLICKED(IDC_CHECK_AUTO_SCALE_F, &CRFTS_InterfaceDlg::OnClickedCheckAutoScaleF)
	ON_BN_CLICKED(IDC_CHECK_AUTO_SCALE_T, &CRFTS_InterfaceDlg::OnClickedCheckAutoScaleT)
END_MESSAGE_MAP()


// CRFT_IF_ECATDlg 메시지 처리기

BOOL CRFTS_InterfaceDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.
	
	
	///////////////////////////////////////////////////////////////////////////////
	// Process priority setting
	if (::SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) // 본 프로그램을 최고 프라이어티 클래스로...
	{
		//CONSOLE_S(CONSOLE_RED, "\n======== Priority setting OK.. =========\n");
	}
	// priority 설정이 잘못 되었더라도... 
	timeBeginPeriod(1); // increase time resolution



	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			
	SetIcon(m_hIcon, FALSE);		

	// Font Setup
	setup_font();

	// RFTS Info List
	m_list_rfts_info.setup_size(6, 6);
	m_list_rfts_info.setup_default_font(mFontSet.f_normal);
	m_list_rfts_info.setup_default_line_color(RT_DARK_GREY);
	m_list_rfts_info.setup_border(2, RT_BLACK);
	m_list_rfts_info.setup_row_seperator(1, 2, RT_BLACK);
	m_list_rfts_info.setup_row_seperator(2, 2, RT_BLACK);
	m_list_rfts_info.setup_row_seperator(4, 2, RT_BLACK);
	m_list_rfts_info.setup_row_space_weight(3, 0.7);

	//m_list_rfts_info.setup_row_seperator(2, 2, RT_BLACK);
	//m_list_rfts_info.setup_row_seperator(7, 2, RT_BLACK);
	//m_list_rfts_info.setup_row_seperator(8, 2);
	//m_list_rfts_info.setup_col_seperator(0, 2);
	//m_list_rfts_info.setup_col_space_weight(0, 1.8);
	//m_list_rfts_info.set_emphasizing_rect_property(1);

	m_list_rfts_info.init();
	m_list_rfts_info.set_font_row(0, mFontSet.f_normal_bold);
	m_list_rfts_info.set_font_row(1, mFontSet.f_normal_bold);
	m_list_rfts_info.set_font_row(2, mFontSet.f_normal_bold);
	m_list_rfts_info.set_font_row(3, mFontSet.f_small_bold);
	m_list_rfts_info.set_font_row(4, mFontSet.f_normal_bold);
	m_list_rfts_info.set_font_row(5, mFontSet.f_normal_bold);

	m_list_rfts_info.merge_cells(0, 0, 0, 1);
	m_list_rfts_info.merge_cells(0, 2, 0, 3);
	m_list_rfts_info.merge_cells(0, 4, 0, 5);
	m_list_rfts_info.merge_cells(1, 0, 1, 1);
	m_list_rfts_info.merge_cells(1, 2, 1, 3);
	m_list_rfts_info.merge_cells(1, 4, 1, 5);

	
	m_list_rfts_info.set_text(0, 0, _T("Model Name"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(0, 2, _T("Serial Number"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(0, 4, _T("F/W Version"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(1, 0, _T("--"));
	m_list_rfts_info.set_text(1, 2, _T("--"));
	m_list_rfts_info.set_text(1, 4, _T("--"));

	m_list_rfts_info.merge_cells(2, 0, 2, 2);
	m_list_rfts_info.merge_cells(2, 3, 2, 5);
	m_list_rfts_info.set_text(2, 0, _T("Force(N)"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(2, 3, _T("Torque(Nm)"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(3, 0, _T("Fx"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(3, 1, _T("Fy"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(3, 2, _T("Fz"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(3, 3, _T("Tx"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(3, 4, _T("Ty"), RT_BLACK, RT_LIGHT_GREY);
	m_list_rfts_info.set_text(3, 5, _T("Tz"), RT_BLACK, RT_LIGHT_GREY);

	m_list_rfts_info.merge_cells(5, 0, 5, 5);
	m_list_rfts_info.set_text(5, 0, _T("---"));

	m_list_rfts_info.update();


	// Show and build combo NIC info
	int num_of_NIC = m_ecat_interface.usable_NICs.size();
	for (int i = 0; i < num_of_NIC; i++)
	{
		CString desc(m_ecat_interface.usable_NICs[i].description.c_str());
//		CString name(m_ecat_interface.usable_NICs[i].name.c_str());
//		CString data = desc + ": " + name;
		m_nic_info.InsertString(i, desc.GetBuffer());
	}
	m_nic_info.SetCurSel(0);

	// build combo for ecat run cycle
	m_combo_ecat_cycle.InsertString(0, _T("1ms (1,000 samples/s)"));
	m_combo_ecat_cycle.InsertString(1, _T("2ms (500 samples/s)"));
	m_combo_ecat_cycle.InsertString(2, _T("3ms (333 samples/s)"));
	m_combo_ecat_cycle.InsertString(3, _T("4ms (250 samples/s)"));
	m_combo_ecat_cycle.InsertString(4, _T("5ms (200 samples/s)"));
	m_combo_ecat_cycle.InsertString(5, _T("10ms (100 samples/s)"));
	m_combo_ecat_cycle.SetCurSel(4);


	// build combo for filter setting
	m_combo_Filter_Cutoff_Frq.InsertString(0, _T("No filter"));
	m_combo_Filter_Cutoff_Frq.InsertString(1, _T("500Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(2, _T("300Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(3, _T("200Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(4, _T("150Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(5, _T("100Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(6, _T("50Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(7, _T("40Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(8, _T("30Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(9, _T("20Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(10, _T("10Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(11, _T("5Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(12, _T("3Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(13, _T("2Hz"));
	m_combo_Filter_Cutoff_Frq.InsertString(14, _T("1Hz"));
	m_combo_Filter_Cutoff_Frq.SetCurSel(0);

	// enable/disable controls
	GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(FALSE);
	GetDlgItem(IDC_READ_OVERLOAD_COUNT)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(FALSE);

	// for graph (RT_GRAPH_ARRAY)
	mGA.init_graph_array(2, 1);
	mGA.get_graph(0, 0)->set_x_axis_min_max(0, time_to_display);
	mGA.get_graph(0, 0)->set_y_axis_title(_T("Force(N)"), 18, 0, RT_GRAPH::Bold);
	mGA.get_graph(0, 0)->set_y_axis_ticks(7,14,45,RT_GRAPH::NarrowBold);
	mGA.get_graph(0, 0)->set_legend(true);

	mLS[0] = mGA.get_graph(0, 0)->create_line_series(_T("Fx"), RT_RED, 1.5f);
	mLS[1] = mGA.get_graph(0, 0)->create_line_series(_T("Fy"), RT_GREEN, 1.5f);
	mLS[2] = mGA.get_graph(0, 0)->create_line_series(_T("Fz"), RT_BLUE, 1.5f);

	
	mGA.get_graph(1, 0)->set_x_axis_min_max(0, time_to_display);
	mGA.get_graph(1, 0)->set_y_axis_title(_T("Torque(Nm)"), 18, 0,RT_GRAPH::Bold);
	mGA.get_graph(1, 0)->set_y_axis_ticks(7, 14, 45, RT_GRAPH::NarrowBold);
	mGA.get_graph(1, 0)->set_legend(true);
	mLS[3] = mGA.get_graph(1, 0)->create_line_series(_T("Tx"), RT_RED, 1.5f);
	mLS[4] = mGA.get_graph(1, 0)->create_line_series(_T("Ty"), RT_GREEN, 1.5f);
	mLS[5] = mGA.get_graph(1, 0)->create_line_series(_T("Tz"), RT_BLUE, 1.5f);

	mGA.synchronize(true, false);
	mGA.draw(); // Initial Draw

	// set default F/T display range
	m_disp_F_max = 50.0;
	m_disp_F_min = -50.0;
	m_disp_T_max = 5.0;
	m_disp_T_min = -5.0;

	// set auto scale check box
	m_auto_scale_F = TRUE;
	m_auto_scale_T = TRUE;


	// Auto Display 
	((CButton*)GetDlgItem(IDC_CHECK_AUTO_DISPLAY))->SetCheck(1);

	// to mesaure time
	QueryPerformanceFrequency(&g_freq);

	// UpdateData 
	UpdateData(FALSE);

	// set OnTimer period
	SetTimer(0, TIMER_PERIOD, NULL);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CRFTS_InterfaceDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CRFTS_InterfaceDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CRFTS_InterfaceDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRFTS_InterfaceDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	KillTimer(0);

}

void CRFTS_InterfaceDlg::OnTimer(UINT_PTR nIDEvent)
{

	double time_to_draw_graph = 0;
	double time_to_get_data = 0;

	if (m_rfts_set.size() > 0 )  // if a rfts exist 		
	if (((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck())// && (m_RFTS_stack[mRFTS_ID].op_mode == RFTS_OP_MODE::FT_CONT) )
	{
		// Draw Graph -----------------------------------------------------------------
		// disable graph display

		// get start time
		QueryPerformanceCounter(&g_st); 

		// get data 
		uint32 read_size = (uint32)(time_to_display / (float)run_cycle *1000.0f);
		if (read_size > (uint32)buf_size)
			read_size = buf_size;

		RFTS_RX_FT_DATA *rx_ft_data = new RFTS_RX_FT_DATA[read_size];

		int read_size_actual = (int)m_rfts_set.at(mRFTS_ID)->get_FT_buffer()->read_latest(rx_ft_data, read_size);

		QueryPerformanceCounter(&g_ed); // get time 
		time_to_get_data = ((double)g_ed.QuadPart - g_st.QuadPart) / ((double)g_freq.QuadPart);  
		// result: ~0.01ms @(release mode & 1000 sample) 
		// 기존 vector를 이용하는 방식에서는 < 0.1ms @ 600 samples  
		if (read_size_actual > 1)
		{
			// allocate memory
			float	*t = new float[read_size_actual];
			float  *ft = new float[read_size_actual];
			// copy data
			for (int axis = 0; axis < 6; axis++)
			{
				for (int idx = 0; idx < read_size_actual; idx++)
				{
					t[idx] = (float)(idx * run_cycle)/1000.0;
					ft[idx] = (float)rx_ft_data[idx].FT.entry[axis];
				}
				mLS[axis]->set_points(t, ft, read_size_actual);
			}
			delete[] t;
			delete[] ft;
		}
		delete[] rx_ft_data;
		mGA.draw();

		QueryPerformanceCounter(&g_ed); // get end time
		time_to_draw_graph = ((double)g_ed.QuadPart - g_st.QuadPart) / ((double)g_freq.QuadPart);  
		// ~13ms @(release mode & 1000 sample & line width = 2)
		// 기존 ChartCtrl을 사용할 때는 ~ 60ms @ 600 samples
		// 기존 ChartCtrl경우에는 그래프의 복잡한 정도나 라인의 두께에 영향을 받았지만, RT_GRAPH를 사용한 경우에는 거의 영향이 없다.

		// display FT values
		disp_FT();


		float T_sensor;
		m_rfts_set.at(mRFTS_ID)->get_temperaure(T_sensor);
		CString msg;
		//msg.Format(_T("data counter: %d, time[data:%.3fms, graph:%.2fms] T=%.1f"), callback_cnt, time_to_get_data*1000.0, time_to_draw_graph*1000.0, T_sensor);
		msg.Format(_T("data counter: %d, Temperature=%.1f"), callback_cnt, T_sensor);

		GetDlgItem(IDC_EDIT_MAIN_MSG)->SetWindowText(msg);
	}

	CDialogEx::OnTimer(nIDEvent);
}


BOOL CRFTS_InterfaceDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	int YesNo = 0;
	bool isForcedReturn = false;
	switch (pMsg->message)    /// 종료 키보드 메세지 리턴
	{
	case WM_KEYDOWN:
		if ((pMsg->wParam == VK_ESCAPE) | (pMsg->wParam == VK_RETURN))
			isForcedReturn = true;
		break;

	case WM_SYSKEYDOWN:    // Alt + F4 메세지 처리
		if (pMsg->wParam == VK_F4)
		{
			YesNo = AfxMessageBox(_T("Do you want to exit?"), MB_YESNO, NULL);

			if (YesNo == IDYES)
			{
				DestroyWindow();
			}

			isForcedReturn = true;
		}
		break;
	case WM_LBUTTONDOWN:
		//CONSOLE_S("MAIN DLG BN_CLICKED\n");
		break;
	default:
		break;
	}

	if (isForcedReturn)
		return TRUE;

	return CDialogEx::PreTranslateMessage(pMsg);
}



void CRFTS_InterfaceDlg::OnBnClickedCheckInterfaceOpen()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData();

	BOOL isOn = (BOOL)((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->GetCheck();
	if (isOn)
	{
		int selected_nic = m_nic_info.GetCurSel();

		string nic_dev_name = m_ecat_interface.usable_NICs[selected_nic].name;

		UINT cycle[] = { 1, 2, 3, 4, 5, 10 };
		int selected_cycle = m_combo_ecat_cycle.GetCurSel();
		run_cycle = cycle[selected_cycle];

		// 그래프 x축 범위 설정
		mGA.get_graph(0, 0)->set_x_axis_min_max(0, time_to_display);
		mGA.get_graph(1, 0)->set_x_axis_min_max(0, time_to_display);

		if (m_ecat_interface.start(nic_dev_name, run_cycle)) 
		{
 			((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetWindowText(_T("EtherCAT Stop"));

			GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(TRUE);
			GetDlgItem(IDC_READ_OVERLOAD_COUNT)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(TRUE);

			mRFTS_ID = 0;

			m_rfts_set.attach_all_rfts_on_ecat(m_ecat_interface);
			m_rfts_set.at(mRFTS_ID)->set_callback_on_FT_receive(callback_RFT_Data_Receive, this);
			m_ecat_interface.enable();

			m_rfts_set.update_info();
			disp_rfts_info();
			disp_setup_info();

			for (int i = 0; i < m_rfts_set.size();i++)
			{
				RFTS *rfts = NULL;
				rfts = m_rfts_set.at(i);
				rfts->activate_vendor_mode(); 
				rfts->stop_Cap_out(); 
				//rfts->set_filter(RFTS_LPF_CUTOFF::_100Hz);

				if (((CButton*)GetDlgItem(IDC_CHECK_BIAS))->GetCheck())
					rfts->set_bias(true);
				else
					rfts->set_bias(false);
				rfts->get_FT_buffer()->set_size(buf_size);
			}

			if (m_rfts_set.at(mRFTS_ID)->protocol == RFTS::PROTOCOL::EC02)
			{
				m_rfts_set.at(mRFTS_ID)->set_output_rate(RFTS_OUTPUT_RATE::_1000Hz);
			}

		}
		else
		{

			((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetCheck(0);
			((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetWindowText(_T("EtherCAT Start"));

			GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(FALSE);
			GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(FALSE);
			GetDlgItem(IDC_READ_OVERLOAD_COUNT)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
			GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(FALSE);

			AfxMessageBox((CString)m_ecat_interface.last_err_msg.c_str());
		}
	}
	else // 인터페이스 종료
	{
		// 연속출력 정지
		if (((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck())
		{
			((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->SetCheck(0);
			OnBnClickedCheckFtOutCont();
		}
		// un-bias
		if (((CButton*)GetDlgItem(IDC_CHECK_BIAS))->GetCheck())
		{
			((CButton*)GetDlgItem(IDC_CHECK_BIAS))->SetCheck(0);
			m_rfts_set.at(0)->set_bias(false);
		}


		((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetWindowText(_T("EtherCAT Start"));

		GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->SetCheck(FALSE);
		GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(FALSE);
		GetDlgItem(IDC_READ_OVERLOAD_COUNT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(FALSE);

		m_ecat_interface.stop();
		m_rfts_set.clear();
	}
}


void CRFTS_InterfaceDlg::callback_RFT_Data_Receive(void *object_callbacked, double *ft)
{

	CRFTS_InterfaceDlg *me = (CRFTS_InterfaceDlg*)object_callbacked;

	if (me->m_rfts_set.size())
	if (me->m_rfts_set.at(me->mRFTS_ID)->op_mode == RFTS::FT_CONT)
	{
		me->callback_cnt++;

		// save data if logging is on 
		if (me->logging)
		{
			double sampling_time = (double)(me->run_cycle) / 1000.0;
			FTS_DATA data;
			data.time = (double)me->measured_data.size() * sampling_time;
			for (int axis = 0; axis < 6; axis++)
				data.FT[axis] = ft[axis];
			me->measured_data.push_back(data);
		}
	}
}

void CRFTS_InterfaceDlg::OnBnClickedCheckBias()
{
	if (((CButton*)GetDlgItem(IDC_CHECK_BIAS))->GetCheck())
	{
		m_rfts_set.at(mRFTS_ID)->set_bias();
	}
	else
	{
		m_rfts_set.at(mRFTS_ID)->set_bias(false);
	}
}

void CRFTS_InterfaceDlg::OnBnClickedCheckFtOutCont()
{
	if (((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck())
	{
		m_rfts_set.at(mRFTS_ID)->start_FT_out();
		m_rfts_set.at(mRFTS_ID)->get_FT_buffer()->clear();
		callback_cnt = 0;

		// 그래프 x축 범위 설정
		mGA.get_graph(0, 0)->set_x_axis_min_max(0, time_to_display);
		mGA.get_graph(1, 0)->set_x_axis_min_max(0, time_to_display);

		// zoom 해제
		mGA.get_graph(0, 0)->zoom(false);
		mGA.get_graph(1, 0)->zoom(false);


		// 그래프 y축 범위 설정
		UpdateData();
		if (m_auto_scale_F)
			mGA.get_graph(0, 0)->set_autoscale(false,true);
		else
			mGA.get_graph(0, 0)->set_y_axis_min_max(m_disp_F_min, m_disp_F_max);
		if (m_auto_scale_T)
			mGA.get_graph(1, 0)->set_autoscale(false, true);
		else
			mGA.get_graph(1, 0)->set_y_axis_min_max(m_disp_T_min, m_disp_T_max);

		GetDlgItem(IDC_CHECK_FT_OUT_CONT)->SetWindowText(_T("Stop Gettting F/T Value"));
		GetDlgItem(IDC_READ_OVERLOAD_COUNT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
	}
	else
	{
		m_rfts_set.at(mRFTS_ID)->stop_FT_out();
		GetDlgItem(IDC_CHECK_FT_OUT_CONT)->SetWindowText(_T("Get F/T Value Continuously"));
		
		GetDlgItem(IDC_READ_OVERLOAD_COUNT)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(TRUE);
	}
}


void CRFTS_InterfaceDlg::OnBnClickedReadOverloadCount()
{

	if (mRFTS_ID >= m_rfts_set.size())
	{
		GetDlgItem(IDC_EDIT_MAIN_MSG)->SetWindowText(_T("No sensor Matched!"));
		return;
	}
	
	RFTS_OVERLOAD_COUNTER oc;
	m_rfts_set.at(mRFTS_ID)->get_overload_counter(oc);

	CString msg = _T("Overload Counter: ");
	for (auto& cnt : oc.byte)
		msg.AppendFormat(_T("%d "), cnt);
	GetDlgItem(IDC_EDIT_MAIN_MSG)->SetWindowText(msg);
}

void CRFTS_InterfaceDlg::OnBnClickedButtonFilterSetting()
{
	UpdateData();

	int filterSel = m_combo_Filter_Cutoff_Frq.GetCurSel();

	if (m_rfts_set.at(mRFTS_ID)->set_filter((RFTS_LPF_CUTOFF)filterSel))
	{
		disp_setup_info();
	}
	else
	{
		GetDlgItem(IDC_EDIT_MAIN_MSG)->SetWindowText(CString(m_rfts_set.at(mRFTS_ID)->error_msg.c_str()));
	}
}


void CRFTS_InterfaceDlg::OnBnClickedCheckDataLogging()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	BOOL isTrue = ((CButton*)GetDlgItem(IDC_CHECK_DATA_LOGGING))->GetCheck();

	if (isTrue)
	{
		if (!((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck())
		{
			((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->SetCheck(1);
			OnBnClickedCheckFtOutCont();
		}

		measured_data.clear();
		callback_cnt = 0;
		GetDlgItem(IDC_CHECK_DATA_LOGGING)->SetWindowText(_T("Stop to SAVE"));
		logging = true;
	}
	else
	{
		logging = false;
		Sleep(200);
		GetDlgItem(IDC_CHECK_DATA_LOGGING)->SetWindowText(_T("Start to SAVE"));

		int measured_data_size = measured_data.size();

		// 저장된 데이터 자동 디스플레이 - 실시간 출력을 정지한 후 저장된 데이터 디스플레이
		if (((CButton*)GetDlgItem(IDC_CHECK_AUTO_DISPLAY))->GetCheck())
		{
			// 출력 정지
			((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->SetCheck(0);
			OnBnClickedCheckFtOutCont();

			if (measured_data_size > 1)
			{
				// allocate memory
				float	*t = new float[measured_data_size];
				float  *ft = new float[measured_data_size];
				// copy data
				for (int axis = 0; axis < 6; axis++)
				{
					for (int idx = 0; idx < measured_data_size; idx++)
					{
						t[idx] = (float)measured_data[idx].time;
						ft[idx] = (float)measured_data[idx].FT[axis];
					}
					mLS[axis]->set_points(t, ft, measured_data_size);
				}
				delete[] t;
				delete[] ft;

				// 전체 출력을 위해 그래프 autoscale
				mGA.get_graph(0, 0)->set_autoscale();
				mGA.get_graph(1, 0)->set_autoscale();

				mGA.draw();
			}
		}

		// Save Data
		CString filename;
		SYSTEMTIME sysTime;
		::GetLocalTime(&sysTime);
		filename.Format(_T("FT_DATA_%04d%02d%02d_%02d%02d%02d.csv"), sysTime.wYear, sysTime.wMonth, sysTime.wDay,
			sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

		FILE *file = NULL;
		_tfopen_s(&file, filename, _T("w"));
		if (file == NULL)
		{
			AfxMessageBox(_T("Error : File Creation"));
			return;
		}

		fprintf_s(file, "Time(s), Fx(N), Fy(N), Fz(N), Tx(Nm), Ty(Nm), Tz(Nm),\n");
		for (int i = 0; i < measured_data_size; i++)
		{
			fprintf_s(file, "%6.03f, %6.03f, %6.03f, %6.03f, %6.03f, %6.03f, %6.03f,\n",
				measured_data[i].time,
				measured_data[i].FT[0], measured_data[i].FT[1], measured_data[i].FT[2],
				measured_data[i].FT[3], measured_data[i].FT[4], measured_data[i].FT[5]);
		}
		fclose(file);
	}
}

void CRFTS_InterfaceDlg::OnBnClickedOk()
{
	KillTimer(0);

	int num_of_rfts = m_rfts_set.size();

	for (int i = 0; i < num_of_rfts; i++)
	{
		m_rfts_set.at(i)->stop_FT_out();
		Sleep(100);
	}

	m_ecat_interface.stop();

	Sleep(200);


	CDialogEx::OnOK();
}

void CRFTS_InterfaceDlg::OnBnClickedButtonDispSavedData()
{
	vector <FTS_DATA> data_set;
	bool error_occured = false;

	// 연속 출력 디스플레이 중지
	if (((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->GetCheck())
	{
		((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->SetCheck(0);
		OnBnClickedCheckFtOutCont();
	}

	// load data
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, _T("CSV File(*.csv)|*.csv|"), this, 0, FALSE); 
	// 맨앞 TRUE - 열기, FALSE - 저장 // 비스타스타일 FALSE해야 문제가 없다
	if (IDOK == dlg.DoModal())
	{
		CString filepathname;
		filepathname = dlg.GetPathName();

		CStdioFile file;
		CFileException e;

		if (!file.Open(filepathname, CFile::modeRead, &e))
		{
			return;
		}
		else
		{
			CString line; // a line string of the file 
			int currPos = 0; // colunm number of data file
			CString field;
			CString dataStr;

			// read out the first line
			file.ReadString(line);
			currPos = 0;
			dataStr = line.Tokenize(_T(","), currPos);

			FTS_DATA sample;
			do{
				if (file.ReadString(line) == NULL)
					break;

				// column number initialize
				currPos = 0;

				// time
				dataStr = line.Tokenize(_T(","), currPos); // currPos는 읽으면 자동으로 증가
				sample.time = _tstof(dataStr);
				// FT data
				for (int i = 0; i < 6; i++)
				{
					dataStr = line.Tokenize(_T(","), currPos);
					sample.FT[i] = _tstof(dataStr);
				}

				data_set.push_back(sample);
			} while (true);
		}
		file.Close();
	}
	else
	{
		return;
	}

	// draw data
	int measured_data_size = data_set.size();

	if (measured_data_size > 1)
	{
		// allocate memory
		float	*t = new float[measured_data_size];
		float  *ft = new float[measured_data_size];
		// copy data
		for (int axis = 0; axis < 6; axis++)
		{
			for (int idx = 0; idx < measured_data_size; idx++)
			{
				t[idx] = (float)data_set[idx].time;
				ft[idx] = (float)data_set[idx].FT[axis];
			}
			mLS[axis]->set_points(t, ft, measured_data_size);
		}
		delete[] t;
		delete[] ft;

		// 전체 출력을 위해 그래프 autoscale
		mGA.get_graph(0, 0)->set_autoscale();
		mGA.get_graph(1, 0)->set_autoscale();

		mGA.draw();
	}
}
void CRFTS_InterfaceDlg::setup_font(void)
{
	// Big Bold
	int nIDs_big_bold[] = {
		IDC_CHECK_INTERFACE_OPEN, 	IDOK };
	for (auto& nid : nIDs_big_bold)
		GetDlgItem(nid)->SetFont(mFontSet.f_big_bold, true);

	// Normal Bold
	int nIDs_normal_bold[] = {
		IDC_CHECK_BIAS, IDC_CHECK_FT_OUT_CONT, IDC_CHECK_DATA_LOGGING
	};
	for (auto& nid : nIDs_normal_bold)
		GetDlgItem(nid)->SetFont(mFontSet.f_normal_bold, true);

	// Narrow Bold
	int nIDs_narrow_bold[] = {
		IDC_EDIT_DISP_F_MIN, IDC_EDIT_DISP_F_MAX,
		IDC_EDIT_DISP_T_MIN, IDC_EDIT_DISP_T_MAX };
	for (auto& nid : nIDs_narrow_bold)
		GetDlgItem(nid)->SetFont(mFontSet.f_normal_narrow_bold, true);

	// Small Bold
	int nIDs_small_bold[] = { IDC_STATIC_F_RANGE, IDC_STATIC_T_RANGE };
	for (auto& nid : nIDs_small_bold)
		GetDlgItem(nid)->SetFont(mFontSet.f_small_bold, true);
}
void CRFTS_InterfaceDlg::disp_rfts_info(void)
{
	RFTS *rfts = m_rfts_set.at(mRFTS_ID);
	m_list_rfts_info.set_text(1, 0, CString(rfts->product_name.c_str()));
	m_list_rfts_info.set_text(1, 2, CString(rfts->serial_number.c_str()));
	m_list_rfts_info.set_text(1, 4, CString(rfts->firmware_version.c_str()));
		
	m_list_rfts_info.update();
}
void CRFTS_InterfaceDlg::disp_FT(void)
{
	RFTS *rfts = m_rfts_set.at(mRFTS_ID);

	FT_double ft_val;
	rfts->get_FT_values(ft_val);

	m_list_rfts_info.set_value(4, 0, ft_val.Fx, 3);
	m_list_rfts_info.set_value(4, 1, ft_val.Fy, 3);
	m_list_rfts_info.set_value(4, 2, ft_val.Fz, 3);
	m_list_rfts_info.set_value(4, 3, ft_val.Tx, 3);
	m_list_rfts_info.set_value(4, 4, ft_val.Ty, 3);
	m_list_rfts_info.set_value(4, 5, ft_val.Tz, 3);

	m_list_rfts_info.update();
}

void CRFTS_InterfaceDlg::disp_setup_info(void)
{
	RFTS *rfts = m_rfts_set.at(mRFTS_ID);
	CString str_setup;
	uint16_t cut_off;

	
	if (rfts->get_filter(cut_off))
	{
		if(cut_off != 0)
			str_setup.Format(_T("LPF Cut_off: %d Hz,    Output Rate: %d samples/sec"), cut_off, 1000/run_cycle);
		else
			str_setup.Format(_T("LPF Cut_off: No Filter,    Output Rate: %d samples/sec"), 1000 / run_cycle);
	}


	m_list_rfts_info.set_text(5, 0, str_setup);

	m_list_rfts_info.update();
}



void CRFTS_InterfaceDlg::OnClickedButtonSetFixedRangeF()
{
	UpdateData();
	mGA.get_graph(0, 0)->set_autoscale(false, false);

	// re-setup y axis range
	if (m_disp_F_max > m_disp_F_min)
		mGA.get_graph(0, 0)->set_y_axis_min_max(m_disp_F_min, m_disp_F_max);
	else
		mGA.get_graph(0, 0)->set_y_axis_min_max(m_disp_F_max, m_disp_F_min);

	// set auto scale ckeck box to be false (disabled)
	m_auto_scale_F = false;
	UpdateData(FALSE);
	
	// re-draw graph if continous drawing is disabled.
	if (!((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck()) //  
	{
		mGA.draw();
	}
}

void CRFTS_InterfaceDlg::OnClickedButtonSetFixedRangeT()
{
	UpdateData();
	mGA.get_graph(1, 0)->set_autoscale(false, false);

	// re-setup y axis range
	if (m_disp_T_max > m_disp_T_min)
		mGA.get_graph(1, 0)->set_y_axis_min_max(m_disp_T_min, m_disp_T_max);
	else
		mGA.get_graph(1, 0)->set_y_axis_min_max(m_disp_T_max, m_disp_T_min);

	// set auto scale ckeck box to be false (disabled)
	m_auto_scale_T = false;
	UpdateData(FALSE);

	// re-draw graph if continous drawing is disabled.
	if (!((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck()) //  
	{
		mGA.draw();
	}
}

void CRFTS_InterfaceDlg::OnClickedCheckAutoScaleF()
{
	UpdateData();
	if (m_auto_scale_F)
		mGA.get_graph(0, 0)->set_autoscale(false, true);
	else
	{
		// re-setup y axis range
		if (m_disp_F_max > m_disp_F_min)
			mGA.get_graph(0, 0)->set_y_axis_min_max(m_disp_F_min, m_disp_F_max);
		else
			mGA.get_graph(0, 0)->set_y_axis_min_max(m_disp_F_max, m_disp_F_min);
	}
	
	if (!((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck()) //  
	{
		mGA.draw();
	}
}

void CRFTS_InterfaceDlg::OnClickedCheckAutoScaleT()
{
	UpdateData();
	if (m_auto_scale_T)
		mGA.get_graph(1, 0)->set_autoscale(false, true);
	else
	{
		// re-setup y axis range
		if (m_disp_T_max > m_disp_T_min)
			mGA.get_graph(1, 0)->set_y_axis_min_max(m_disp_T_min, m_disp_T_max);
		else
			mGA.get_graph(1, 0)->set_y_axis_min_max(m_disp_T_max, m_disp_T_min);
	}

	if (!((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck()) //  
	{
		mGA.draw();
	}

}
