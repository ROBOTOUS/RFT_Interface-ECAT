// LogGraphDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "RFTS_Interface.h"
#include "LogGraphDlg.h"
#include "afxdialogex.h"



// CLogGraphDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CLogGraphDlg, CDialogEx)

CLogGraphDlg::CLogGraphDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLogGraphDlg::IDD, pParent)
{

}

CLogGraphDlg::~CLogGraphDlg()
{
}

void CLogGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_DATA_CHART1, m_DataChart1);
	//DDX_Control(pDX, IDC_DATA_CHART2, m_DataChart2);
}


BEGIN_MESSAGE_MAP(CLogGraphDlg, CDialogEx)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SELECT_DATA_SENSOR1, &CLogGraphDlg::OnBnClickedSelectDataSensor1)
	ON_BN_CLICKED(IDC_SELECT_DATA_SENSOR2, &CLogGraphDlg::OnBnClickedSelectDataSensor2)
	ON_BN_CLICKED(IDC_SELECT_DATA_SENSOR3, &CLogGraphDlg::OnBnClickedSelectDataSensor3)
	ON_BN_CLICKED(IDC_SELECT_DATA_SENSOR4, &CLogGraphDlg::OnBnClickedSelectDataSensor4)
	ON_BN_CLICKED(IDC_LOAD_LOG_FILE, &CLogGraphDlg::OnBnClickedLoadLogFile)
END_MESSAGE_MAP()


// CLogGraphDlg 메시지 처리기입니다.



BOOL CLogGraphDlg::Create(CWnd* pParentWnd)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CDialogEx::Create(IDD, pParentWnd);
}


BOOL CLogGraphDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//
	LONG style = ::GetWindowLong(m_hWnd, GWL_STYLE);

//	style &= ~WS_CAPTION; //테두리를 없앤다.
//	style &= ~WS_SYSMENU; //메뉴를 없앤다 (상단 오른쪽 아이콘 메뉴)

	::SetWindowLong(m_hWnd, GWL_STYLE, style);
	int screenx = GetSystemMetrics(SM_CXSCREEN);
	int screeny = GetSystemMetrics(SM_CYSCREEN);

	// resize:
	SetWindowPos(NULL, -4, -4, screenx + 8, screeny + 4, SWP_NOZORDER);


	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	///////////////////////////////////////////////////////////
	// Chart Initialization
	//// 1st Chart
	//CChartStandardAxis* pBottomAxis = m_DataChart1.CreateStandardAxis(CChartCtrl::BottomAxis);
	//pBottomAxis->SetMinMax(0, 1000);
	//pBottomAxis->SetAutomatic(true);
	//CChartStandardAxis* pLeftAxis = m_DataChart1.CreateStandardAxis(CChartCtrl::LeftAxis);
	//pLeftAxis->SetMinMax(-1000, 1000);
	//pLeftAxis->SetAutomatic(true);

	//m_DataChart1.SetBackColor(RGB(255, 255, 255)); // white back-ground color
	//m_DataChart1.GetLegend()->SetVisible(true);

	//m_pChart_value[0] = m_DataChart1.CreateLineSerie(false, false);
	//m_pChart_value[0]->SetWidth(1);
	//m_pChart_value[0]->SetPenStyle(0);
	//m_pChart_value[0]->SetName(_T(" Fx[N]  "));
	//m_pChart_value[0]->SetColor(RGB(255, 0, 0));

	//m_pChart_value[1] = m_DataChart1.CreateLineSerie(false, false);
	//m_pChart_value[1]->SetWidth(1);
	//m_pChart_value[1]->SetPenStyle(0);
	//m_pChart_value[1]->SetName(_T(" Fy[N]  "));
	//m_pChart_value[1]->SetColor(RGB(0, 255, 0));

	//m_pChart_value[2] = m_DataChart1.CreateLineSerie(false, false);
	//m_pChart_value[2]->SetWidth(1);
	//m_pChart_value[2]->SetPenStyle(0);
	//m_pChart_value[2]->SetName(_T(" Fz[N]"));
	//m_pChart_value[2]->SetColor(RGB(0, 0, 255));

	//// 2nd Graph
	//CChartStandardAxis* pBottomAxis2 = m_DataChart2.CreateStandardAxis(CChartCtrl::BottomAxis);
	//pBottomAxis2->SetMinMax(0, 1000);
	//pBottomAxis2->SetAutomatic(true);
	//CChartStandardAxis* pLeftAxis2 = m_DataChart2.CreateStandardAxis(CChartCtrl::LeftAxis);
	//pLeftAxis2->SetMinMax(-1000, 1000);
	//pLeftAxis2->SetAutomatic(true);

	//m_DataChart2.SetBackColor(RGB(255, 255, 255)); // white back-ground color
	//m_DataChart2.GetLegend()->SetVisible(true);

	//m_pChart_value[3] = m_DataChart2.CreateLineSerie(false, false);
	//m_pChart_value[3]->SetWidth(1);
	//m_pChart_value[3]->SetPenStyle(0);
	//m_pChart_value[3]->SetName(_T("Tx[Nm]"));
	//m_pChart_value[3]->SetColor(RGB(255, 0, 0));

	//m_pChart_value[4] = m_DataChart2.CreateLineSerie(false, false);
	//m_pChart_value[4]->SetWidth(1);
	//m_pChart_value[4]->SetPenStyle(0);
	//m_pChart_value[4]->SetName(_T("Ty[Nm]"));
	//m_pChart_value[4]->SetColor(RGB(0, 255, 0));

	//m_pChart_value[5] = m_DataChart2.CreateLineSerie(false, false);
	//m_pChart_value[5]->SetWidth(1);
	//m_pChart_value[5]->SetPenStyle(0);
	//m_pChart_value[5]->SetName(_T("Tz[Nm]"));
	//m_pChart_value[5]->SetColor(RGB(0, 0, 255));

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


