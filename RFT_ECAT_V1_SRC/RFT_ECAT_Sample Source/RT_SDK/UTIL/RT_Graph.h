#pragma once
#include "RT_CommonUtility.h"

// Driect2D 사용을 위한 설정
#include <d2d1.h>
#include <d2d1_1helper.h>
#pragma comment(lib, "d2d1.lib")

typedef enum{
	Solid,
	Dash,
	Dot,
	DashDot
}LineStyle;
class LINE_SERIES
{
public:
	LINE_SERIES(){
		x_data = NULL;
		y_data = NULL;
		m_data_size = 0;
	};
	~LINE_SERIES(){
		if (x_data)
		{
			delete[] x_data;
			x_data = NULL;
		}
		if (y_data)
		{
			delete[] y_data;
			x_data = NULL;
		}
		//SafeRelease(&line_geometry);
	};

private:	
	float *x_data;
	float *y_data;
	int m_data_size = 0;

	// Line Properties
	LineStyle m_style;
	float m_width;
	COLORREF m_color;
	float m_opacity = 1.0f; // 불투명도

	bool is_this_shown = true;
	bool is_data_serial = false;
	bool is_in_order = false;  // 데이터의 x가 오름차순으로 정리될 경우 true
	bool is_drawn = false;

	ID2D1Factory *D2D_Factory = NULL;
public:
	int size;
	CString name;

	float x_min = 0.0f;
	float x_max = 0.0f;
	float y_min = 0.0f;
	float y_max = 0.0f;

	float m_sample_inteval = 1.0f;

	// set pen properties
	// possible styles: PS_SOLID, PS_DASH, PS_DOT,...  
	void set_width_and_style(float width, LineStyle style = LineStyle::Solid)
	{
		m_width = width;
		m_style = style;
	}
	void set_color(COLORREF color, float opacity = 1.0f)
	{
		m_color = color;
		m_opacity = opacity;
	}
	COLORREF get_color()
	{
		return m_color;
	}
	float get_opacity()
	{
		return m_opacity;
	}
	float get_width()
	{
		return m_width;
	}
	LineStyle get_stroke_style()
	{
		return m_style ;
	}

	void set_points(float *x, float *y, int data_size, bool ordering = false)
	{
		if (data_size < 2)
		{
			data_size = 0;
			return;
		}
		if (x_data != NULL) 
			delete[] x_data;
		if (y_data != NULL)
			delete[] y_data;

		m_data_size = data_size;
		x_data = new float[data_size];
		y_data = new float[data_size];

		x_data[0] = x[0];
		y_data[0] = y[0];

		x_min = x_max = x_data[0];
		y_min = y_max = y_data[0];

		for (int i = 1; i < data_size; i++)
		{
			x_data[i] = x[i];
			updateMinMax(x_min, x_max, x[i]);
			y_data[i] = y[i];
			updateMinMax(y_min, y_max, y[i]);
		}

		is_in_order = true;  // 아래가 정리되면 지울 것
		if (ordering)
		{
			is_in_order = true;
		}
		is_data_serial = false;
		is_drawn = false;
	}
	void set_serial_data(float *serial_data, int data_size, float sample_interval = 1.0f)
	{
		if (data_size < 1)
		{
			data_size = 0;
			return;
		}
		m_sample_inteval = sample_interval;

		if (x_data != NULL)
			delete[] x_data;
		if (y_data != NULL)
			delete[] y_data;

		m_data_size = data_size;
		x_data = new float[data_size];
		y_data = new float[data_size];

		x_data[0] = 0.0f;
		y_data[0] = serial_data[0];
		
		y_min = y_max = serial_data[0];
		for (int i = 1; i < data_size; i++)
		{
			x_data[i] = i*sample_interval;
			y_data[i] = serial_data[i];
			updateMinMax(y_min, y_max, y_data[i]);
		}
		x_min = 0.0f;
		x_max = x_data[data_size-1];

		is_in_order = true;
		is_data_serial = true;
		is_drawn = false;
	}
	bool data_is_in_order()
	{
		return is_in_order;
	}
	void show(bool on)
	{
		is_this_shown = on;
	}
	bool is_shown()
	{
		return is_this_shown;
	}

