#include "RT_UserInterface.h"
#include "RT_CommonUtility.h"
#include <atlimage.h> // for CImage

/***************************************************************/
/*	RTUI_FONT_SET												/
/***************************************************************/
RTUI_FONT_SET::RTUI_FONT_SET(CString face_name)
{
	set_font(face_name);
}

RTUI_FONT_SET::~RTUI_FONT_SET()
{
	delete_font_set();
}
bool RTUI_FONT_SET::set_font(CString face_name)
{
	if (is_font_set)
		delete_font_set();
	// Setup Font Properties
	f_big_bold = new CFont;
	f_big_bold->CreateFont(20, // nHeight
		0, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_BOLD, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		0,                              // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		face_name); // lpszFacename

	f_normal = new CFont;
	f_normal->CreateFont(15, // nHeight
		0, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_MEDIUM, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		0,                              // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		face_name); // lpszFacename

	f_normal_bold = new CFont;
	f_normal_bold->CreateFont(15, // nHeight
		0, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_BOLD, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		0,                              // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		face_name); // lpszFacename

	f_normal_narrow_bold = new CFont;
	f_normal_narrow_bold->CreateFont(15, // nHeight
		6, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_BOLD, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		0,                              // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		face_name); // lpszFacename

	f_small_bold = new CFont;
	f_small_bold->CreateFont(14, // nHeight
		0, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_BOLD, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		0,                              // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		face_name); // lpszFacename

	f_small = new CFont;
	f_small->CreateFont(14, // nHeight
		0, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_NORMAL, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		0,                              // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		face_name); // lpszFacename

	is_font_set = true;
	return true;
}
void RTUI_FONT_SET::delete_font_set()
{
	if (!is_font_set)
		return;

	f_big_bold->DeleteObject();
	f_big_bold->DeleteObject();
	f_normal_bold->DeleteObject();
	f_normal_narrow_bold->DeleteObject();
	f_small->DeleteObject();
	f_small_bold->DeleteObject();

	delete f_big_bold;
	delete f_normal;
	delete f_normal_bold;
	delete f_normal_narrow_bold;
	delete f_small;
	delete f_small_bold;

	is_font_set = false;
}
/***************************************************************/
/*	RTUI_BAR													/
/***************************************************************/

RTUI_BAR::RTUI_BAR(	CWnd *pWnd, int nID, Mode m, COLORREF bar, COLORREF border,	COLORREF bar_back)
{
	m_pWnd = pWnd;
	mMode = m;
	mBarColor = bar;
	mBorderColor = border;
	mBGColor = bar_back;

	pWnd->GetDlgItem(nID)->GetClientRect(mBaseRect);
	m_pCDC = pWnd->GetDlgItem(nID)->GetDC();

	// 드로우 영역을 구한다.
	mDrawRect.top = mBaseRect.top + 1;
	mDrawRect.bottom = mBaseRect.bottom - 1;
	mDrawRect.left = mBaseRect.left + 1;
	mDrawRect.right = mBaseRect.right - 1;

	// 사이즈를 보고 수직 / 수평 결정 --> Discrete Mode에서는 초기화에서 새로 결정됨다.
	if (mBaseRect.right > mBaseRect.bottom) // 수평으로 긴 영역
	{
		mHorizontal = true;
		mDrawLength = (double)(mDrawRect.right - mDrawRect.left);
	}
	else
	{
		mHorizontal = false;
		mDrawLength = (double)(mDrawRect.bottom - mDrawRect.top);
	}

	// default Brush 설정
	mBarBrush.CreateSolidBrush(mBarColor);
	mBarDeactivatedBrush.CreateSolidBrush(RT_GREY);
	mBGBrush.CreateSolidBrush(mBGColor);
	// clear mark stack
	mark_stack.clear();

	// Create a Compatible DC For Double Buffering 
	memDC.CreateCompatibleDC(m_pCDC);
	// Create a Compatible Bitmap For Double Buffering 
	mBarBitmap.CreateCompatibleBitmap(m_pCDC, mBaseRect.Width(), mBaseRect.Height());
	// Connect bitmap to DC  
	m_pBarBitmap = memDC.SelectObject(&mBarBitmap);

	// 베이스 영역의 에지를 그린다.
	memDC.Draw3dRect(mBaseRect, mBorderColor, mBorderColor);
	memDC.FillRect(mDrawRect, &mBGBrush);
	m_pCDC->BitBlt(0, 0, mBaseRect.Width(), mBaseRect.Height(), &memDC, 0, 0, SRCCOPY);
}
RTUI_BAR::~RTUI_BAR()
{
	mBarBrush.DeleteObject();
	mBarDeactivatedBrush.DeleteObject();
	mBGBrush.DeleteObject();
	if (mStepFontLoaded)
		mFont.DeleteObject();

	memDC.SelectObject(m_pBarBitmap);
	memDC.DeleteDC();
	mBarBitmap.DeleteObject();
}

bool RTUI_BAR::init_continuous_mode(CM_STYLE cm_style, double min, double max)
{
	if (mMode == Mode::Continous)
	{
		mCMStyle = cm_style;
		if (min > max)
			exchange(min, max);
		mPosMin = min;
		mPosMax = max;
		mRangeSize = max - min;

		if (cm_style == CM_STYLE::Range)
		{
			mRatioValueToPos = (mDrawLength - 1.0) / mRangeSize;
		}
		else if (cm_style == CM_STYLE::PositonOnly)
		{
			mRatioValueToPos = (mDrawLength - 1.0 - (double)(mPosMarkSize - 1)) / mRangeSize;
		}
		isInitialzied = true;
		return true;
	}
	else
	{
		mErrorMsg = _T("Mode is mis-matched!!");
		return false;
	}

}

bool RTUI_BAR::init_discrete_mode(int nStep, bool horizontal)
{
	if (nStep < 1)
		nStep = 1;
	if (mMode == Mode::Discrete)
	{
		mTotalStep = nStep;

		mHorizontal = horizontal;
		if (mHorizontal) // 수평으로 긴 영역
			mDrawLength = (double)(mDrawRect.right - mDrawRect.left);
		else
			mDrawLength = (double)(mDrawRect.bottom - mDrawRect.top);

		save_range_stack_for_discrete_mode();

		// bar 사이즈 결정 후
		init_font();

		// Setup memDC for Transparent Text Out.
		// 폰트 설정 전에 투명을 설정하면 안된다. 폰트 설정하면서 특성이 사라지는 것 같음.
		memDC.SetBkMode(TRANSPARENT);

		isInitialzied = true;
		return true;
	}
	else
	{
		mErrorMsg = _T("Mode is mis-matched!!");
		return false;
	}
}
void RTUI_BAR::init_font(CString facename)
{
	if (!isInitialzied)
		return;
	int font_height;
	if (mHorizontal)
	{
		font_height = mDrawRect.Height() - mSideGap * 2 - 2;
	}
	else
	{
		font_height = bar_range_stack[0].end - bar_range_stack[0].start;
	}
	set_step_font(font_height);
}
int RTUI_BAR::get_draw_pos_from_value(double v)
{
	return (int)((v - mPosMin)*mRatioValueToPos + 0.5);
}
void RTUI_BAR::save_range_stack_for_discrete_mode()
{
	UNIT_STEP range;
	// initialze range stack
	bar_range_stack.clear();

	// calculate space length
	int space_count = mTotalStep - 1;
	int space_len = (int)(mDrawLength / (double)mTotalStep * mStepSpaceRatio); // ~8%가 스페이스 공간
	if (space_len < 1)
		space_len = 1; // 무조건 1이상

	int start_gap = 2;
	int draw_len_d = (int)(mDrawLength - (double)(space_len*space_count) - (double)start_gap * 2); // 정확한 end위치 계산을 위해 스페이스를 제외한 실제 그려질 공간을 계산한다.

	int end_last = 0;
	for (int i = 0; i < mTotalStep; i++)  // 실제 영역 숫자는 total_step + 1 --> 0 를 그리기 위해
	{
		if (i == 0) // first step
		{
			range.start = start_gap;
			range.end = (int)(draw_len_d *(double)(i + 1) / (double)mTotalStep + 0.5) + start_gap;
		}
		else
		{
			range.start = end_last + space_len;
			range.end = (int)(draw_len_d *(double)(i + 1) / (double)mTotalStep + 0.5) + space_len*i + start_gap;
		}
		end_last = range.end;

		if (i == mTotalStep) // last range --> match end position
			range.end = (int)mDrawLength - start_gap; // end_gap --> start_gap
		// save range.
		bar_range_stack.push_back(range);
	}
}

void RTUI_BAR::draw_position_mark(double p)
{
	if (!isInitialzied)
		return;
	if (mMode != Mode::Continous)
		return; // 동작하지 않는다.
	if (mCMStyle != CM_STYLE::PositonOnly)
		return; // 동작하지 않는다.

	saturate(p, mPosMax, mPosMin);

	int draw_pos = get_draw_pos_from_value(p);

	CRect rect;
	// draw background --> erase the old bar
	memDC.FillRect(mDrawRect, &mBGBrush);

	int pos_mark_offset = mPosMarkSize / 2;

	if (mHorizontal)
	{
		rect.top = mDrawRect.top + mSideGap;
		rect.bottom = mDrawRect.bottom - mSideGap;;
		rect.left = mDrawRect.left + draw_pos;
		rect.right = rect.left + mPosMarkSize;
	}
	else // vertical
	{
		rect.left = mDrawRect.left + mSideGap;;
		rect.right = mDrawRect.right - mSideGap;;
		rect.bottom = mDrawRect.bottom - draw_pos;		// 기준 0 자리는 mDrawRect.bottom-1 그러나 그려져야해서 다시 +1
		rect.top = rect.bottom - mPosMarkSize;
	}
	memDC.FillRect(rect, &mBarBrush);

	// draw Mark
	if (mark_stack.size())
	{
		for (auto& mark : mark_stack)
			memDC.Draw3dRect(mark.rect, mark.color, mark.color);
	}
	m_pCDC->BitBlt(0, 0, mBaseRect.Width(), mBaseRect.Height(), &memDC, 0, 0, SRCCOPY);

}

void RTUI_BAR::draw_from_to(double from, double to) // d
{
	if (!isInitialzied)
		return;
	if (mMode != Mode::Continous)
		return; // 동작하지 않는다.
	if (mCMStyle != CM_STYLE::Range)
		return; // 동작하지 않는다.

	saturate(from, mPosMax, mPosMin);
	saturate(to, mPosMax, mPosMin);

	// 그릴 영역에서의 상대 위치를 찾는다.
	int range_from;
	int range_to;		// 그려질 영역이다. --> 실제 그릴때는 rect의 오른쪽 및 아래라인은 그려지지 않으니 주의 

	range_from = get_draw_pos_from_value(from);
	range_to = get_draw_pos_from_value(to);

	if (range_from > range_to) // range_from should be smaller than or equal to range_to  
		exchange(range_from, range_to);

	CRect rect;
	// draw background --> erase the old bar
	memDC.FillRect(mDrawRect, &mBGBrush);

	if (mHorizontal)
	{
		rect.top = mDrawRect.top + mSideGap;
		rect.bottom = mDrawRect.bottom - mSideGap;;
		rect.left = mDrawRect.left + range_from;
		rect.right = mDrawRect.left + range_to + 1; // range_to을 그리기 위해 +1
	}
	else // vertical
	{
		rect.left = mDrawRect.left + mSideGap;;
		rect.right = mDrawRect.right - mSideGap;;
		rect.bottom = mDrawRect.bottom - range_from;	// 기준 0 자리는 mDrawRect.bottom-1, 거기까지 그리기 위해서는 하나로 아래 (+1)
		rect.top = mDrawRect.bottom - range_to - 1;			// 기준 0 자리는 mDrawRect.bottom-1
	}
	memDC.FillRect(rect, &mBarBrush);

	// draw Mark
	if (mark_stack.size())
	{
		for (auto& mark : mark_stack)
			memDC.Draw3dRect(mark.rect, mark.color, mark.color);
	}
	m_pCDC->BitBlt(0, 0, mBaseRect.Width(), mBaseRect.Height(), &memDC, 0, 0, SRCCOPY);

}

void RTUI_BAR::draw_step(int step, STEP_STATE step_state)
{
	if (!isInitialzied)
		return;
	if (mMode != Mode::Discrete) // Discrete 모드가 아니면 동작하지 않는다.
		return;

	CRect rect;
	if (step < 1)
		return;
	if (mHorizontal)
	{
		rect.top = mDrawRect.top + 2;
		rect.bottom = mDrawRect.bottom - 2;
		rect.left = bar_range_stack[step - 1].start + mDrawRect.left;
		rect.right = bar_range_stack[step - 1].end + mDrawRect.left;
	}
	else // vertical
	{
		rect.left = mDrawRect.left + 2;
		rect.right = mDrawRect.right - 2;
		rect.bottom = -bar_range_stack[step - 1].start + mDrawRect.bottom;
		rect.top = -bar_range_stack[step - 1].end + mDrawRect.bottom;
	}
	// draw step bar
	if (bar_range_stack[step - 1].state == Deactivated)
		memDC.FillRect(rect, &mBarDeactivatedBrush);
	else
		memDC.FillRect(rect, &mBarBrush);

	// draw Text
	rect.bottom = rect.bottom + 1;
	rect.top = rect.top + 1;
	memDC.DrawText(bar_range_stack[step - 1].text, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); //DT_SINGLELINE가 설정되어야 DT_VCENTER가 동작

}

void RTUI_BAR::set_step_from_to(int start_step, int end_step)
{
	if (!isInitialzied)
		return;
	if (mMode != Mode::Discrete)
		return; // 동작하지 않는다.

	saturate(start_step, 0, mTotalStep);
	saturate(end_step, 0, mTotalStep);

	if (start_step > end_step)
		exchange(start_step, end_step);

	// draw background --> erase the old bar
	memDC.FillRect(mDrawRect, &mBGBrush);

	for (int i = start_step; i <= end_step; i++)
		draw_step(i);
	m_pCDC->BitBlt(0, 0, mBaseRect.Width(), mBaseRect.Height(), &memDC, 0, 0, SRCCOPY);
}
bool RTUI_BAR::set_step_text(int step, CString t)
{
	if (!isInitialzied)
		return false;
	if (!is_value_in_range(step, 1, mTotalStep))
		return false;
	else
	{
		bar_range_stack[step - 1].text = t;
		return true;
	}
}

bool RTUI_BAR::set_step_font(int font_height, int font_width,
	COLORREF font_color, int font_weight, CString facename)
{
	if (mStepFontLoaded)
		mFont.DeleteObject();
	BOOL ret = mFont.CreateFont(
		font_height, // nHeight
		font_width, // nWidth
		0, // nEscapement
		0, // nOrientation
		font_weight, // nWeight
		0, // bItalic
		0, // bUnderline 
		0, // cStrikeOut 
		0, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		0,                         // nClipPrecision 
		DEFAULT_QUALITY,       // nQuality
		DEFAULT_PITCH | FF_DONTCARE,  // nPitchAndFamily 
		facename); // lpszFacename
	mStepTextColor = font_color;

	memDC.SelectObject(&mFont);
	memDC.SetTextColor(mStepTextColor);
	if (ret)
	{
		mStepFontLoaded = true;
		return true;
	}
	else
		return false;
}

void RTUI_BAR::set_bar_color(COLORREF bar_color, COLORREF bg_color)
{
	mBarColor = bar_color;
	mBGColor = bg_color;

	mBarBrush.DeleteObject();
	mBarBrush.CreateSolidBrush(mBarColor);
	mBGBrush.DeleteObject();
	mBGBrush.CreateSolidBrush(mBGColor);
}

// Fill to Zero Mode 가 아닐때 사용되는 포지션 마크의 사이즈를 변경하고 싶을 때
// 홀수가 아니면 1을 증가시켜 자동으로 홀수를 만든다.
void RTUI_BAR::set_pos_mark_size(int s)
{
	if (s > 0)
		mPosMarkSize = s;
	if ((mPosMarkSize % 2) == 0) //짝수면 하나더 키운다.
		mPosMarkSize++;

}

void RTUI_BAR::add_mark(double pos, COLORREF color)
{
	if (!isInitialzied)
		return;
	if (mMode != Mode::Continous) // 연속모드가 아니면 동작하지 않는다
		return;
	saturate(pos, mPosMax, mPosMin);
	POS_MARK mark;
	int pos_mark_offset = (mCMStyle == PositonOnly) ? mPosMarkSize / 2 : 0;
	if (mHorizontal)
	{
		mark.rect.top = mBaseRect.top;
		mark.rect.bottom = mBaseRect.bottom;
		int draw_pos = get_draw_pos_from_value(pos);
		mark.rect.left = mDrawRect.left + draw_pos + pos_mark_offset;
		mark.rect.right = mark.rect.left + 1;
	}
	else
	{
		mark.rect.left = mBaseRect.left;
		mark.rect.right = mBaseRect.right;
		int draw_pos = get_draw_pos_from_value(pos);
		mark.rect.top = mDrawRect.bottom - 1 - draw_pos - pos_mark_offset;
		mark.rect.bottom = mark.rect.top + 1;
	}
	mark.color = color;
	mark_stack.push_back(mark);
}
// 바와 배경사이의 갭을 설정한다. 양쪽 끝은 변하지 않음.
// 폭의 1/2보다 크면 최대값로 설정된다.
void RTUI_BAR::set_side_gap(int gap)
{
	if (gap < 0)
		mSideGap = 0;
	else
	{
		int draw_width = (mHorizontal) ? mDrawRect.Height() : mDrawRect.Width(); // 방향과 상관없이 그릴 영역의 짧은 쪽의 길이 
		if ((gap * 2) >= draw_width)
			mSideGap = ((draw_width % 2) == 1) ? draw_width / 2 : draw_width / 2 - 1;
		else
			mSideGap = gap;
	}
}
void RTUI_BAR::set_step_state(int step, STEP_STATE s)
{
	if (is_value_in_range(step, 1, (int)bar_range_stack.size()))
	{
		bar_range_stack[step - 1].state = s;
	}
}


/***************************************************************/
/*	RTUI_LIST													/
/***************************************************************/

RTUI_LIST::RTUI_LIST()
{
	RegisterWindowClass();

	original_bg_color = GetSysColor(COLOR_3DFACE);


	border.color = RT_DARK_GREY;
	border.width = 1;

	m_text_color = RT_BLACK;
	m_bg_color = RT_WHITE;

	emphasizing_rect.color = RT_RED;
	emphasizing_rect.width = 2;

	m_brush_color = RT_WHITE;
	m_brush.CreateSolidBrush(m_brush_color);
}

RTUI_LIST::~RTUI_LIST()
{
	m_brush.DeleteObject();

	memDC.SelectObject(m_pBitmap);
	memDC.DeleteDC();
}
bool RTUI_LIST::setup_size(int num_of_rows, int num_of_cols)
{
	if ((num_of_cols < 1) || (num_of_rows < 1))
		return false;

	// clear mark stack
	serialized_cell_stack.clear();
	row_space_weight.clear();
	col_space_weight.clear();

	rows = num_of_rows;
	cols = num_of_cols;

	// set default seperate line and space weight
	setup_default_line_color(border.color);

	for (int i = 0; i < rows ; i++)
	{
		row_space_weight.push_back(1.0);
	}
	for (int i = 0; i < cols ; i++)
	{
		col_space_weight.push_back(1.0);
	}

	// 원래 폰트
	p_default_font = GetFont();

	size_is_set = true;
	return size_is_set;
}

void RTUI_LIST::setup_default_line_color(COLORREF color)
{
	if (!size_is_set)
		return;

	border.color = color;

	LIST_LINE line;
	line.color = border.color;
	line.width = 1;

	for (int i = 0; i < (rows - 1); i++)
	{
		row_seperate_line.push_back(line);
	}
	for (int i = 0; i < (cols - 1); i++)
	{
		col_seperate_line.push_back(line);
	}
}

void RTUI_LIST::setup_default_text_color(COLORREF color)
{
	m_text_color = color;
}

void RTUI_LIST::setup_default_bg_color(COLORREF color)
{
	m_bg_color = color;
}

void RTUI_LIST::setup_default_font(CFont *font)
{
	p_default_font = font;
}

#define MAX_WIDTH_OF_LINE  20
void RTUI_LIST::setup_border(int width, COLORREF color)
{
	if (!size_is_set)
		return;
	saturate(width, 0, MAX_WIDTH_OF_LINE);

	border.width = width;
	border.color = color;
}

void RTUI_LIST::setup_row_seperator(int row_idx, int width)
{
	if (!size_is_set)
		return;
	if (!is_value_in_range(row_idx, 0, rows - 1))
		return;
	saturate(width, 0, MAX_WIDTH_OF_LINE);
	if ((row_idx < rows) && (row_idx >= 0))
	{
		row_seperate_line[row_idx].width = width;
	}
}
void RTUI_LIST::setup_row_seperator(int row_idx, int width, COLORREF color)
{
	setup_row_seperator(row_idx, width);
	row_seperate_line[row_idx].color = color;
}
void RTUI_LIST::setup_col_seperator(int col_idx, int width)
{
	if (!size_is_set)
		return;
	if (!is_value_in_range(col_idx, 0, cols - 1))
		return;
	saturate(width, 0, MAX_WIDTH_OF_LINE);
	if ((col_idx < cols) && (col_idx >= 0))
	{
		col_seperate_line[col_idx].width = width;
	}
}
void RTUI_LIST::setup_col_seperator(int col_idx, int width, COLORREF color)
{
	setup_col_seperator(col_idx, width);
	col_seperate_line[col_idx].color = color;
}

bool RTUI_LIST::setup_row_space_weight(int row_idx, double weight)
{
	if (weight <= 0)
		return false;
	if (!size_is_set)
		return false;
	if (is_value_in_range(row_idx, 0, rows - 1))
	{
		row_space_weight[row_idx] = weight;
		return true;
	}
	else
		return false;
}

bool RTUI_LIST::setup_col_space_weight(int col_idx, double weight)
{
	if (weight <= 0)
		return false;
	if (!size_is_set)
		return false;
	if (is_value_in_range(col_idx, 0, cols - 1))
	{
		col_space_weight[col_idx] = weight;
		return true;
	}
	else
		return false;
}

bool RTUI_LIST::init()
{
	if (!size_is_set)
		return false;
	if (initialized)
		return false;

	GetClientRect(mBaseRect);

	// 원래 배경색

	// make initial cell stack
	CELL c;
	for (int cell = 0; cell < (rows*cols); cell++)
	{
		c.p_font = p_default_font;
		c.is_merged = false;
		serialized_cell_stack.push_back(c);
		serialized_cell_stack[cell].last_merged_cell = &(serialized_cell_stack[cell]);
	}

	// calculate rect for all cells of the list 
	int total_lines_space = border.width * 2 + get_space_for_row_seperate_lines();
	int remained_space = mBaseRect.Height() - total_lines_space;
	double sum_of_weight = 0.0;
	for (auto w : row_space_weight)
		sum_of_weight += w;
	double ratio_weight_to_space = (double)remained_space / sum_of_weight;

	for (int r = 0; r < rows; r++)
	{
		int top, bottom;
		int space = (int)(row_space_weight[r] * ratio_weight_to_space);
		if (r == 0)
		{
			top = border.width;
			bottom = top + space;
			for (int c = 0; c < cols; c++)
			{
				get_cell(r, c)->rect.top = top;
				get_cell(r, c)->rect.bottom = bottom;
			}
		}
		else 
		{
			top = get_cell(r - 1, 0)->rect.bottom +  row_seperate_line[r - 1].width;
			bottom = top + space;
			for (int c = 0; c < cols; c++)
			{
				get_cell(r, c)->rect.top = top;
				get_cell(r, c)->rect.bottom = bottom;
			}
		}
	}
	
	total_lines_space = border.width * 2 + get_space_for_col_seperate_lines();
	remained_space = mBaseRect.Width() - total_lines_space;
	sum_of_weight = 0.0;
	for (auto w : col_space_weight)
		sum_of_weight += w;
	ratio_weight_to_space = (double)remained_space / sum_of_weight;

	for (int c = 0; c < cols; c++)
	{
		int left, right;
		int space = (int)(col_space_weight[c] * ratio_weight_to_space);
		if (c == 0)
		{
			left = border.width;
			right = left + space;
			for (int r = 0; r < rows; r++)
			{
				get_cell(r, c)->rect.left = left;
				get_cell(r, c)->rect.right = right;
			}
		}
		else
		{
			left = get_cell(0, c - 1)->rect.right + col_seperate_line[c - 1].width;
			right = left + space;
			for (int r = 0; r < rows; r++)
			{
				get_cell(r, c)->rect.left = left;
				get_cell(r, c)->rect.right = right;
			}
		}
	}

	// Create a Compatible DC For Double Buffering 
	memDC.CreateCompatibleDC(GetDC());
	// Create a Compatible Bitmap For Double Buffering 
	mBitmap.CreateCompatibleBitmap(GetDC(), mBaseRect.Width(), mBaseRect.Height());
	// Connect bitmap to DC  
	m_pBitmap = memDC.SelectObject(&mBitmap);
	memDC.SetBkMode(TRANSPARENT);

	// 전체 그라운드를 그린다.
	change_brush_color(original_bg_color);
	memDC.FillRect(mBaseRect, &m_brush);

	// 실제 그리는 영역 구하기
	mDrawRect.left = mDrawRect.top = 0;
	mDrawRect.right = get_cell(rows - 1, cols - 1)->rect.right + border.width;
	mDrawRect.bottom = get_cell(rows - 1, cols - 1)->rect.bottom + border.width;

	CRect rect = mDrawRect;
	// 보더를 그린다.
	for (int i = 0; i < border.width; i++)
	{
		memDC.Draw3dRect(rect, border.color, border.color);
		rect.DeflateRect(1, 1);
	}
	// 내부 백그라운드를 그린다.
	change_brush_color(m_bg_color);
	memDC.FillRect(rect, &m_brush);

	// 내부의 수직 라인을 그린다.
	for (int c = 0; c < (cols - 1); c++)
	{
		rect.left = get_cell(0, c)->rect.right;
		rect.right = rect.left + col_seperate_line[c].width;
		rect.top = get_cell(0, 0)->rect.top;
		rect.bottom = get_cell(rows - 1, 0)->rect.bottom;

		change_brush_color(col_seperate_line[c].color);
		memDC.FillRect(rect, &m_brush);
	}

	// 내부의 수평 라인을 그린다.
	for (int r = 0; r < (rows - 1); r++)
	{
		rect.top = get_cell(r, 0)->rect.bottom;
		rect.bottom = rect.top + row_seperate_line[r].width;
		rect.left = get_cell(0, 0)->rect.left;
		rect.right = get_cell(0, cols - 1)->rect.right;
		
		change_brush_color(row_seperate_line[r].color);
		memDC.FillRect(rect, &m_brush);
	}
	update();

	initialized = true;
	return initialized;
}

// set font of the cells
void RTUI_LIST::set_font_block(int row_from, int col_from, int row_to, int col_to, CFont *font)
{
	for (int r = row_from; r <= row_to; r++)
	for (int c = col_from; c <= col_to; c++)
		set_font_cell(r, c, font);
}
// set font of the cells with row index
void RTUI_LIST::set_font_row(int row_idx, CFont *font)
{
	for (int c = 0; c < cols; c++)
		set_font_cell(row_idx, c, font);
}
// set font of the cells with column index
void RTUI_LIST::set_font_col(int col_idx, CFont *font)
{
	for (int r = 0; r < rows; r++)
		set_font_cell(r, col_idx, font);
}
// set font of a cell
void RTUI_LIST::set_font_cell(int row_idx, int col_idx, CFont *font)
{
	if (initialized)
		get_cell(row_idx, col_idx)->p_font = font;
}

void RTUI_LIST::set_emphasizing_rect_property(int width, COLORREF color)
{
	emphasizing_rect.width = width;
	emphasizing_rect.color = color;
}
void RTUI_LIST::emphasize_cell(int row_idx, int col_idx, bool on)
{

}

void RTUI_LIST::merge_cells(int row_from, int col_from, int row_to, int col_to)
{
	if ((row_from > row_to) || (col_from > col_to))
		return;
	if ((!is_value_in_range(row_from, 0, rows-1)) || (!is_value_in_range(row_to, 0, rows-1)))
		return;
	if ((!is_value_in_range(col_from, 0, cols-1)) || (!is_value_in_range(col_to, 0, cols-1)))
		return;

	CELL *cell_merged = get_cell(row_from, col_from);
	cell_merged->is_merged = true;
	cell_merged->last_merged_cell = get_cell(row_to, col_to);
}


void RTUI_LIST::set_text(int row_idx, int col_idx, CString  text)
{
	set_text(row_idx, col_idx, text, false, m_text_color, m_bg_color);
}

void RTUI_LIST::set_text(int row_idx, int col_idx, CString  text, bool emphasizing_rect_on)
{
	set_text(row_idx, col_idx, text, emphasizing_rect_on, m_text_color, m_bg_color);
}

void RTUI_LIST::set_text(int row_idx, int col_idx, CString  text, COLORREF text_color)
{
	set_text(row_idx, col_idx, text, false, text_color, m_bg_color);
}

void RTUI_LIST::set_text(int row_idx, int col_idx, CString  text, COLORREF text_color, COLORREF bg_color)
{
	set_text(row_idx, col_idx, text, false, text_color, bg_color);
}

void RTUI_LIST::set_text(int row_idx, int col_idx, CString  text, bool emphasizing_rect_on, COLORREF text_color)
{
	set_text(row_idx, col_idx, text, emphasizing_rect_on, text_color, m_bg_color);
}

void RTUI_LIST::set_text(int row_idx, int col_idx, CString  text, bool emphasizing_rect_on, COLORREF text_color, COLORREF bg_color)
{
	if (!initialized)
		return;
	
	CELL *cell = get_cell(row_idx, col_idx);
	CRect rect;
	if (cell->is_merged)
	{
		rect.left = cell->rect.left;
		rect.top = cell->rect.top;
		
		rect.right = cell->last_merged_cell->rect.right;
		rect.bottom = cell->last_merged_cell->rect.bottom;
	}
	else
	{
		rect = cell->rect;
	}
	CRect rect_in;
	if (emphasizing_rect_on)
	{
		// first, draw empasizing rect to background
		change_brush_color(emphasizing_rect.color);
		memDC.FillRect(rect, &m_brush);
	
		// erase background
		rect_in = rect;
		rect_in.DeflateRect(emphasizing_rect.width, emphasizing_rect.width);

		change_brush_color(bg_color);
		memDC.FillRect(rect_in, &m_brush);
	}
	else
	{
		// erase background
		change_brush_color(bg_color);
		memDC.FillRect(rect, &m_brush);
	}
	// Set the font of the cell
	memDC.SelectObject(get_cell(row_idx, col_idx)->p_font);
	memDC.SetTextColor(text_color);
	memDC.DrawText(text, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); //DT_SINGLELINE가 설정되어야 DT_VCENTER가 동작
}

void RTUI_LIST::set_value(int row_idx, int col_idx, double  value, int decimal_places)
{
	set_value(row_idx, col_idx, value, decimal_places, false, m_text_color, m_bg_color);
}

void RTUI_LIST::set_value(int row_idx, int col_idx, double  value, int decimal_places, bool emphasizing_rect_on)
{
	set_value(row_idx, col_idx, value, decimal_places, emphasizing_rect_on, m_text_color, m_bg_color);
}

void RTUI_LIST::set_value(int row_idx, int col_idx, double  value, int decimal_places, COLORREF text_color)
{
	set_value(row_idx, col_idx, value, decimal_places, false, text_color, m_bg_color);
}

void RTUI_LIST::set_value(int row_idx, int col_idx, double value, int decimal_places, bool emphasizing_rect_on, COLORREF text_color)
{
	set_value(row_idx, col_idx, value, decimal_places, emphasizing_rect_on, text_color, m_bg_color);
}

void RTUI_LIST::set_value(int row_idx, int col_idx, double value, int decimal_places, bool emphasizing_rect_on, COLORREF text_color, COLORREF bg_color)
{
	CString text;
	CString format;
	saturate(decimal_places, 0, 9);  // 9자리까지만..
	format.Format(_T("%%.0%df"), decimal_places);
	text.Format(format.GetBuffer(), value);
	set_text(row_idx, col_idx, text, emphasizing_rect_on, text_color, bg_color);
}

void RTUI_LIST::update()
{
	Invalidate();
}

bool RTUI_LIST::save_image(CString filename, const GUID& filetype)
{
	CImage image;

	image.Create(mDrawRect.Width(), mDrawRect.Height(), memDC.GetDeviceCaps(BITSPIXEL));
	CDC* imageDC = CDC::FromHandle(image.GetDC()); // 저장할 이미지의 dc;
	imageDC->BitBlt(0, 0, mDrawRect.Width(), mDrawRect.Height(), &memDC, 0, 0, SRCCOPY);
	image.ReleaseDC();

	// image 저장 
	return (image.Save(filename, filetype) == S_OK);
}


int RTUI_LIST::get_space_for_row_seperate_lines()
{
	int space = 0;
	for (int i = 0; i < (rows - 1); i++) // 마지막 것 제외
		space += row_seperate_line[i].width;
	return space;
}
int RTUI_LIST::get_space_for_col_seperate_lines()
{
	int space = 0;
	for (int i = 0; i < (cols - 1); i++) // 마지막 것 제외
		space += col_seperate_line[i].width;
	return space;
}
void RTUI_LIST::change_brush_color(COLORREF color)
{
	if (m_brush_color != color)
	{
		m_brush_color = color;
		m_brush.DeleteObject();
		m_brush.CreateSolidBrush(m_brush_color);
	}
}

RTUI_LIST::CELL* RTUI_LIST::get_cell(int r, int c)
{
	return &(serialized_cell_stack[r*cols + c]);
}

IMPLEMENT_DYNAMIC(RTUI_LIST, CWnd)

BEGIN_MESSAGE_MAP(RTUI_LIST, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//ON_WM_MOUSEMOVE()
	//ON_WM_LBUTTONDOWN()
	//ON_WM_LBUTTONUP()
	//ON_WM_LBUTTONDBLCLK()
	//ON_WM_RBUTTONDOWN()
	//ON_WM_RBUTTONUP()
	//ON_WM_RBUTTONDBLCLK()
	//ON_WM_SIZE()
	//ON_WM_HSCROLL()
	//ON_WM_VSCROLL()
END_MESSAGE_MAP()

#define RTUI_LIST_CLASSNAME    _T("RTUI_LIST")
BOOL RTUI_LIST::RegisterWindowClass()
{
	WNDCLASS    WndClass;
	HINSTANCE   hInstance = AfxGetInstanceHandle();

	if (GetClassInfo(hInstance, RTUI_LIST_CLASSNAME, &WndClass) == FALSE)
	{
		memset(&WndClass, 0, sizeof(WNDCLASS));

		WndClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		WndClass.lpfnWndProc = ::DefWindowProc;
		WndClass.cbClsExtra = 0;
		WndClass.cbWndExtra = 0;
		WndClass.hInstance = hInstance;
		WndClass.hIcon = NULL;
		WndClass.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		WndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		WndClass.lpszMenuName = NULL;
		WndClass.lpszClassName = RTUI_LIST_CLASSNAME;

		if (!AfxRegisterClass(&WndClass))
		{
			//	AfxThrowResourceException();
			return FALSE;
		}
	}
	return TRUE;
}
void RTUI_LIST::OnPaint()
{
	CPaintDC dc(this);

	GetDC()->BitBlt(0, 0, mBaseRect.Width(), mBaseRect.Height(), &memDC, 0, 0, SRCCOPY);
}
BOOL RTUI_LIST::OnEraseBkgnd(CDC *pDC)
{
	//return CWnd::OnEraseBkgnd(pDC);
	// 위의 원래함수는 배경을 한번 지우기 때문에 깜박거리는 현상이 생긴다.
	// 아래처럼 리턴해 주면 깜박임이 사라진다.
	return false;
}