BOOL CLogGraphDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	int YesNo = 0;
	bool isForcedReturn = false;
	switch (pMsg->message)    /// 종료 키보드 메세지 리턴
	{
	case WM_KEYDOWN:
		if ((pMsg->wParam == VK_ESCAPE) || (pMsg->wParam == VK_RETURN))
			isForcedReturn = true;
		break;

	case WM_SYSKEYDOWN:    // Alt + F4 메세지 처리
		if (pMsg->wParam == VK_F4)
		{
			YesNo = AfxMessageBox(_T(" 디스플레이를 종료 하시겠습니까? "));

			if (YesNo == IDYES)
			{
				ShowWindow(SW_HIDE);
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


void CLogGraphDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}


void CLogGraphDlg::updateChart()
{
	int sensor_id = m_SensorNumOfSelected;
	int num_of_samples = m_DataLoaded[sensor_id - 1][0].size(); 

	for (int i = 0; i < 6; i++)
	{

		double *x = new double[num_of_samples];
		double *y = new double[num_of_samples];

		for (int idx = 0; idx < num_of_samples; idx++)
		{
		//	x[idx] = (double)m_DataTime[idx];
			x[idx] = (double)m_DataLoaded[sensor_id - 1][0][idx];
			y[idx] = (double)m_DataLoaded[sensor_id - 1][i+1][idx];
		}

		//m_pChart_value[i]->SetPoints(x, y, num_of_samples);

		delete[] x;
		delete[] y;

	}

	//m_DataChart1.RefreshCtrl();
	//m_DataChart2.RefreshCtrl();
}


bool CLogGraphDlg::loadDataFile(void)
{
	CString szFilter = _T("Log Data File(*.csv)|*.csv||");
//	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, szFilter); // 맨앞 TRUE - 열기, FALSE - 저장 
//	CFileDialog dlg(TRUE);// , (TRUE, NULL, NULL, OFN_HIDEREADONLY, szFilter); // 맨앞 TRUE - 열기, FALSE - 저장 
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, szFilter, this, 0, FALSE);
	// 맨앞 TRUE - 열기, FALSE - 저장  // 마지막 비스타 모드 OFF --> 이유는 모르겠지만 비스타 모드는 잘 안된다.

	CString fileFullPathName;

	if (IDOK == dlg.DoModal())
	{
		fileFullPathName = dlg.GetPathName();
	}
	else
		return false;

	CStdioFile file;
	CFileException e;

	if (!file.Open(fileFullPathName, CFile::modeRead, &e))
	{
		::AfxMessageBox(_T("DATA 파일 로딩 에러"));
		return false;
	}

	//
	CString line;
	bool isStop = false;
	int currPos = 0;
	CString dataStr;

	int data_cnt = 0;			 // date set의 개수 : 라인 수와 동일
	int data_number_in_line = 0; //라인당 데이터 개수


	isStop = false;

	// Clear Data Buffer
	for (int i = 0; i < 8; i++)
	for (int j = 0; j < 7; j++)
		m_DataLoaded[i][j].clear();
//	m_DataTime.clear();


	do{
		if (file.ReadString(line) == NULL)
		{
			isStop = true;
			break;
		}

		currPos = 0;
		data_number_in_line = 0;
		do
		{
			dataStr = line.Tokenize(_T(", "), currPos); // token이 ',' 이거나 ' ' (space)인 경우
			//if (data_number_in_line == 0)
			//	m_DataTime.push_back(::atof(dataStr));
			//else
				m_DataLoaded[(data_number_in_line) / 7][(data_number_in_line) % 7].push_back(::atof((const char *)dataStr.GetBuffer()));
			data_number_in_line++;
		} while (dataStr != ""); // 스트링이 끝나면
		data_number_in_line--; 
		data_cnt++;

	} while (!isStop);

	file.Close();

	m_NumDataLoaded = data_cnt;
	m_NumSensor = (data_number_in_line - 1) / 6;

//	CONSOLE_S("data#: %d data#_in_line: %d, sensor#: %d", m_NumDataLoaded, data_number_in_line, m_NumSensor);

	// display sensor1 data initially
	m_SensorNumOfSelected = 1;


	((CButton*)GetDlgItem(IDC_SELECT_DATA_SENSOR1))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_SELECT_DATA_SENSOR2))->SetCheck(0);
	((CButton*)GetDlgItem(IDC_SELECT_DATA_SENSOR3))->SetCheck(0);
	((CButton*)GetDlgItem(IDC_SELECT_DATA_SENSOR4))->SetCheck(0);

	// Diable Radio Button
	GetDlgItem(IDC_SELECT_DATA_SENSOR1)->EnableWindow(FALSE);
	GetDlgItem(IDC_SELECT_DATA_SENSOR2)->EnableWindow(FALSE);
	GetDlgItem(IDC_SELECT_DATA_SENSOR3)->EnableWindow(FALSE);
	GetDlgItem(IDC_SELECT_DATA_SENSOR4)->EnableWindow(FALSE);
	switch (m_NumSensor)
	{
	case 4: GetDlgItem(IDC_SELECT_DATA_SENSOR4)->EnableWindow(TRUE);
	case 3: GetDlgItem(IDC_SELECT_DATA_SENSOR3)->EnableWindow(TRUE);
	case 2: GetDlgItem(IDC_SELECT_DATA_SENSOR2)->EnableWindow(TRUE);
	case 1: GetDlgItem(IDC_SELECT_DATA_SENSOR1)->EnableWindow(TRUE);
	}   

	return true;
}


void CLogGraphDlg::OnBnClickedSelectDataSensor1()
{
	m_SensorNumOfSelected = 1;
	updateChart();
}


void CLogGraphDlg::OnBnClickedSelectDataSensor2()
{
	m_SensorNumOfSelected = 2;
	updateChart();
}


void CLogGraphDlg::OnBnClickedSelectDataSensor3()
{
	m_SensorNumOfSelected = 3;
	updateChart();
}


void CLogGraphDlg::OnBnClickedSelectDataSensor4()
{
	m_SensorNumOfSelected = 4;
	updateChart();
}



void CLogGraphDlg::OnBnClickedLoadLogFile()
{
	loadDataFile();
	updateChart();
}
