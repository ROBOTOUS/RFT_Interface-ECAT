#ifndef __RT_USER_INTERFACE_H__
#define __RT_USER_INTERFACE_H__
#include "afxwin.h"
#include <vector>
using namespace std;
#include "gdiplus.h"

/***************************************************************/
/*	RTUI_FONT_SET												/
/***************************************************************/
class RTUI_FONT_SET
{
public:
	RTUI_FONT_SET(CString face_name= _T("맑은 고딕"));
	~RTUI_FONT_SET();

	CFont *f_big_bold;
	CFont *f_normal;
	CFont *f_normal_bold;
	CFont *f_normal_narrow_bold;
	CFont *f_small;
	CFont *f_small_bold;

	bool set_font(CString face_name);
private:
	bool is_font_set = false;
	void delete_font_set();
};

/***************************************************************/
/*	RTUI_BAR													/
/***************************************************************/
class RTUI_BAR
{
public:
	enum Mode{
		Continous,
		Discrete,
		NumOfMode
	};
	// Continous 모드에서의 스타일 
	enum CM_STYLE{
		Range,		// 영역을 그리는 스타일
		PositonOnly,	// 포지션만 그리는 스타일
		NumOfCMStyle
	};
	enum STEP_STATE{
		Activated,		// 지금 활성화된 상태 
		Emphasized,		// 강조된 상태
		Deactivated,	// 비활성화된 상태
		Warning,		// 경고를 주는 상태
		Hide,			// 보여지지 않는 상태
		NumOfStepState
	};

private:
	CWnd* m_pWnd;
	CDC* m_pCDC;
	int m_nID;

	CDC memDC;	// For Double Buffering
	CBitmap *m_pBarBitmap, mBarBitmap;

	CRect mBaseRect;
	CRect mDrawRect;
	COLORREF mBarColor;		// Nice Blue
	COLORREF mBGColor;
	COLORREF mBorderColor;	// Dark Grey

	int mSideGap = 0;		// 바와 드로영역사이의 갭을 설정: 수직모드이면 좌우갭, 수평모드면 상하갭

	CBrush mBarBrush;
	CBrush mBarDeactivatedBrush;
	CBrush mBGBrush;

	Mode mMode = Mode::Continous;
	CM_STYLE mCMStyle = CM_STYLE::Range;
	bool mHorizontal;
	bool isInitialzied = false;  // set_range함수를 통해 intitialzie 해야 그림이 그려질 수 있다.

	double mPosMax = 100.0;
	double mPosMin = 0.0;
	double mRangeSize = 100.0;

	double mStepMax = 10;
	double mStepMin = 0;

	double mDrawLength;			// 실제 그릴영역의 길이를 double로 변환 (생성되면서 결정)
	double mRatioValueToPos;	// 표시할 전체값의 범위와  그릴영역의 길이의 비율 --> 값을 영역의 위치로 전환할때 사용 

	int mTotalStep;

	int mPosMarkSize = 5;
	bool mDrawZeroStep = false;
	double mStepSpaceRatio = 0.08; // Discrete 모드에서 스텝에서 차지하는 스페이스의 비율 


	CFont mFont;
	COLORREF mStepTextColor = RGB(255,255,255);
	bool mStepFontLoaded = false;

	// 그리는 영역을 정의하는 변수 타입
	// 그리기 시작하는 위치와 마지막 위치를 정의. 
	// Note: CRect와 동일하게 마지막 위치는 그리지 않는다.  
	typedef struct{
		int start;
		int end;
		CString text;
		COLORREF text_color;
		bool show_text;
		STEP_STATE state;
	}UNIT_STEP;
	// Range Stack
	vector<UNIT_STEP> bar_range_stack;	// bar 부분

	// Mark Stack
	typedef struct{
		CRect rect;
		COLORREF color;
	}POS_MARK;
	vector<POS_MARK> mark_stack;

	CString mErrorMsg;
public:

	RTUI_BAR( CWnd *pWnd, int nID,
		Mode m = Mode::Continous,
		COLORREF bar = RGB(0, 128, 192),
		COLORREF border = RGB(150, 150, 150),
		COLORREF bar_back = RGB(255, 255, 255));
	~RTUI_BAR();