	float* get_x_data_pointer(void)
	{
		return x_data;
	}

	float* get_y_data_pointer(void)
	{
		return y_data;
	}

	int data_size()
	{
		return m_data_size;
	}
	void delete_data()
	{

	}
	bool has_serial_data()
	{
		return is_data_serial;
	}
	bool was_drawn()
	{
		return is_drawn;
	}
	void has_been_drawn(bool drawn = true)
	{
		if (drawn)
			is_drawn = true;
	}
	void set_d2d_factory(ID2D1Factory *factory)
	{
		D2D_Factory = factory;
	}
};

class RT_GRAPH
{
public:
	// 텍스트의 스타일을 정의하는 Enum
	typedef enum{
		Bold,
		Normal,
		Narrow,
		NarrowBold
	}TEXT_STYLE;

	//정렬을 정의하는 Enum
	typedef enum{
		AlignCenter,
		AlignRight,
		AlignLeft,
	}ALIGNMENT;

protected:
	//기본 크기 정의하는 Enum
	typedef enum{
		DefaultTitleHeight = 20,
		DefaultAxisTitleHeight  = 18,
		DefaultLegendHeight = 16,
		DefaultNumOfTicks = 10,
		DefaultTickLabelWidth = 50,
		DefaultTickLabelHeight = 14,
	}DefaultValues;
	typedef struct{
		float pos;
		float width;
		COLORREF color;
		LineStyle style;
	}ExtraGrid;

	COLORREF m_bg_color;		// 전체 그래프의 배경색: 모든 드로잉 영역에 적용
	COLORREF m_text_color;		// default text color
	COLORREF m_graph_bg_color;	// 그래프를 그리는 영역(mGraphRectAtDaraw)의 배경색
	CString m_font_face;		 

	COLORREF m_brush_color;		// 현재 선택된 브러쉬의 색

	// 라인을 정의하는 구조체
	typedef struct{
		float width;
		COLORREF color;
		LineStyle style;
	}LINE;

	// 텍스트 블록 Class
	class TEXT_BLOCK{
	public:
		TEXT_BLOCK(){ m_border.width = 0.5f; m_border.color = RT_GREY; }
		CString m_text = _T("");
		D2D1_RECT_F rect;
		COLORREF m_text_color;
		COLORREF m_graph_bg_color;
		LINE m_border;
		TEXT_STYLE style = Normal;

		bool show_border = false;
		bool activated = false;

		void set_text(CString text, COLORREF text_color, TEXT_STYLE text_style, COLORREF bg_color) 
		{ 
			m_text = text; 
			m_text_color = text_color; 
			m_graph_bg_color = bg_color;
			style = text_style;

			if (m_text == _T(""))
				activated = false;
			else
				activated = true;
		}
		void set_border_line(float width, COLORREF color)
		{ 
			m_border.width = width; 
			m_border.color = color; 
		}
	};
	// 축을 정의 Class 
	class Axis{
	public:
		Axis(){ extra_grid_stack.clear(); };
		~Axis(){};
		D2D1_RECT_F tick_rect;				// 틱과 값을 그리는 공간 
		float tick_increment = 1.0;			// tick increment : 틱간격
		bool  tick_increment_auto = true;	// tick_increment를 자동 계산	
		float tick_length = 2;										// 설정에서 변경하는 값
		float tick_label_width  = (float)DefaultTickLabelHeight*3.5;// 설정에서 변경하는 값
		float tick_label_height = (float)DefaultTickLabelHeight;	// 설정에서 변경하는 값
		COLORREF tick_text_color = RT_BLACK;										// 설정에서 변경하는 값
		int num_of_ticks = DefaultNumOfTicks;
		TEXT_STYLE tick_label_style = Narrow;
		TEXT_BLOCK title;
		float min, max;
		float title_height = (float)DefaultAxisTitleHeight;			// 설정에서 변경하는 값
		bool show_grid = true;
		LINE grid;
		vector<ExtraGrid> extra_grid_stack;
	};

