#pragma once

//#include "ChartCtrl.h"
//#include "ChartLineSerie.h"
#include <vector>
#include <thread>
#include <mutex>
using namespace std;
//#include "resource.h"


// CLogGraphDlg 대화 상자입니다.

class CLogGraphDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CLogGraphDlg)

public:
	CLogGraphDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CLogGraphDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_LOG_GRAPH_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	// FOR CHART
	//CChartCtrl m_DataChart1;
	//CChartCtrl m_DataChart2;
	//CChartLineSerie *m_pChart_value[6];

	vector<double> m_DataLoaded[8][1+6]; // 8개 센서까지  // 타임택 추가
	vector<double> m_DataTime;

	UINT32 m_NumDataLoaded; // 측정 데이터 세트의 개수
	int	m_NumSensor;	    // 측정된 데이터로 확인하는 센서 개수

	int m_SensorNumOfSelected;
	
	bool loadDataFile(void);
	void updateChart();



	virtual BOOL Create(CWnd* pParentWnd = NULL);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedSelectDataSensor1();
	afx_msg void OnBnClickedSelectDataSensor2();
	afx_msg void OnBnClickedSelectDataSensor3();
	afx_msg void OnBnClickedSelectDataSensor4();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedLoadLogFile();
};