	bool init_continuous_mode(CM_STYLE cm_style, double min, double max);
	// nStep:	number of total steps to use
	// horizontal: stack direction
	bool init_discrete_mode(int nStep, bool horizontal);
	// 폰트 설정 초기화 하지 않은 상태거나 연속 모드에서는 작동하지 않는다.
	void init_font(CString facename = _T("맑은 고딕"));
	void draw_position_mark(double p);
	void draw_from_to(double from, double to);
	void set_step_from_to(int start_step, int end_step);
	bool set_step_text(int step, CString t);
	bool set_step_font(int font_height, int font_width = 0,
		COLORREF font_color = RGB(255, 255, 255), int font_weight = FW_NORMAL,
		CString facename = _T("맑은 고딕"));
	void set_bar_color(COLORREF bar_color, COLORREF bg_color = RGB(255, 255, 255));
	// Fill to Zero Mode 가 아닐때 사용되는 포지션 마크의 사이즈를 변경하고 싶을 때
	// 홀수가 아니면 1을 증가시켜 자동으로 홀수를 만든다.
	void set_pos_mark_size(int s);
	void add_mark(double pos, COLORREF color);
	// 바와 배경사이의 갭을 설정한다. 양쪽 끝은 변하지 않음.
	// 폭의 1/2보다 크면 최대값로 설정된다.
	void set_side_gap(int gap);
	void set_step_state(int step, STEP_STATE s);

private:
	int get_draw_pos_from_value(double v);
	void save_range_stack_for_discrete_mode();
	// step 0 means draw nothing 
	void draw_step(int step, STEP_STATE step_state = STEP_STATE::Activated);
};

/****************************************************************/
/*	How to use RTUI_LIST Class  
	1. Attaching to Custom Control
	- set the class of Custom Control as "RTUI_LIST"
	- include header and source of "RTUI_LIST" class (RT_UserInterface.h, RT_UserInterface.cpp)
	- add a member variable of the attached Custom control

	2. Setup List
	setup_size()	-Mandatory
	- setup_border()
	- setup_default_line_color()
	- setup_default_text_color()
	- setup_default_bg_color()	
	- setup_default_font()
	- setup_row_seperator()
	- setup_col_seperator()
	- setup_row_space_weight()
	- setup_col_space_weight()
	3. Initialize List 
	init()			-Mandatory
	4. Assign Cell Property and Draw
	- set_font_X()
	- set_emphasizing_rect()
	- set_text()
	- set_text_to_merged_cell()
	- set_value()
	- set_value_to_merged_cell()
	5. update the List
	- update()
	6. save list to an image file
	- save_image()

	[NOTE]
	- 위에서 스텝별 순서가 지켜져야 정확히 동작됨.
	- 윈도우를 받아 Attach하는 방식으로 최초 구현했으나,
	- 화면이 지워지는 문제와  
	- OnInitDialog()에서 보통 초기화를 하는데, 업데이트해도 반영되지 않는 문제가 있어
	  (윈도우가 모두 생성된 후 부터 업데이트가 동작됨)
	- CWnd로 부터 상속받아 클래스를 만들어 OnPaint 함수에서 업데이트하는 방식으로 변경
	- OnEraseBkgnd는 화면이 깜박거리는 것을 없애기 위해 수정

****************************************************************/
class RTUI_LIST:public CWnd
{
private:
	CRect mBaseRect;			// dc를 받아오는 윈도우의 전체 클라이언트 영역 
	CRect mDrawRect;			// 실제 리스트가 그려지는 영역
	COLORREF original_bg_color;	// 그릴 윈도우의 기본 배경색

	CDC memDC;					// For Double Buffering
	CBitmap *m_pBitmap, mBitmap;

	bool size_is_set = false;
	bool initialized = false;

	int rows;					// number of rows
	int cols;					// number of columns

	COLORREF m_text_color;		// default text color
	COLORREF m_bg_color;		// default background color

	COLORREF m_brush_color;
	CBrush m_brush;

	CFont *p_default_font;

	typedef struct{
		int width;
		COLORREF color;
	}LIST_LINE;

	LIST_LINE border;
	LIST_LINE emphasizing_rect;

	vector<LIST_LINE> row_seperate_line;  // bottom line of the row
	vector<LIST_LINE> col_seperate_line;  // right line of the column

	// cell 정의
	class CELL{
		//CELL(){};
		//~CELL(){};
	public:
		CRect rect;		// 내용을 그리는 공간 
		CFont *p_font;
		bool is_merged = false;;
		CELL *last_merged_cell;
	};