	LINE border;
	LINE graph_border;

	// 그래프의 제목명
	TEXT_BLOCK graph_title;
	// 그래프 제목의 높이 -->설정에서 변경하는 값 
	float graph_title_height = DefaultTitleHeight;	

	bool show_legend = false;
	D2D1_RECT_F legend_rect;
	float legend_width = (float)DefaultLegendHeight*5;			// 하나의 레전드가 차지하는 폭 --> 설정에서 변경하는 값
	float legend_height = (float)DefaultLegendHeight;			// 하나의 레전드가 차지하는 폭 --> 설정에서 변경하는 값
	COLORREF legend_text_color = RT_BLACK;
	TEXT_STYLE legend_text_style = Normal;

	// 텍스트 블록의 높이와 폰트사이즈 간의 비율. 지금은 상수로 사용하지만 추후에 필요하면 변경
	const float ratio_height_to_font_size = 0.8f;	

	Axis x_axis;
	Axis y_axis;
	
	bool x_axis_is_auto_scaled = true;
	bool y_axis_is_auto_scaled = true;

	bool is_zoomed = false;
	
	float zoom_center_x;	// 그래프영역에서 비율로 정의 ( 0.0 에서 1.0 사이의 값을 가짐 )
	float zoom_center_y;	// 그래프영역에서 비율로 정의 ( 0.0 에서 1.0 사이의 값을 가짐 )
	float zoom_scale_x;		// x방향 zoom 비율 ( 0.0 에서 1.0 사이의 값을 가짐 )
	float zoom_scale_y;		// y방향 zoom 비율 ( 0.0 에서 1.0 사이의 값을 가짐 )

	float zoomed_x_min;		// zoom 모드일때의 x_min
	float zoomed_x_max;
	float zoomed_y_min;
	float zoomed_y_max;
	D2D1_RECT_F zoom_rect;

	float m_x_scale = 1.0f; // 누적되는 전체 x 스케일
	float m_y_scale = 1.0f; // 누적되는 전체 y 스케일

	float m_x_pixel_scale;
	float m_y_pixel_scale;

	vector<LINE_SERIES *> line_stack;

	// for alarm message
	bool is_alarmed = false;
	CString alarm_msg;

	D2D1_RECT_F mBaseRect;			// dc를 받아오는 윈도우의 전체 클라이언트 영역 
	D2D1_RECT_F mDrawRect;			// 실제 그래프가 그려지는 영역
	D2D1_RECT_F mGraphRectAtDraw;	// 드로우영역 기준으로 그래프가 그려지는 영역

	D2D1::Matrix3x2F transform_to_draw;		// 드로우영역으로 좌표계 변환 : 렌더사이즈/화면사이즈 스케일 <- 드로우영역으로 원점이동  
	D2D1::Matrix3x2F transform_to_graph;	// 그래프영역으로 좌표계 변환 : 그래프 영역으로 원점이동


	ID2D1Factory *D2D_Factory = NULL;
	ID2D1DCRenderTarget *RenderTarget = NULL;
	IDWriteFactory	*write_factory = NULL;

	IDWriteTextFormat *wtf_alarm_msg = NULL; // 무조건 생성되기 때문에 기본으로 사용
	IDWriteTextFormat *wtf_x_axis_tick_label = NULL; // 무조건 생성되기 때문에 기본으로 사용
	IDWriteTextFormat *wtf_y_axis_tick_label = NULL;
	IDWriteTextFormat *wtf_legend_label = NULL;

	IDWriteTextLayout *wtl_graph_title = NULL;
	IDWriteTextLayout *wtl_x_axis_title = NULL;
	IDWriteTextLayout *wtl_y_axis_title = NULL;

	ID2D1SolidColorBrush *m_brush;  // 그래프를 그리면서 공용으로 사용할 브러쉬: 특성을 바꿔가며 사용

	ID2D1StrokeStyle *SS_Dash;
	ID2D1StrokeStyle *SS_Dot;
	ID2D1StrokeStyle *SS_DashDot;

public:
	RT_GRAPH(
		ID2D1Factory *factory,
		ID2D1DCRenderTarget *render_target,
		D2D_RECT_F base_rect,
		float pixel_scale_x, float pixel_scale_y);
	~RT_GRAPH();

	// setup title of this graph. 
	void set_title(CString title, int height = DefaultTitleHeight, COLORREF color = RT_BLACK, 
		TEXT_STYLE style = Bold, COLORREF bg_color = COLOR_NOT_DEFINED);

	// setup x axis title of this graph. 
	void set_x_axis_title(CString title, int height = DefaultAxisTitleHeight, COLORREF color = RT_BLACK, 
		TEXT_STYLE style = Normal, COLORREF bg_color = COLOR_NOT_DEFINED);

	// setup x axis title of this graph. 
	void set_y_axis_title(CString title, int height = DefaultAxisTitleHeight, COLORREF color = RT_BLACK, 
		TEXT_STYLE style = Normal, COLORREF bg_color = COLOR_NOT_DEFINED);

	// setup x axis tick properties of this graph. 
	void set_x_axis_ticks(int num_of_ticks = DefaultNumOfTicks, 
		int tick_label_height = DefaultTickLabelHeight, int tick_label_width = DefaultTickLabelWidth,
		TEXT_STYLE style = Narrow, COLORREF tick_label_color = RT_BLACK);
	// setup y axis tick properties of this graph.
	void set_y_axis_ticks(int num_of_ticks = DefaultNumOfTicks, 
		int tick_label_height = DefaultTickLabelHeight, int tick_label_width = DefaultTickLabelWidth,
		TEXT_STYLE style = Narrow, COLORREF tick_label_color = RT_BLACK);
	// setup x axis tick increment of this graph. 
	// if automatic = true, the increment of tick will be automtically calculated from num_of_ticks
	void set_x_axis_tick_increments(bool automatic, float increment);
	// setup y axis tick increment of this graph. 
	// if automatic = true, the increment of tick will be automtically calculated from num_of_ticks
	void set_y_axis_tick_increments(bool automatic, float increment);
	
	// set the total scale of this graph
	void set_scale(float x_scale, float y_scale);

	// setup border line width and color. 
	void set_border(float w, COLORREF color = RT_BLACK);

	// setup border of graph area. 
	void set_graph_border(float w, COLORREF color = RT_BLACK);

	// setup default text color of this graph. 
	void set_default_text_color(COLORREF color);

	// setup background color of graph drawing area.  
	void set_graph_area_bg_color(COLORREF color);

	// setup background color of total drawing area. 
	void set_bg_color(COLORREF color);

	// setup font_face
	// note: after attached to a ui control, the font of the attacted control is automatically set to the default,
	//		 but can be changed with this function
	void set_font_face(CString font_face);

	void set_legend(bool show, int height = DefaultLegendHeight, int width = DefaultLegendHeight * 5);

	void set_x_axis_min_max(float min, float max) { x_axis.min = min; x_axis.max = max; x_axis_is_auto_scaled = false; }
	void set_y_axis_min_max(float min, float max) { y_axis.min = min; y_axis.max = max; y_axis_is_auto_scaled = false; }

	void get_x_axis_min_max(float& min, float& max, bool& is_auto) { min = x_axis.min; max = x_axis.max; is_auto = x_axis_is_auto_scaled; }
	void get_y_axis_min_max(float& min, float& max, bool& is_auto) { min = y_axis.min; max = y_axis.max; is_auto = y_axis_is_auto_scaled; }

	CRect get_graph_rect();
	void zoom(bool on, float center_x = 0.5f, float center_y = 0.5f, float scale_x = 1.0f, float scale_y = 1.0f);

	void set_autoscale(bool auto_scale_x = true, bool auto_scale_y = true) 
	{ 
		x_axis_is_auto_scaled = auto_scale_x; 
		y_axis_is_auto_scaled = auto_scale_y; 
	}
	
	void add_x_grid(float pos, float width = 0.6f, COLORREF color = RT_RED, LineStyle style = Solid);
	void add_y_grid(float pos, float width = 0.6f, COLORREF color = RT_RED, LineStyle style = Solid);