	// 원래는 2차원이지만 1차원으로 만든 cell stack
	vector<CELL> serialized_cell_stack;

	vector<double> row_space_weight;		// 가로 줄 높이에 대한 weight
	vector<double> col_space_weight;		// 세로 줄   폭에 대한 weight

public:
	RTUI_LIST();
	~RTUI_LIST();
	
	// set size of list 
	// note: this fuction is mandatory.
	bool setup_size(int num_of_rows, int num_of_cols);

	// setup border line width and color. note: after init(), this function has no effect
	void setup_border(int width, COLORREF color = RGB(150, 150, 150)/*Dark Grey*/);

	// setup default line(boder and seperate line color of this list. note: after init(), this function has no effect
	void setup_default_line_color(COLORREF color);

	// setup default text color of this list. note: after init(), this function has no effect
	void setup_default_text_color(COLORREF color);

	// setup default background color of this list. note: after init(), this function has no effect
	void setup_default_bg_color(COLORREF color);
	
	// setup default_font
	// note: after attached to a ui control, the font of the attacted control is automatically set to the default,
	//		 but can be changed with this function
	void setup_default_font(CFont *font);

	// setup width of row seperate line. note: after init(), this function has no effect
	void setup_row_seperator(int row_idx, int width);
	// setup width and color of row seperate line. note: after init(), this function has no effect
	void setup_row_seperator(int row_idx, int width, COLORREF color);

	// setup width of col seperate line. note: after init(), this function has no effect
	void setup_col_seperator(int col_idx, int width);
	// setup width and color of col seperate line. note: after init(), this function has no effect
	void setup_col_seperator(int col_idx, int width, COLORREF color);

	// setup row space weight
	bool setup_row_space_weight(int row_idx, double weight);
	// setup col space weight
	bool setup_col_space_weight(int col_idx, double weight);

	// initialize this list with setup parameters
	// note: this fuction is mandatory.
	bool init();

	// set font of the cells
	void set_font_block(int row_from, int col_from, int row_to, int col_to, CFont *font);
	// set font of the cells with row index
	void set_font_row(int row_idx, CFont *font);
	// set font of the cells with column index
	void set_font_col(int col_idx, CFont *font);
	// set font of a cell
	void set_font_cell(int row_idx, int col_idx, CFont *font);
	// set line with of the emphasizing rect
	void set_emphasizing_rect_property(int width, COLORREF color = RGB(255,0,0));
	void emphasize_cell(int row_idx, int col_idx, bool on = true);

	void merge_cells(int row_from, int col_from, int row_to, int col_to);

	void set_text(int row_idx, int col_idx, CString	 text);
	void set_text(int row_idx, int col_idx, CString	 text, bool emphasizing_rect_on);
	void set_text(int row_idx, int col_idx, CString	 text, COLORREF text_color);
	void set_text(int row_idx, int col_idx, CString  text, COLORREF text_color, COLORREF bg_color);
	void set_text(int row_idx, int col_idx, CString  text, bool emphasizing_rect_on, COLORREF text_color);
	void set_text(int row_idx, int col_idx, CString  text, bool emphasizing_rect_on, COLORREF text_color, COLORREF bg_color);
	
	void set_value(int row_idx, int col_idx, double value, int decimal_places);
	void set_value(int row_idx, int col_idx, double value, int decimal_places, bool emphasizing_rect_on);
	void set_value(int row_idx, int col_idx, double value, int decimal_places, COLORREF text_color);
	void set_value(int row_idx, int col_idx, double value, int decimal_places, bool emphasizing_rect_on, COLORREF text_color);
	void set_value(int row_idx, int col_idx, double value, int decimal_places, bool emphasizing_rect_on, COLORREF text_color, COLORREF bg_color);

	// update list
	void update();

	// save image to the file as the given type
	// if sucess, return true
	bool save_image(CString filename, const GUID& filetype = Gdiplus::ImageFormatPNG);

private:
	int get_space_for_row_seperate_lines();
	int get_space_for_col_seperate_lines();
	void change_brush_color(COLORREF color);
	CELL* get_cell(int r, int c);

	DECLARE_DYNAMIC(RTUI_LIST)
private:
	BOOL RegisterWindowClass();
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);

	DECLARE_MESSAGE_MAP()
};

#endif