	// create a line serie and add to line stack
	LINE_SERIES* create_line_series(CString name, COLORREF color = RT_BLACK, float width = 1.f, LineStyle style = LineStyle::Solid);

	void draw();


protected:
	ID2D1SolidColorBrush* get_brush(COLORREF color, float opacity = 1.0f);
	ID2D1StrokeStyle* get_stroke_style(LineStyle s);
	void init_variables();
	void attach(CWnd *pWnd);
	void init_render_target();
	void create_brush_and_line_stroke_styles();

	void build_rects_for_draw();
	void build_write_text_format_and_layouts();
	void draw_fixed();
	void draw_line_series(
		LINE_SERIES *line_series, float x_min, float x_max, float y_min, float y_max);	
	void draw_graph();	// 데이터 라인, 그리드
	void draw_title_and_legend();

	void draw_alarm_msg_box();
};

class RT_GRAPH_ARRAY : public CWnd
{
public:
	RT_GRAPH_ARRAY();
	virtual ~RT_GRAPH_ARRAY();

private:
	typedef struct{
		int row;
		int col;
	}GraphPosition;

	typedef struct{
		RT_GRAPH *pGraph;
		bool is_merged = false;
		bool is_shown = true;
		D2D1_RECT_F rect;  // base rect 정의
		GraphPosition merged_to;
	}GraphCell; // 개별 그래프

	int rows = 0;	// row size 
	int cols = 0;	// col size
	
	vector <GraphCell> serialized_graph_stack;

	vector<float> row_weight;
	vector<float> col_weight;

	float    m_border_width = 1.f;
	COLORREF m_border_color = RT_BLACK;// RT_MEDIUM_GREY;
	bool m_show_border = false;
	float	 m_divider_width ;
	COLORREF m_divider_color = RT_BLACK;//RT_MEDIUM_GREY;
	bool m_show_divider = false;
	float m_gap_btw_graph;

	bool is_x_synchronized = false;
	bool is_y_synchronized = false;

	D2D1_RECT_F mBaseRect;
	CString m_font_face;
	COLORREF m_bg_color;

	float m_x_pixel_scale;
	float m_y_pixel_scale;

	ID2D1Factory *D2D_Factory = NULL ;
	ID2D1DCRenderTarget *RenderTarget = NULL;
	IDWriteFactory	*write_factory = NULL;


	CDC *memCDC = NULL;
	CDC *pDC;
	//CImage client_image;
	CBitmap *m_pBitmap = NULL, mBitmap;

	ID2D1SolidColorBrush *m_brush;  // 그래프를 그리면서 공용으로 사용할 브러쉬: 특성을 바꿔가며 사용

	void init_variables();
	void init();
	void build_base_rect_of_cells();
	void create_graph_cells();
	void draw_border_and_divider();
	ID2D1SolidColorBrush* get_brush(COLORREF color, float opacity = 1.0f);
public:
	void init_graph_array(int row_size = 1, int col_size = 1, int gap_btw_graph = 8, int devider_width = 1);
	void set_row_weight(int row, float w = 1.0f);
	void set_col_weight(int col, float w = 1.0f);

	RT_GRAPH* get_graph(int row, int col);

	// note: this function can be used only when initialization is done.
	void set_font_of_all_cells(CString face_name);
	void set_bg_color(COLORREF color);
	void set_border(int width, COLORREF color);
	void set_divider(int width, COLORREF color);
	void show_border(bool on = true) { m_show_border = on; }
	void show_divider(bool on = true) { m_show_divider = on; }
	void draw();
	bool save(CString filename, const GUID &filetype = Gdiplus::ImageFormatPNG);

	void synchronize(bool sync_x = true, bool sync_y = false);

	DECLARE_DYNAMIC(RT_GRAPH_ARRAY)
private:

	// For Zooming
	bool  LB_down = false;
	CPoint LB_down_pos;

	// For Panning
	bool  RB_down = false;
	CPoint RB_down_pos;
	
	int	graph_selected;

	CBrush m_dc_brush;

	BOOL RegisterWindowClass();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

