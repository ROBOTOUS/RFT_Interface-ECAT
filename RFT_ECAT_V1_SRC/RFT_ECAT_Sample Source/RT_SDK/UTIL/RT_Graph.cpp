
#include "RT_Graph.h"
//#include "RT_UserInterface.h"
#include "RT_Direct2D_Utility.h"
#include <atlimage.h> // for CImage

#include <dwrite.h>
#pragma comment(lib, "dwrite.lib")

#include <vector>
#include <string>

//  변환에 관한 노트 -------------------------------------------------------------
//  모든 변환은 좌표 변환이 아니다. 실제 기하를 절대 원점기준(RenderTarget의 원점) 으로 바꿔 주는 것이다.
//	모든 변환의 기준 점이 절대원점이란 말이고, 변환에 의해 원점이 이동하는 개념이 아니니 주의 할 것
//	참고로 회전 등의 변환에서 기준점은 원점이다.

using namespace std;
using namespace D2D1;

ColorF get_D2D1_color(COLORREF color)
{
	UINT32 r = GetRValue(color);
	UINT32 g = GetGValue(color);
	UINT32 b = GetBValue(color);

	return ColorF(((r << 16) & 0xFF0000) | ((g << 8) & 0xFF00) | (b & 0xFF));
}
D2D1_RECT_F resized_rect(const D2D1_RECT_F& rect, float x, float y)
{
	D2D1_RECT_F r;

	r.left = rect.left - x;
	r.right = rect.right + x;
	r.top = rect.top - y;
	r.bottom = rect.bottom + y;

	return r;
}
D2D1_RECT_F moved_rect(const D2D1_RECT_F& rect, float x, float y)
{
	D2D1_RECT_F r;

	r.left = rect.left + x;
	r.right = rect.right + x;
	r.top = rect.top + y;
	r.bottom = rect.bottom + y;

	return r;
}

//RT_GRAPH::RT_GRAPH(CWnd *pWnd)
//{
//	// 변수 초기화
//	init_variables();
//
//	// 라인스택 초기화
//	line_stack.clear();
//
//	attach(pWnd);
//	init_render_target();
//	create_brush_and_line_stroke_styles();
//
//	// 그래프를 그리기 위한 모든 rect를 드로우영역의 원점 기준으로 기준으로 계산
//	build_rects_for_draw();
//	// 텍스트 출력을 위한 기본 포맷 설정
//	build_write_text_format_and_layouts();
//
//
//	//RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // --> 이렇게 하면 안티 앨리어싱이 해제
//}

RT_GRAPH::RT_GRAPH(
	ID2D1Factory *factory, 
//	ID2D1HwndRenderTarget *render_target,
	ID2D1DCRenderTarget *render_target,
	D2D_RECT_F base_rect,
	float pixel_scale_x, float pixel_scale_y)
{
	// 변수 초기화
	init_variables();

	// 라인스택 초기화
	line_stack.clear();

	D2D_Factory = factory;
	RenderTarget = render_target;
	mBaseRect = base_rect;

	// 시작시 클라이언트 영역과 동일하게 드로우 영역을 설정
	mDrawRect = mBaseRect;

	// pixel scale
	m_x_pixel_scale = pixel_scale_x;
	m_y_pixel_scale = pixel_scale_y;
	
	create_brush_and_line_stroke_styles();

	// 그래프를 그리기 위한 모든 rect를 드로우영역의 원점 기준으로 기준으로 계산
	build_rects_for_draw();
	// 텍스트 출력을 위한 기본 포맷 설정
	build_write_text_format_and_layouts();
}
void RT_GRAPH::init_variables()
{
	m_text_color = RT_BLACK;
	m_graph_bg_color = RT_WHITE;

	border.width = 0.f;
	border.color = RT_BLACK;

	graph_border.color = RT_DARK_GREY;
	graph_border.width = 1.f;

	graph_title.style = Bold;
	x_axis.title.style = Bold;
	y_axis.title.style = Bold;

	x_axis.grid.color = RT_GREY;
	x_axis.grid.width = 0.6f;
	x_axis.grid.style = LineStyle::Solid;

	y_axis.grid.color = RT_GREY;
	y_axis.grid.width = 0.6f;
	y_axis.grid.style = LineStyle::Solid;

	legend_text_style = Bold;
	legend_text_color = RT_BLACK;

	m_brush_color = RT_WHITE;
	m_brush = NULL;

	m_font_face = _T("맑은 고딕");

	is_zoomed = false;
}
RT_GRAPH::~RT_GRAPH()
{
	for (auto& pLS:line_stack)
	{
		if (pLS)
		{
			delete pLS;
			pLS = NULL;
		}
	}
	line_stack.clear();

	SafeRelease(&m_brush);

	// release write text format
	SafeRelease(&wtf_alarm_msg);
	SafeRelease(&wtf_x_axis_tick_label);
	SafeRelease(&wtf_y_axis_tick_label);
	SafeRelease(&wtf_legend_label);
	
	// release write text layout
	SafeRelease(&wtl_graph_title);
	SafeRelease(&wtl_x_axis_title);
	SafeRelease(&wtl_y_axis_title);

	// Release Line Stroke Style
	SafeRelease(&SS_Dash);
	SafeRelease(&SS_Dot);
	SafeRelease(&SS_DashDot);
}

void RT_GRAPH::create_brush_and_line_stroke_styles()
{
	// 브러쉬 최초생성
	RenderTarget->CreateSolidColorBrush(
		//D2D1::ColorF(D2D1::ColorF::Red), 
		get_D2D1_color(m_brush_color),
		&m_brush);

	// 라인 그리기용 스트로크 스타일 생성
	// dash
	D2D1_STROKE_STYLE_PROPERTIES strokeStyleProperties = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_FLAT,  // The start cap.
		D2D1_CAP_STYLE_FLAT,  // The end cap.
		D2D1_CAP_STYLE_FLAT, // The dash cap.
		D2D1_LINE_JOIN_MITER, // The line join.
		10.0f, // The miter limit.
		D2D1_DASH_STYLE_DASH, // The dash style.
		0.0f // The dash offset.
		);
	D2D_Factory->CreateStrokeStyle(strokeStyleProperties, NULL, 0, &SS_Dash);
	// dot
	strokeStyleProperties = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_ROUND,  // The start cap.
		D2D1_CAP_STYLE_ROUND,  // The end cap.
		D2D1_CAP_STYLE_ROUND, // The dash cap.
		D2D1_LINE_JOIN_MITER, // The line join.
		10.0f, // The miter limit.
		D2D1_DASH_STYLE_DOT, // The dash style.
		0.0f // The dash offset.
		);
	D2D_Factory->CreateStrokeStyle(strokeStyleProperties, NULL, 0, &SS_Dot);
	// dot
	strokeStyleProperties = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_ROUND,  // The start cap.
		D2D1_CAP_STYLE_ROUND,  // The end cap.
		D2D1_CAP_STYLE_ROUND, // The dash cap.
		D2D1_LINE_JOIN_MITER, // The line join.
		10.0f, // The miter limit.
		D2D1_DASH_STYLE_DASH_DOT, // The dash style.
		0.0f // The dash offset.
		);
	D2D_Factory->CreateStrokeStyle(strokeStyleProperties, NULL, 0, &SS_DashDot);
}

void RT_GRAPH::set_border(float width, COLORREF color)
{
	border.width = (float)width;
	border.color = color;
	build_rects_for_draw();
}

void RT_GRAPH::set_graph_border(float width, COLORREF color)
{
	graph_border.width = (float)width;
	graph_border.color = color;
	build_rects_for_draw();
}


void RT_GRAPH::set_title(CString title, int height, COLORREF color, TEXT_STYLE style, COLORREF bg_color)
{
	graph_title.set_text(title, color, style,
		(bg_color == COLOR_NOT_DEFINED) ? m_bg_color : bg_color);
	graph_title_height = (float)height;

	build_rects_for_draw();
	build_write_text_format_and_layouts();
}

void RT_GRAPH::set_x_axis_title(CString title, int height, COLORREF color, TEXT_STYLE style, COLORREF bg_color)
{
	x_axis.title.set_text(title, color, style,
		(bg_color == COLOR_NOT_DEFINED) ? m_bg_color : bg_color);
	x_axis.title_height = (float)height;

	build_rects_for_draw();
	build_write_text_format_and_layouts();
}

void RT_GRAPH::set_y_axis_title(CString title, int height, COLORREF color, TEXT_STYLE style, COLORREF bg_color)
{
	y_axis.title.set_text(title, color, style,
		(bg_color == COLOR_NOT_DEFINED) ? m_bg_color : bg_color);
	y_axis.title_height = (float)height;

	build_rects_for_draw();
	build_write_text_format_and_layouts();
}
void RT_GRAPH::set_x_axis_ticks(int num_of_ticks, int tick_label_height, int tick_label_width, TEXT_STYLE style, COLORREF tick_label_color)
{
	x_axis.num_of_ticks = (num_of_ticks>3)?num_of_ticks:3;
	x_axis.tick_label_height = (tick_label_height > 6) ? (float)tick_label_height : (float)6;
	x_axis.tick_label_width = (float)tick_label_width;
	x_axis.tick_label_style = style;
	x_axis.tick_text_color = tick_label_color;

	build_rects_for_draw();
	build_write_text_format_and_layouts();
}
void RT_GRAPH::set_y_axis_ticks(int num_of_ticks, int tick_label_height, int tick_label_width, TEXT_STYLE style, COLORREF tick_label_color)
{
	y_axis.num_of_ticks = (num_of_ticks>2) ? num_of_ticks : 2;
	y_axis.tick_label_height = (tick_label_height > 6) ? (float)tick_label_height : (float)6;
	y_axis.tick_label_width = (float)tick_label_width;
	y_axis.tick_label_style = style;
	y_axis.tick_text_color = tick_label_color;

	build_rects_for_draw();
	build_write_text_format_and_layouts();
}

void  RT_GRAPH::set_legend(bool show, int height, int width)
{
	show_legend = show;
	legend_height = (float)height;
	legend_width = (float)width;

	build_rects_for_draw();
	build_write_text_format_and_layouts();
}
void  RT_GRAPH::set_x_axis_tick_increments(bool automatic, float increment)
{
	x_axis.tick_increment_auto = automatic;
	if (!automatic)
		x_axis.tick_increment = increment > 0 ? increment : -increment;
}
void  RT_GRAPH::set_y_axis_tick_increments(bool automatic, float increment)
{
	y_axis.tick_increment_auto = automatic;
	if (!automatic)
		y_axis.tick_increment = increment > 0 ? increment : -increment;
}

void  RT_GRAPH::set_scale(float x_scale, float y_scale)
{
	m_x_scale = x_scale;
	m_y_scale = y_scale;
}

void RT_GRAPH::set_default_text_color(COLORREF color)
{
	m_text_color = color;
}

void RT_GRAPH::set_graph_area_bg_color(COLORREF color)
{
	m_graph_bg_color = color;
}

void RT_GRAPH::set_bg_color(COLORREF color)
{
	m_bg_color = color;
}

void RT_GRAPH::set_font_face(CString font_face)
{
	m_font_face = font_face;

	build_write_text_format_and_layouts();
}

void RT_GRAPH::build_rects_for_draw()
{
	// 공간 분배를 위해 타이틀 존재 유무 확인
	if (!graph_title.activated)
		graph_title_height = 0;
	
	if (!x_axis.title.activated)
		x_axis.title_height = 0;
	
	if (!y_axis.title.activated)
		y_axis.title_height = 0;
	
	if (!show_legend)
	{
		legend_width = 0;
		legend_height = 0;
	}
	// 아래는 모두 드로우 영역 기준으로 계산
	// set title rect and legend rect 
	graph_title.rect.left = y_axis.title_height + y_axis.tick_label_width + border.width;
	graph_title.rect.top = ((graph_title_height > legend_height) ? 0 : legend_height - graph_title_height) + border.width;
	graph_title.rect.bottom = ((graph_title_height > legend_height) ? graph_title_height : legend_height) + border.width;
	graph_title.rect.right = mDrawRect.right - mDrawRect.left - legend_width* line_stack.size() - x_axis.tick_label_width / 2.f - border.width;

	legend_rect.bottom = graph_title.rect.bottom;
	legend_rect.top = legend_rect.bottom - legend_height;
	legend_rect.right = mDrawRect.right - mDrawRect.left - x_axis.tick_label_width / 2.f - border.width;
	legend_rect.left = legend_rect.right - legend_width * line_stack.size();

	// set x axis rect
	x_axis.title.rect.bottom = mDrawRect.bottom - mDrawRect.top - border.width;
	x_axis.title.rect.top = x_axis.title.rect.bottom - x_axis.title_height;
	x_axis.title.rect.right = legend_rect.right;
	x_axis.title.rect.left = graph_title.rect.left;

	// set y axis rect
	y_axis.title.rect.left = border.width;
	y_axis.title.rect.top = legend_rect.bottom;;
	y_axis.title.rect.right = border.width + y_axis.title_height;
	y_axis.title.rect.bottom = x_axis.title.rect.top - x_axis.tick_label_height;

	// 드로우 영역 기준 그래프영역 찾기
	mGraphRectAtDraw.left = graph_title.rect.left + graph_border.width;
	mGraphRectAtDraw.top = legend_rect.bottom + graph_border.width;
	mGraphRectAtDraw.right = x_axis.title.rect.right - graph_border.width;
	mGraphRectAtDraw.bottom = y_axis.title.rect.bottom - graph_border.width;
}
CRect RT_GRAPH::get_graph_rect()
{
	CRect r_g;
	r_g.left	= (LONG)(mBaseRect.left + mGraphRectAtDraw.left		+ 0.5f);
	r_g.top		= (LONG)(mBaseRect.top  + mGraphRectAtDraw.top		+ 0.5f);
	r_g.right	= (LONG)(mBaseRect.left + mGraphRectAtDraw.right	+ 0.5f);
	r_g.bottom	= (LONG)(mBaseRect.top  + mGraphRectAtDraw.bottom	+ 0.5f);

	return r_g;
}
void RT_GRAPH::zoom(bool on, float center_x, float center_y, float scale_x,	float scale_y)
{
	if (on)
	{
		if ((zoom_scale_x > 0.9e-4f) && (zoom_scale_y > 0.9e-4f))  // 1/10,000 이하의 스케일은 허용 안함
		{
			zoom_center_x = zoom_center_x + zoom_scale_x*(center_x - 0.5f);
			zoom_center_y = zoom_center_y + zoom_scale_y*(center_y - 0.5f);

			zoom_scale_x *= scale_x;
			zoom_scale_y *= scale_y;
			if (zoom_scale_x < 1.e-4f)
				zoom_scale_x = 1.e-4f;
			if (zoom_scale_y < 1.e-4f)
				zoom_scale_y = 1.e-4f;
		}
		is_zoomed = true;
	}
	else
		is_zoomed = false;
}
void RT_GRAPH::add_x_grid(float pos, float width , COLORREF color, LineStyle style)
{
	ExtraGrid extra_grid;
	extra_grid.pos = pos;
	extra_grid.width = width;
	extra_grid.color = color;
	extra_grid.style = style;
	x_axis.extra_grid_stack.push_back(extra_grid);
}
void RT_GRAPH::add_y_grid(float pos, float width, COLORREF color, LineStyle style)
{
	ExtraGrid extra_grid;
	extra_grid.pos = pos;
	extra_grid.width = width;
	extra_grid.color = color;
	extra_grid.style = style;
	y_axis.extra_grid_stack.push_back(extra_grid);
}

// create a line serie and add to line stack
LINE_SERIES* RT_GRAPH::create_line_series(CString name, COLORREF color, float width, LineStyle style)
{
	LINE_SERIES *pLS;

	line_stack.push_back(pLS);
	line_stack[line_stack.size() - 1] = new LINE_SERIES;

	line_stack[line_stack.size() - 1]->name = name;
	line_stack[line_stack.size() - 1]->set_width_and_style(width, style);
	line_stack[line_stack.size() - 1]->set_color(color);
//	line_stack[line_stack.size() - 1]->set_d2d_factory(D2D_Factory);

	build_rects_for_draw();
	return line_stack[line_stack.size() - 1];
}

ID2D1SolidColorBrush* RT_GRAPH::get_brush(COLORREF color, float opacity)
{
	m_brush->SetColor(get_D2D1_color(color));
	saturate(opacity, 0.0f, 1.0f);
	m_brush->SetOpacity(opacity);

	return m_brush;
}
ID2D1StrokeStyle* RT_GRAPH::get_stroke_style(LineStyle s)
{
	if (s == LineStyle::Solid)
		return NULL;
	else if (s == LineStyle::Dash)
		return SS_Dash;
	else if (s == LineStyle::Dot)
		return SS_Dot;
	else if (s == LineStyle::DashDot)
		return SS_DashDot;
	else // default solid
		return NULL;
}

void RT_GRAPH::draw()
{
	is_alarmed = false;
	alarm_msg = _T("");

	transform_to_draw = 
		Matrix3x2F::Scale(m_x_pixel_scale, m_y_pixel_scale)*
		Matrix3x2F::Translation(mBaseRect.left*m_x_pixel_scale, mBaseRect.top*m_y_pixel_scale);
	// 그래프 영역으로의 변환 매트릭스 계산 - Translation
	D2D1_POINT_2F t = Point2F(mGraphRectAtDraw.left, mGraphRectAtDraw.top);
	transform_to_graph = transform_to_draw * Matrix3x2F::Translation(t.x*m_x_pixel_scale, t.y*m_y_pixel_scale);

	D2D1_RECT_F base_rect;
	base_rect.top = base_rect.left = 0.f;
	base_rect.right = mBaseRect.right - mBaseRect.left;
	base_rect.bottom = mBaseRect.bottom - mBaseRect.top;

	RenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
	RenderTarget->SetTransform(transform_to_draw);
	RenderTarget->PushAxisAlignedClip(base_rect, D2D1_ANTIALIAS_MODE_ALIASED);

	draw_fixed();
	draw_graph();
	
	RenderTarget->SetTransform(transform_to_draw);
	RenderTarget->PopAxisAlignedClip();  // 변환에 영향을 받지 않는다. 그냥 클립된 그대로 다시 그린다.

	if (is_alarmed)
		draw_alarm_msg_box();
}

void RT_GRAPH::draw_fixed()
{
	// 일단 사용영역을 시스템 배경색으로 그린다.
	D2D1_RECT_F base_rect;
	base_rect.top = base_rect.left = 0.f;
	base_rect.right = mBaseRect.right - mBaseRect.left;
	base_rect.bottom = mBaseRect.bottom - mBaseRect.top;

	// 보더를 깔끔하게 그리기 위해 일단 안티앨리어싱 모드들 off
	RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	RenderTarget->FillRectangle(base_rect, get_brush(m_bg_color));
	RenderTarget->DrawRectangle(
		resized_rect(base_rect,-border.width/2.f, -border.width/2.f), get_brush(border.color), border.width);
	RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	// 그래프 영역 내부 백그라운드를 그린다.
	RenderTarget->FillRectangle(mGraphRectAtDraw, get_brush(m_graph_bg_color));

	// 타이틀과 레전드를 그린다.
	draw_title_and_legend();

	// 그래프 보더를 여기서 그리면 좋지만 그리드가 덮는 경우가 생겨 그래프를 다그리고 그리는 것으로 변경
}

void RT_GRAPH::draw_line_series(
	LINE_SERIES *line_series,float x_min, float x_max, float y_min, float y_max)	
{
	// 변환을 쓰면 느려진다. ?? 특히 스케일을 쓰면 느려지니 가능한 안쓰는 것이 속도면에서 유리
	// TransformGeometry를 사용하였으나 오히려 늦어짐. 
	// 그나마 바로 그리지 않고 geometry를 생성해서 그리는 것이 빠름.

	if (line_series->data_size() < 2) // 그릴 포인트가 2개 이상이어야 한다.
		return;

	float scale_x = (mGraphRectAtDraw.right - mGraphRectAtDraw.left) / (x_max - x_min);
	float scale_y = (mGraphRectAtDraw.bottom - mGraphRectAtDraw.top) / (y_max - y_min);
	float graph_height = mGraphRectAtDraw.bottom - mGraphRectAtDraw.top;

	float *x = line_series->get_x_data_pointer();
	float *y = line_series->get_y_data_pointer();
	int data_size = line_series->data_size();

	float width = line_series->get_width();
	ID2D1SolidColorBrush* brush = get_brush(line_series->get_color(), line_series->get_opacity());
	ID2D1StrokeStyle *stroke_style = get_stroke_style(line_series->get_stroke_style());

	int start_idx = 0;
	float draw_interval = 1.0f;
	
	// 정렬된 데이터인 경우, 데이터가 많으면....
	if ((line_series->data_is_in_order()) && (data_size > 1500) ) 
	{
		// Calculate Draw Interval : 라인폭을 기준으로 그려야할 간격을 x축의 단위로 계산
		draw_interval = width / scale_x;

		// Find first data to draw
		start_idx = 0;
		while (x[start_idx] < x_min)
		{
			if (++start_idx >= data_size)
				break;
		}

		int idx = start_idx > 0 ? start_idx - 1 : 0; // 직전 부터 시작
		int last_idx = idx;
		D2D1_POINT_2F start;
		D2D1_POINT_2F end	// 마지막으로 선을 그린 엔드 포인트 
			= Point2F((x[idx] - x_min) * scale_x, graph_height - (y[idx] - y_min) *scale_y);
		D2D1_RECT_F rect_d;			// 그리는 렉트
		float h_to_width = width / 2.0f; // 렉트 그릴때 두께 확보

		static float last_min = y[idx];
		static float last_max = last_min;

		while (x[idx] < x_max)
		{
			int data_num_in_interval = 0;  // 선폭 사이의 데이터 수
			// 다음 인덱스를 찾는다.
			float next_x = x[last_idx] + draw_interval;
			do{
				if (idx >= data_size - 1)
					break;
			} while (x[++idx] < next_x);
			data_num_in_interval = idx - last_idx;

			if ((idx == last_idx) && (idx == (data_size - 1))) // 데이터 최종 위치 도달
				break;

			if (data_num_in_interval > 1)
			{
				rect_d.left = (x[last_idx] - x_min)*scale_x;
				rect_d.right = rect_d.left + width;
				float min, max;
				get_min_max(min, max, &y[last_idx], data_num_in_interval);

					if (y[last_idx] < min) // 상방 분리으로 분리되는 경우 (.') 
					{
						rect_d.top = graph_height - (max - y_min)*scale_y;
						rect_d.bottom = graph_height - (y[last_idx] - y_min)*scale_y;
					}
					else if (y[last_idx] > max) // 하방 분리으로 분리되는 경우 ('.)
					{
						rect_d.top = graph_height - (y[last_idx] - y_min)*scale_y;
						rect_d.bottom = graph_height - (min - y_min)*scale_y;
					}
					else
					{
						rect_d.top = graph_height - (max - y_min)*scale_y;
						rect_d.bottom = graph_height - (min - y_min)*scale_y;
					}

				RenderTarget->FillRectangle(
					RectF(rect_d.left, rect_d.top - h_to_width, rect_d.right, rect_d.bottom + h_to_width),
					brush);
				start = Point2F((x[idx-1] - x_min) * scale_x, graph_height - (y[idx-1] - y_min) *scale_y);
				end = Point2F((x[idx] - x_min) * scale_x, graph_height - (y[idx] - y_min) *scale_y);
				RenderTarget->DrawLine(start, end, brush, width, stroke_style);

				last_min = min;
				last_max = max;
			}
			else // 인터벌 안에 데이터가 1개면
			{
				start = end;
				end = Point2F((x[idx] - x_min) * scale_x, graph_height - (y[idx] - y_min) *scale_y);
				RenderTarget->DrawLine(start, end, brush, width, stroke_style);
			}
			last_idx = idx;
		}
	}
	// 데이터가 적거나, 정렬된 데이터가 아니면
	else  
	{
		// 라인을 그리기 위한 Geometry 생성
		ID2D1PathGeometry *line_geometry;
		D2D_Factory->CreatePathGeometry(&line_geometry);
		ID2D1GeometrySink *pSink;

		line_geometry->Open(&pSink);
		// 라인을 직접그리는 것보다, Geometry를 이용해서 그리는 것이 빠르다.
		pSink->BeginFigure(
			Point2F((x[0] - x_min) * scale_x, graph_height - (y[0] - y_min) * scale_y),
			D2D1_FIGURE_BEGIN_HOLLOW);

		D2D1_POINT_2F *pP = new D2D1_POINT_2F[data_size];
		for (int i = 1; i < data_size; i++)
		{
			pP[i] =	Point2F((x[i] - x_min) * scale_x, graph_height - (y[i] - y_min) * scale_y);
		}
		// 하나씩 추가하는 것보다 이렇게 한꺼번에 하는 것이 빠르다.
		pSink->AddLines(&pP[1], data_size - 1);
		pSink->EndFigure(D2D1_FIGURE_END_OPEN);
		pSink->Close();

		RenderTarget->DrawGeometry(line_geometry, brush, width, stroke_style);
		delete[] pP;
		SafeRelease(&line_geometry);
	}
	line_series->has_been_drawn();
}

// 전체 데이터의 최소/최대값과 그려야할 최대 틱수를 받아 틱 증가량과 축의 전체 레인지를 계산하여 반환
// 입력:			num_of_ticks
// 입력 후 수정:	min, max
// 출력:			tick_inc, range
void calculate_tick_inc_and_axis_range(float& tick_inc, float& range, int num_of_ticks, float& min, float& max)
{
	float log_value;
	float log_first_digit;
	int exponent;

	// 레인지 최초 계산 --> 계산 중 바뀔 수 있음.
	range = max - min;
	// step 1: tick increment를 자동으로 계산하는 모드이면 값을 계산
	if (range > 0)
	{
		tick_inc = range / (float)(num_of_ticks - 1);
		log_value = log10(tick_inc);
		exponent = (int)floorf(log_value);
		log_first_digit = log_value - floorf(log_value);
		if (log_first_digit < 0.1760912f) // 1.5
		{
			tick_inc = 1.f * pow(10.f, (int)(exponent));
		}
		else if (log_first_digit < 0.5740312f) // 3.75
		{
			tick_inc = 2.f * pow(10.f, (int)(exponent));
		}
		else if (log_first_digit < 0.8750612f) // 7.5
		{
			tick_inc = 5.f * pow(10.f, (int)(exponent));
		}
		else
		{
			tick_inc = 10.f * pow(10.f, (int)(exponent));
		}
	}
	else
	{
		float abs = fabs(max);
		float val_center = max;
		if (abs > 10.f)
		{
			tick_inc = 1.1f;
			max = val_center + tick_inc * (float)(num_of_ticks / 2);
			min = val_center - tick_inc * (float)(num_of_ticks / 2);
		}
		if (abs > 1.f)
		{
			tick_inc = 0.1f;
			max = val_center + tick_inc * (float)(num_of_ticks / 2);
			min = val_center - tick_inc * (float)(num_of_ticks / 2);
		}
		else if (abs > 0.1f)
		{
			tick_inc = 0.01f;
			max = val_center + tick_inc * (float)(num_of_ticks / 2);
			min = val_center - tick_inc * (float)(num_of_ticks / 2);
		}
		else
		{
			tick_inc = 0.001f;
			max = val_center + tick_inc * (float)(num_of_ticks / 2);
			min = val_center - tick_inc * (float)(num_of_ticks / 2);
		}
		range = max - min;
	}

}

// 틱 증가량을 받아 틱 레이블의 포맷을 반환
// 입력: tick_inc
// 반환: tick_label_format
CString get_tick_label_format(float& tick_inc)
{
	CString tick_lable_format;

	if (tick_inc > 1000001.f)
		tick_lable_format = _T("%.1e");
	else if (tick_inc > 0.999)
		tick_lable_format = _T("%.0f");
	else if (tick_inc > 0.0999f)
		tick_lable_format = _T("%.1f");
	else if (tick_inc > 0.00999f)
		tick_lable_format = _T("%.2f");
	else if (tick_inc > 0.000999f)
		tick_lable_format = _T("%.3f");
	else if (tick_inc > 0.0000999f)
		tick_lable_format = _T("%.4f");
	else
		tick_lable_format = _T("%.5f");

	//else if (tick_inc > 0.00000999f)
	//	tick_lable_format = _T("%.5f");
	//else if (tick_inc > 0.000000999f)
	//	tick_lable_format = _T("%.6f");
	//else if (tick_inc > 0.0000000999f)
	//	tick_lable_format = _T("%.7f");
	//else
	//	tick_lable_format = _T("%f");

	return tick_lable_format;
}
void RT_GRAPH::draw_graph()
{
	if (line_stack.size() < 1) // 그릴 것이 없다.
	{
		return; 
	}

	// 드로우 영역으로의 변환매트릭스 설정
	RenderTarget->SetTransform(transform_to_draw);

	// 그래프 영역의 x y 최대/최소값 결정 - zoom 모드가 아니고 자동 모드일때만

	if (x_axis_is_auto_scaled)
	{
		x_axis.min = line_stack[0]->x_min;
		x_axis.max = line_stack[0]->x_max;

		for (auto& pLS : line_stack)
		{
			updateMin(x_axis.min, pLS->x_min);
			updateMax(x_axis.max, pLS->x_max);
		}
	}
	if (y_axis_is_auto_scaled)
	{
		y_axis.min = line_stack[0]->y_min;
		y_axis.max = line_stack[0]->y_max;

		for (auto& pLS : line_stack)
		{
			updateMin(y_axis.min, pLS->y_min);
			updateMax(y_axis.max, pLS->y_max);
		}
	}

	// range ->  범위의 크기
	float x_range;
	float y_range;

	// 줌기능을 위해 임시 변수 사용
	float x_axis_min;
	float x_axis_max;
	float y_axis_min;
	float y_axis_max;

	if (!is_zoomed)
	{
		x_axis_min = x_axis.min;
		x_axis_max = x_axis.max;
		y_axis_min = y_axis.min;
		y_axis_max = y_axis.max;

		x_range = x_axis_max - x_axis_min;
		y_range = y_axis_max - y_axis_min;

		zoom_center_x = 0.5f;
		zoom_center_y = 0.5f;
		zoom_scale_x = 1.0f;
		zoom_scale_y = 1.0f;
	}
	else
	{
		float z_x_min = zoom_center_x - zoom_scale_x * 0.5f;
		float z_x_max = zoom_center_x + zoom_scale_x * 0.5f;
		float z_y_min = zoom_center_y - zoom_scale_y * 0.5f;
		float z_y_max = zoom_center_y + zoom_scale_y * 0.5f;

		x_axis_min = x_axis.min + (x_axis.max - x_axis.min) * z_x_min;
		x_axis_max = x_axis.min + (x_axis.max - x_axis.min) * z_x_max;
		y_axis_min = y_axis.min + (y_axis.max - y_axis.min) * z_y_min;
		y_axis_max = y_axis.min + (y_axis.max - y_axis.min) * z_y_max;

		x_range = x_axis_max - x_axis_min;
		y_range = y_axis_max - y_axis_min;
	}

	// 틱, 틱레이블 그리기 ------------------------------------------------

	// step 1: tick increment를 자동으로 계산하는 모드이면 값을 계산
	// x 축
	if (x_axis.tick_increment_auto)
	{
		calculate_tick_inc_and_axis_range(
			x_axis.tick_increment, x_range, x_axis.num_of_ticks, x_axis_min, x_axis_max );
	}
	// y 축
	if (y_axis.tick_increment_auto) 
	{
		calculate_tick_inc_and_axis_range(
			y_axis.tick_increment, y_range, y_axis.num_of_ticks, y_axis_min, y_axis_max);
	}

	// 틱 증가량에 따른 틱의 포맷 생성
	CString x_tl_format = get_tick_label_format(x_axis.tick_increment);
	CString y_tl_format = get_tick_label_format(y_axis.tick_increment);

	// 첫번째 틱값을 계산
	float x_axis_first_tick_value = floorf(x_axis_min / x_axis.tick_increment)*x_axis.tick_increment;
	if (x_axis_first_tick_value < x_axis_min)
		x_axis_first_tick_value += x_axis.tick_increment;
	
	float y_axis_first_tick_value = floorf(y_axis_min / y_axis.tick_increment)*y_axis.tick_increment;
	if (y_axis_first_tick_value < y_axis_min)
		y_axis_first_tick_value += y_axis.tick_increment;


	// 틱 레이블과 그리드를 그린다.
	float tick_value;
	CString tl;
	int tl_cnt;
	D2D1_POINT_2F t;
	Matrix3x2F to_tick_label;

	// x축
	t =Point2F(mGraphRectAtDraw.left, mGraphRectAtDraw.bottom + graph_border.width);
	to_tick_label = Matrix3x2F::Translation(t.x*m_x_pixel_scale, t.y*m_y_pixel_scale);
	tick_value = x_axis_first_tick_value;
	tl_cnt = 0;
	do
	{
		float tl_pos_x =
			(x_axis.tick_increment*(float)tl_cnt + 	(x_axis_first_tick_value - x_axis_min)) 
			* (mGraphRectAtDraw.right - mGraphRectAtDraw.left) / x_range;
		Matrix3x2F to_tl = Matrix3x2F::Translation(tl_pos_x*m_x_pixel_scale, 0.f);
		RenderTarget->SetTransform(transform_to_draw * to_tick_label * to_tl);
		// 그리드 그리기
		if (x_axis.show_grid)
		{
			RenderTarget->DrawLine(
				Point2F(0, -graph_border.width), 
				Point2F(0, -(mGraphRectAtDraw.bottom - mGraphRectAtDraw.top) - graph_border.width),
				get_brush(x_axis.grid.color), x_axis.grid.width,get_stroke_style(x_axis.grid.style));
		}
		// tick label 그리기
		_set_output_format(1); 
		tl.Format(x_tl_format, tick_value);

		D2D1_RECT_F r;
		r.left = -x_axis.tick_label_width / 2.f;
		r.top = 0;
		r.right = x_axis.tick_label_width / 2.f;
		r.bottom = x_axis.tick_label_height;

		RenderTarget->DrawText(tl, tl.GetLength(), wtf_x_axis_tick_label, r, get_brush(x_axis.tick_text_color));
				
		tick_value += x_axis.tick_increment;
		tl_cnt++;
	} while (tick_value <= x_axis_max);

	// y 축 
	t = Point2F(y_axis.title.rect.right, mGraphRectAtDraw.bottom - y_axis.tick_label_height / 2.f);
	to_tick_label =Matrix3x2F::Translation(t.x*m_x_pixel_scale,t.y*m_y_pixel_scale);

	tick_value = y_axis_first_tick_value;
	tl_cnt = 0;
	
	Matrix3x2F mod ;  // label 위치 수정용 - 차지 공간을 넘어가지 않도록

	float val_to_pos = (mGraphRectAtDraw.bottom - mGraphRectAtDraw.top) / y_range;
	do
	{
		float tl_y_pos = 
			-(y_axis.tick_increment*(float)tl_cnt + (y_axis_first_tick_value - y_axis_min)) * val_to_pos;;
		
		Matrix3x2F to_tl = Matrix3x2F::Translation(0.f, tl_y_pos*m_y_pixel_scale);
		RenderTarget->SetTransform(transform_to_draw * to_tick_label * to_tl);

		// 그리드 그리기
		if (y_axis.show_grid)
		{
			D2D1_POINT_2F start, end;
			start.x = y_axis.tick_label_width + graph_border.width;
			start.y = y_axis.tick_label_height / 2.f;
			end.x = start.x + mGraphRectAtDraw.right - mGraphRectAtDraw.left;
			end.y = start.y;
			RenderTarget->DrawLine(start, end,
				get_brush(y_axis.grid.color), y_axis.grid.width, get_stroke_style(y_axis.grid.style));
		}

		// tick label 그리기
		_set_output_format(1);  // e형식일때 지수 자리 지정
		tl.Format(y_tl_format, tick_value);
		D2D1_RECT_F r;
		r.left = 0; 
		r.top = 0;
		r.right = y_axis.tick_label_width - 1.0f;
		r.bottom = y_axis.tick_label_height;
		
		// 위치 수정
		if (tl_y_pos > (-y_axis.tick_label_height / 2.f))
		{
			mod = Matrix3x2F::Translation(0.f, -tl_y_pos - y_axis.tick_label_height / 2.f*m_y_pixel_scale);
		}
		else if (tl_y_pos < (mGraphRectAtDraw.top - mGraphRectAtDraw.bottom + y_axis.tick_label_height / 2.f))
		{
			mod = Matrix3x2F::Translation(0.f, 
				((mGraphRectAtDraw.top - mGraphRectAtDraw.bottom + y_axis.tick_label_height / 2.f) - tl_y_pos)*m_y_pixel_scale);
		}
		else
			mod = Matrix3x2F::Identity();

		Matrix3x2F t_c;
		RenderTarget->GetTransform(&t_c);
		RenderTarget->SetTransform(t_c * mod);

		RenderTarget->DrawText(tl, tl.GetLength(), wtf_y_axis_tick_label, r, get_brush(y_axis.tick_text_color));

		//RenderTarget->DrawRectangle(r, get_brush(RT_WHITE), 1);  //--> For Test

		tick_value += y_axis.tick_increment;
		tl_cnt++;
	} while (tick_value <= y_axis_max);

	// 추가 그리드 - Extra grid
	RenderTarget->SetTransform(transform_to_graph);
	float w_ratio = (mGraphRectAtDraw.right - mGraphRectAtDraw.left) / x_range;
	float h_ratio = (mGraphRectAtDraw.bottom - mGraphRectAtDraw.top) / y_range;

	// x축
	for (auto& ex_grid : x_axis.extra_grid_stack)
	{
		if (is_value_in_range(ex_grid.pos, x_axis_min, x_axis_max))
		{
			D2D1_POINT_2F start, end;
			start.x = (ex_grid.pos - x_axis_min)*w_ratio ;
			start.y = 0.f;
			end.x = start.x;
			end.y =  mGraphRectAtDraw.bottom - mGraphRectAtDraw.top;
			RenderTarget->DrawLine(start, end, get_brush(ex_grid.color), ex_grid.width, get_stroke_style(ex_grid.style));
		}
	}
	// y축
	for (auto& ex_grid : y_axis.extra_grid_stack)
	{
		if (is_value_in_range(ex_grid.pos, y_axis_min, y_axis_max))
		{
			D2D1_POINT_2F start, end;
			start.x = 0.f;
			start.y = mGraphRectAtDraw.bottom - mGraphRectAtDraw.top - (ex_grid.pos - y_axis_min)*h_ratio;
			end.x = mGraphRectAtDraw.right - mGraphRectAtDraw.left;
			end.y = start.y;
			RenderTarget->DrawLine(start, end, get_brush(ex_grid.color), ex_grid.width, get_stroke_style(ex_grid.style));
		}
	}

	// 영역 클립 - 클립할 영역을 정한다. Alias 모드는 나중에 합할때 적용되는 것이니 없애도 된다.
	RenderTarget->SetTransform(transform_to_draw);
	RenderTarget->PushAxisAlignedClip(mGraphRectAtDraw, D2D1_ANTIALIAS_MODE_ALIASED);

	// 데이터 라인 그리기
	RenderTarget->SetTransform(transform_to_graph); // 그래프 영역으로의 변환매트릭스 설정
	for (auto& pLS : line_stack)
	{
		if (pLS->is_shown())
		{
			if (pLS->data_size() > 1)
				draw_line_series(pLS, x_axis_min, x_axis_max, y_axis_min, y_axis_max);
			else
			{
				alarm_msg.Append("NO DATA:" + pLS->name + "\n");
				is_alarmed = true;
			}
		}
	}

	// 클립된 영역의 내용을 그린다.
	RenderTarget->PopAxisAlignedClip();

	// 그래프 보더를 그린다.
	RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	RenderTarget->SetTransform(transform_to_draw);
	RenderTarget->DrawRectangle(resized_rect(mGraphRectAtDraw, graph_border.width / 2, graph_border.width / 2),
		get_brush(graph_border.color), graph_border.width);
	RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}
DWRITE_FONT_WEIGHT get_D2D_font_weight(RT_GRAPH::TEXT_STYLE s)
{
	if (s == RT_GRAPH::TEXT_STYLE::Bold)
		return DWRITE_FONT_WEIGHT_BOLD;
	else if (s == RT_GRAPH::TEXT_STYLE::Normal)
		return DWRITE_FONT_WEIGHT_NORMAL;
	else if (s == RT_GRAPH::TEXT_STYLE::Narrow)
		return DWRITE_FONT_WEIGHT_NORMAL;
	else if (s == RT_GRAPH::TEXT_STYLE::NarrowBold)
		return DWRITE_FONT_WEIGHT_BOLD;
	else
		return DWRITE_FONT_WEIGHT_NORMAL;
}
DWRITE_FONT_STRETCH get_D2D_font_stretch(RT_GRAPH::TEXT_STYLE s)
{
	if (s == RT_GRAPH::TEXT_STYLE::Narrow)
		return DWRITE_FONT_STRETCH_CONDENSED;
	else if (s == RT_GRAPH::TEXT_STYLE::NarrowBold)
		return DWRITE_FONT_STRETCH_CONDENSED;
	else
		return DWRITE_FONT_STRETCH_NORMAL;
}
void RT_GRAPH::build_write_text_format_and_layouts()
{
	// 0. 일단 존재하는 text format을 삭제

	//if (write_factory)
	//	write_factory->Release();

	// Create DirectWrite factory. - 텍스트 쓰기와 관련된 인터페이스를 만드는 공장 
	if (write_factory == NULL)
	{
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(write_factory),
			reinterpret_cast<IUnknown **>(&write_factory));
	}

	DWRITE_FONT_WEIGHT w = DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_FONT_STYLE s = DWRITE_FONT_STYLE_NORMAL;
	DWRITE_FONT_STRETCH fs = DWRITE_FONT_STRETCH_NORMAL;

	// 1. build write text format

	// alarm message
	w = DWRITE_FONT_WEIGHT_BOLD;
	fs = DWRITE_FONT_STRETCH_NORMAL;
	write_factory->CreateTextFormat(m_font_face, NULL, w, s, fs,
		(mDrawRect.bottom - mDrawRect.top)/8.0f, (WCHAR *)(""), &wtf_alarm_msg);
	wtf_alarm_msg->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	wtf_alarm_msg->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
	wtf_alarm_msg->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// x axis tick label
	w = get_D2D_font_weight(x_axis.tick_label_style);
	fs = get_D2D_font_stretch(x_axis.tick_label_style);
	write_factory->CreateTextFormat(m_font_face, NULL, w, s, fs,
		x_axis.tick_label_height*ratio_height_to_font_size, (WCHAR *)(""), &wtf_x_axis_tick_label);
	wtf_x_axis_tick_label->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	wtf_x_axis_tick_label->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
	wtf_x_axis_tick_label->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// y axis tick label
	w = get_D2D_font_weight(y_axis.tick_label_style);
	fs = get_D2D_font_stretch(y_axis.tick_label_style);
	write_factory->CreateTextFormat(m_font_face, NULL, w, s, fs,
		x_axis.tick_label_height*ratio_height_to_font_size, (WCHAR *)(""), &wtf_y_axis_tick_label);
	wtf_y_axis_tick_label->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	wtf_y_axis_tick_label->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
	wtf_y_axis_tick_label->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// legend label
	if (show_legend)
	{
		w = get_D2D_font_weight(legend_text_style);
		fs = get_D2D_font_stretch(legend_text_style);
		write_factory->CreateTextFormat(m_font_face, NULL, w, s, fs,
			legend_height*ratio_height_to_font_size, (WCHAR *)(""), &wtf_legend_label);
		wtf_legend_label->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		wtf_legend_label->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
		wtf_legend_label->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// 2. build write text layout
	DWRITE_TEXT_RANGE textRange = { 0, 0 };

	if (graph_title.activated)
	{
		textRange.startPosition = 0;
		// graph title
		w = get_D2D_font_weight(graph_title.style);
		fs = get_D2D_font_stretch(graph_title.style);
	
		write_factory->CreateTextLayout(graph_title.m_text, graph_title.m_text.GetLength(),
			wtf_x_axis_tick_label,  // 무조건 생성되기 때문에 기본으로 이용
			graph_title.rect.right - graph_title.rect.left, graph_title.rect.bottom - graph_title.rect.top,
			&wtl_graph_title);
		wtl_graph_title->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		wtl_graph_title->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
		wtl_graph_title->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		textRange.length = graph_title.m_text.GetLength();
		wtl_graph_title->SetFontWeight(w, textRange);
		wtl_graph_title->SetFontStretch(fs, textRange);
		wtl_graph_title->SetFontSize(graph_title_height*ratio_height_to_font_size, textRange);
	}

	// x_axis title
	if (x_axis.title.activated)
	{
		w = get_D2D_font_weight(x_axis.title.style);
		fs = get_D2D_font_stretch(x_axis.title.style);

		write_factory->CreateTextLayout(x_axis.title.m_text, x_axis.title.m_text.GetLength(),
			wtf_x_axis_tick_label,	// 무조건 생성되기 때문에 기본으로 이용
			x_axis.title.rect.right - x_axis.title.rect.left, x_axis.title.rect.bottom - x_axis.title.rect.top,
			&wtl_x_axis_title);
		wtl_x_axis_title->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		wtl_x_axis_title->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
		wtl_x_axis_title->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		textRange.length = x_axis.title.m_text.GetLength();
		wtl_x_axis_title->SetFontWeight(w, textRange);
		wtl_x_axis_title->SetFontStretch(fs, textRange);
		wtl_x_axis_title->SetFontSize(x_axis.title_height*ratio_height_to_font_size, textRange);
	}
	if (y_axis.title.activated)
	{
		// y_axis title
		w = get_D2D_font_weight(y_axis.title.style);
		fs = get_D2D_font_stretch(y_axis.title.style);

		write_factory->CreateTextLayout(y_axis.title.m_text, y_axis.title.m_text.GetLength(),
			wtf_x_axis_tick_label,	// 무조건 생성되기 때문에 기본으로 이용
			// 텍스트 그린 후 회전 하는 방식으로 바꿈
			//y_axis.title.rect.right - y_axis.title.rect.left, y_axis.title.rect.bottom - y_axis.title.rect.top,
			y_axis.title.rect.bottom - y_axis.title.rect.top, y_axis.title.rect.right - y_axis.title.rect.left,
			&wtl_y_axis_title);
		wtl_y_axis_title->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		wtl_y_axis_title->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
		wtl_y_axis_title->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		
		// 텍스트 방향 조정 - 윈도우 8 이상에서만 사용가능하여 그릴때 회전 시키는 방법으로 전환
		//wtl_y_axis_title->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
		//wtl_y_axis_title->SetFlowDirection(DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT);
		textRange.length = y_axis.title.m_text.GetLength();
		wtl_y_axis_title->SetFontWeight(w, textRange);
		wtl_y_axis_title->SetFontStretch(fs, textRange);
		wtl_y_axis_title->SetFontSize(y_axis.title_height*ratio_height_to_font_size, textRange);
	}
}

void RT_GRAPH::draw_title_and_legend()
{

	D2D1_POINT_2F p;
	if (graph_title.activated)
	{
		p = Point2F(graph_title.rect.left, graph_title.rect.top);
		RenderTarget->DrawTextLayout(p,wtl_graph_title,get_brush(graph_title.m_text_color));
		// for test
		//RenderTarget->DrawRectangle(graph_title.rect, get_brush(RT_BLACK), 0.5);
	}
	if (x_axis.title.activated)
	{
		p = Point2F(x_axis.title.rect.left, x_axis.title.rect.top);
		RenderTarget->DrawTextLayout(p, wtl_x_axis_title, get_brush(x_axis.title.m_text_color));
		// for test
		// RenderTarget->DrawRectangle(x_axis.title.rect, get_brush(RT_BLACK), 0.5);	
	}
	if (y_axis.title.activated)
	{
		// 이동 -> 회전 변환을 사용하여 타이틀을 그린다.
		// 시계 방향 90도 돌려 타이틀을 그릴 예정이기 때문에 중심점은 타이틀 블록의 오른쪽 탑이 된다.
		p = Point2F(y_axis.title.rect.right * m_x_pixel_scale, y_axis.title.rect.top * m_y_pixel_scale);
		Matrix3x2F t =Matrix3x2F::Translation(p.x, p.y);

		// 타이틀 블록의 원점을 Render Target 기준으로 회전 중심점을 찾아야한다.
		p = Point2F(p.x + mDrawRect.left * m_x_pixel_scale, p.y + mDrawRect.top * m_y_pixel_scale);
		Matrix3x2F r = Matrix3x2F::Rotation(90.0f,p);
	
		RenderTarget->SetTransform(transform_to_draw * t * r);
		p = Point2F(0, 0);
		RenderTarget->DrawTextLayout(p, wtl_y_axis_title, get_brush(y_axis.title.m_text_color));
	
		//// Draw Rect for test
		//D2D1_RECT_F rect;
		//rect.left = 0;
		//rect.top = 0;
		//rect.right = y_axis.title.rect.bottom - y_axis.title.rect.top;  // 폭과 높이가 바뀐다.
		//rect.bottom = y_axis.title.rect.right - y_axis.title.rect.left;
		//RenderTarget->DrawRectangle(rect, get_brush(RT_BLACK), 0.5);
	}
	if (show_legend)
	{
		int nl = line_stack.size(); // 전체 라인 수
		// 레전드 위치로 이동
		D2D1_POINT_2F t = Point2F(legend_rect.left, legend_rect.top);
		RenderTarget->SetTransform(transform_to_draw *Matrix3x2F::Translation(t.x*m_x_pixel_scale,t.y*m_y_pixel_scale));
	
		float space_for_line = 0.33f; // 라인을 그릴 공간의 비율
		
		// 다음 레전드로 이동할때, 사용할 이동 매트릭스 설정
		Matrix3x2F to_next = Matrix3x2F::Translation(legend_width*(1.f - space_for_line)*m_x_pixel_scale, 0);
		// 택스트 작성을 위해 이동할 때 사용하는 이동 매트릭스 설정
		Matrix3x2F to_text = Matrix3x2F::Translation(legend_width*space_for_line*m_y_pixel_scale, 0);
		D2D1_RECT_F label_rect = Rect(0.0f, 0.0f, legend_width*(1.f - space_for_line), legend_height);

		for (int l = 0; l < nl; l++)
		{
			LINE_SERIES *pLS = line_stack[l];
			//선 그리기
			RenderTarget->DrawLine(
				Point2F(0, legend_height / 2.f), Point2F(legend_width*space_for_line, legend_height / 2.f),
				get_brush(pLS->get_color()), pLS->get_width(), get_stroke_style(pLS->get_stroke_style()));

			Matrix3x2F t_c; // 현재 변환
			RenderTarget->GetTransform(&t_c);
			RenderTarget->SetTransform(t_c * to_text);

			RenderTarget->DrawText(pLS->name, pLS->name.GetLength(),
				wtf_legend_label, label_rect, get_brush(legend_text_color));
			// 다음 레이블 공간으로 이동
			RenderTarget->GetTransform(&t_c);
			RenderTarget->SetTransform(t_c * to_next);
		}
	}
}

void RT_GRAPH::draw_alarm_msg_box()
{
	float w = mDrawRect.right - mDrawRect.left;
	float h = mDrawRect.bottom - mDrawRect.top;

	D2D1_RECT_F alarm_rect;
	alarm_rect.left = w*0.15f;
	alarm_rect.top =  h*0.15f;
	alarm_rect.right = w - w*0.15f;
	alarm_rect.bottom = h - h*0.15f;
	RenderTarget->FillRectangle(alarm_rect, get_brush(RT_WHITE));
	RenderTarget->DrawRectangle(alarm_rect, get_brush(RT_RED),2);
	
	RenderTarget->DrawText(alarm_msg, alarm_msg.GetLength(), wtf_x_axis_tick_label, alarm_rect, get_brush(RT_DARK_BLUE));
}

/*--------------------------------------------------------------------------------------------------------
/	RT_GRAPH_ARRAY
/--------------------------------------------------------------------------------------------------------*/
RT_GRAPH_ARRAY::RT_GRAPH_ARRAY()
{
	RegisterWindowClass();
}
void RT_GRAPH_ARRAY::init_graph_array(int row_size, int col_size, int gap_btw_graph,  int devider_width)
{
	rows = row_size;
	cols = col_size;

	// gap and devider
	m_gap_btw_graph = (float)gap_btw_graph;
	m_divider_width = (float)devider_width;

	// 변수 초기화
	init_variables();

	// Direct2D 관련 초기화
	CRect base_rect;
	GetClientRect(&base_rect);  // get drawing area

	mBaseRect.left = (float)base_rect.left;
	mBaseRect.top = (float)base_rect.top;
	mBaseRect.right = (float)base_rect.right;
	mBaseRect.bottom = (float)base_rect.bottom;

	// Create Direct2D Factory - 그리는 도구를 생성하는 것이 Factory
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2D_Factory);

	// Create Direct2D Render Target - 그려질 공간

	// 방법 1 - 윈도우를 바로 렌더 타겟으로 설정 ----------------------------------------------------------------
	/* 
	D2D1_SIZE_U size = D2D1::SizeU(base_rect.Width(), base_rect.Height());
	D2D_Factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(this->GetSafeHwnd(), size, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
		&RenderTarget);
	// D2D1_PRESENT_OPTIONS_IMMEDIATELY은 display가 refresh될때까지 기다리지 않고 바로 렌더링하는 옵션
	// 기본은 D2D1_PRESENT_OPTIONS_NONE으로 display가 refresh가 될때까지 기다렸다가 렌더링한다.
	*/

	// 방법 2 - mem CDC를 만들고 렌더링하는 방법 --------------------------------------------------------------

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		D2D1_ALPHA_MODE_IGNORE),
		0,
		0,
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT
		);

	D2D_Factory->CreateDCRenderTarget(&props, &RenderTarget);
	
	// Bind the DC to the DC render target.
	CRect r;
	GetClientRect(r);

	memCDC = new CDC;
	memCDC->CreateCompatibleDC(GetDC());
		
	mBitmap.DeleteObject(); // 혹시 재사용한다면...
	mBitmap.CreateCompatibleBitmap(GetDC(), r.Width(), r.Height());
	memCDC->SelectObject(&mBitmap);
	RenderTarget->BindDC(memCDC->GetSafeHdc(), &r);

	// Calculate pixel scale
	D2D1_SIZE_F rt_size = RenderTarget->GetSize();
	m_x_pixel_scale = (float)rt_size.width / (mBaseRect.right - mBaseRect.left);
	m_y_pixel_scale = (float)rt_size.height / (mBaseRect.bottom - mBaseRect.top);

	// 브러쉬 최초생성
	RenderTarget->CreateSolidColorBrush(
		get_D2D1_color(m_border_color),
		&m_brush);

	// For zoom and pan
	m_dc_brush.DeleteObject();

	m_dc_brush.CreateSolidBrush(RT_LIGHT_GREY);

	init();
}
RT_GRAPH_ARRAY::~RT_GRAPH_ARRAY()
{
	for (auto& gc : serialized_graph_stack)
	{
		if (gc.pGraph)
		{
			delete gc.pGraph;
			gc.pGraph = NULL;
		}
	}
	serialized_graph_stack.clear();

	SafeRelease(&RenderTarget);
	SafeRelease(&D2D_Factory);

	if (m_pBitmap)
	{
		memCDC->SelectObject(m_pBitmap);
		mBitmap.DeleteObject();
	}
	if (memCDC)
		memCDC->DeleteDC();

	m_dc_brush.DeleteObject();
}

void RT_GRAPH_ARRAY::init_variables()
{
	// 원래 배경색 저장 - 버튼 색을 읽어서 가져온다.
	m_bg_color = GetSysColor(COLOR_3DFACE);

	// 폭과 높이 설정을 위한 Weight 초기화
	row_weight.clear();
	col_weight.clear();

	for (int i = 0; i < rows; i++)
		row_weight.push_back(1.0f);
	for (int i = 0; i < cols; i++)
		col_weight.push_back(1.0f);
}

void RT_GRAPH_ARRAY::init()
{
	// 일단 존재하던 그래프 스택을 초기화
	for (auto& gc : serialized_graph_stack)
	{
		SafeDelete(&gc.pGraph);
	}
	serialized_graph_stack.clear();

	serialized_graph_stack.resize(rows*cols);
	build_base_rect_of_cells();
	create_graph_cells();
}

void RT_GRAPH_ARRAY::build_base_rect_of_cells()
{
	float base_rect_width = mBaseRect.right - mBaseRect.left;
	float base_rect_height = mBaseRect.bottom - mBaseRect.top;

	float r_sum = 0.0f;
	float c_sum = 0.0f;
	for (auto& w : row_weight)
		r_sum += w;
	for (auto& w : col_weight)
		c_sum += w;

	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c<cols; c++)
		{
			if (c != 0)
			{
				serialized_graph_stack[r*cols + c].rect.left =	serialized_graph_stack[c - 1].rect.right ;
				serialized_graph_stack[r*cols + c].rect.right =
					serialized_graph_stack[c - 1].rect.right + col_weight[c] / c_sum * base_rect_width;
			}
			else
			{
				serialized_graph_stack[r*cols + c].rect.left = 0;
				serialized_graph_stack[r*cols + c].rect.right = col_weight[c] / c_sum * base_rect_width;
			}

			if (r != 0)
			{
				serialized_graph_stack[r*cols + c].rect.top = serialized_graph_stack[(r - 1)*cols].rect.bottom;
				serialized_graph_stack[r*cols + c].rect.bottom =
					serialized_graph_stack[(r - 1)*cols].rect.bottom + row_weight[r] / r_sum * base_rect_height;
			}
			else
			{
				serialized_graph_stack[r*cols + c].rect.top = 0;
				serialized_graph_stack[r*cols + c].rect.bottom = row_weight[r] / r_sum * base_rect_height;
			}
		}
	}
	// recalcuate for inserting gap
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c<cols; c++)
		{
			serialized_graph_stack[r*cols + c].rect.left	+= m_gap_btw_graph / 2.f;
			serialized_graph_stack[r*cols + c].rect.right	-= m_gap_btw_graph / 2.f;
			serialized_graph_stack[r*cols + c].rect.top		+= m_gap_btw_graph / 2.f; 
			serialized_graph_stack[r*cols + c].rect.bottom  -= m_gap_btw_graph / 2.f;
		}
	}
}
void RT_GRAPH_ARRAY::create_graph_cells()
{
	for (auto& gc : serialized_graph_stack)
	{
		gc.pGraph = new RT_GRAPH(D2D_Factory, RenderTarget, gc.rect, m_x_pixel_scale, m_y_pixel_scale);
		gc.pGraph->set_bg_color(m_bg_color);
	}
}

ID2D1SolidColorBrush* RT_GRAPH_ARRAY::get_brush(COLORREF color, float opacity)
{
	m_brush->SetColor(get_D2D1_color(color));
	saturate(opacity, 0.0f, 1.0f);
	m_brush->SetOpacity(opacity);

	return m_brush;
}


void RT_GRAPH_ARRAY::set_row_weight(int row, float w)
{
	if (is_value_in_range(row, 0, rows - 1))
		row_weight[row] = w;
}
void RT_GRAPH_ARRAY::set_col_weight(int col, float w)
{
	if (is_value_in_range(col, 0, cols - 1))
		col_weight[col] = w;
}

// note: this function can be used only when initialization is done.
void RT_GRAPH_ARRAY::set_font_of_all_cells(CString face_name)
{
	for (auto& gc : serialized_graph_stack)
	{
		if (!gc.is_merged)
			gc.pGraph->set_font_face(face_name);
	}
}

void RT_GRAPH_ARRAY::set_bg_color(COLORREF color)
{
	m_bg_color = color;
	for (auto& gc : serialized_graph_stack)
	{
		if (!gc.is_merged)
			gc.pGraph->set_bg_color(color);
	}
}

void RT_GRAPH_ARRAY::set_border(int width, COLORREF color)
{
	m_border_width = (float)width;
	if (width > 0.0f)
		m_show_border = true;
	m_border_color = color;
}

void RT_GRAPH_ARRAY::set_divider(int width, COLORREF color)
{
	m_divider_width = (float)width;
	if (width > 0.0f)
		m_show_divider = true;
	m_divider_color = color;
}

void RT_GRAPH_ARRAY::draw()
{
	RenderTarget->BeginDraw();

	draw_border_and_divider(); 

	for (auto& gc : serialized_graph_stack)
	{
		if (!gc.is_merged)
		if (gc.pGraph)
			gc.pGraph->draw();
	}
	RenderTarget->EndDraw();

	Invalidate();
}
bool RT_GRAPH_ARRAY::save(CString filename, const GUID &filetype)
{
	if (memCDC == NULL)
		return false;
	CImage image;
	CRect r;
	GetClientRect(r);	

	image.Create(r.Width(), r.Height(), memCDC->GetDeviceCaps(BITSPIXEL));
	CDC* imageDC = CDC::FromHandle(image.GetDC()); // 저장할 이미지의 dc;
	imageDC->BitBlt(0, 0, r.Width(), r.Height(), memCDC, 0, 0, SRCCOPY);
	image.ReleaseDC();

	// image 저장 
	return (image.Save(filename, filetype) == S_OK);
}
void RT_GRAPH_ARRAY::synchronize(bool sync_x, bool sync_y)
{
	is_x_synchronized = sync_x;
	is_y_synchronized = sync_y;
}
void RT_GRAPH_ARRAY::draw_border_and_divider()
{
	RenderTarget->Clear(get_D2D1_color(m_bg_color));
	if (m_show_border)
	{
		RenderTarget->SetTransform(Matrix3x2F::Scale(m_x_pixel_scale, m_y_pixel_scale));
		RenderTarget->DrawRectangle(resized_rect(mBaseRect, -m_border_width / 2.f, -m_border_width / 2.f), 
				get_brush(m_border_color),m_border_width);
		RenderTarget->SetTransform(Matrix3x2F::Identity());
	}
	if (m_show_divider)
	{
		RenderTarget->SetTransform(Matrix3x2F::Scale(m_x_pixel_scale, m_y_pixel_scale));
		RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		D2D1_POINT_2F start, end;
		for (int c = 0; c < cols - 1; c++)
		{
			start.x = 0.5f * (serialized_graph_stack[c].rect.right + serialized_graph_stack[c + 1].rect.left);
			start.y = m_border_width;
			end.x = start.x;
			end.y = mBaseRect.bottom - m_border_width;
			RenderTarget->DrawLine(start, end, get_brush(m_divider_color), m_divider_width);
		}
		for (int r = 0; r < rows - 1; r++)
		{
			start.x = m_border_width;
			start.y = 0.5f * (serialized_graph_stack[r*cols].rect.bottom + serialized_graph_stack[(r+1)*cols].rect.top) * m_y_pixel_scale;
			end.x = mBaseRect.right - m_border_width;
			end.y = start.y;
			RenderTarget->DrawLine(start, end, get_brush(m_divider_color), m_divider_width);
		}
		RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		RenderTarget->SetTransform(Matrix3x2F::Identity());
	}
}

RT_GRAPH* RT_GRAPH_ARRAY::get_graph(int row, int col)
{
	if (!is_value_in_range(row, 0, rows - 1))
		return serialized_graph_stack[0].pGraph;
	if (!is_value_in_range(col, 0, cols - 1))
		return serialized_graph_stack[0].pGraph;
	return serialized_graph_stack[row * cols + col].pGraph;
}

IMPLEMENT_DYNAMIC(RT_GRAPH_ARRAY, CWnd)

BEGIN_MESSAGE_MAP(RT_GRAPH_ARRAY, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_MOUSEWHEEL()
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

#define RT_GRAPH_ARRAY_CLASSNAME    _T("RT_GRAPH_ARRAY")
BOOL RT_GRAPH_ARRAY::RegisterWindowClass()
{
	WNDCLASS    WndClass;
	HINSTANCE   hInstance = AfxGetInstanceHandle();

	if (::GetClassInfo(hInstance, RT_GRAPH_ARRAY_CLASSNAME, &WndClass) == FALSE)
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
		WndClass.lpszClassName = RT_GRAPH_ARRAY_CLASSNAME;

		if (!AfxRegisterClass(&WndClass))
		{
			//	AfxThrowResourceException();
			return FALSE;
		}
	}
	return TRUE;
}
void RT_GRAPH_ARRAY::OnPaint()
{
	CPaintDC dc(this);

	CRect r;
	GetClientRect(r);
	dc.BitBlt(0, 0, r.Width(), r.Height(), memCDC, 0, 0, SRCCOPY);

	// Draw Zoom Rect
	if (LB_down)
	{
		CPoint p;
		CRect r_z;
		GetCursorPos(&p);
		ScreenToClient(&p);
		if (graph_selected < (int)serialized_graph_stack.size())
		{
			if (serialized_graph_stack[graph_selected].pGraph)
			{
				CRect r_g = serialized_graph_stack[graph_selected].pGraph->get_graph_rect();
				r_z.left = LB_down_pos.x;
				r_z.top = LB_down_pos.y;
				saturate(p.x, r_g.left, r_g.right);
				saturate(p.y, r_g.top, r_g.bottom);
				r_z.right = p.x;
				r_z.bottom = p.y;
				r_z.NormalizeRect();
				dc.Draw3dRect(r_z, RT_NICE_BLUE, RT_NICE_BLUE);
			}
		}
	}
	// Draw Pan Line
	if (RB_down)
	{
		CPoint p;
		GetCursorPos(&p);
		ScreenToClient(&p);
		if (graph_selected < (int)serialized_graph_stack.size())
		{
			if (serialized_graph_stack[graph_selected].pGraph)
			{
				CRect r_g = serialized_graph_stack[graph_selected].pGraph->get_graph_rect();
				saturate(p.x, r_g.left, r_g.right);
				saturate(p.y, r_g.top, r_g.bottom);

				LONG copy_w = r_g.Width() - abs(p.x - RB_down_pos.x) - 1;
				LONG copy_h = r_g.Height() - abs(p.y - RB_down_pos.y) - 1;

				CRect r_g_s = r_g;
				r_g_s.right -= 1;
				dc.FillRect(r_g_s, &m_dc_brush);
				dc.BitBlt(
					p.x >= RB_down_pos.x ? r_g.left + (p.x - RB_down_pos.x) : r_g.left,
					p.y >= RB_down_pos.y ? r_g.top + (p.y - RB_down_pos.y) : r_g.top,
					copy_w, copy_h, memCDC,
					p.x >= RB_down_pos.x ? r_g.left : r_g.left - (p.x - RB_down_pos.x),
					p.y >= RB_down_pos.y ? r_g.top : r_g.top - (p.y - RB_down_pos.y),
					SRCCOPY);
			}
		}
	}
}
BOOL RT_GRAPH_ARRAY::OnEraseBkgnd(CDC *pDC)
{
	//return CWnd::OnEraseBkgnd(pDC);
	// 위의 원래함수는 배경을 한번 지우기 때문에 깜박거리는 현상이 생긴다.
	// 아래처럼 리턴해 주면 깜박임이 사라진다.
	return false;
}
bool is_point_in_rect(CPoint& p, CRect& r)
{
	if (is_value_in_range(p.x, r.left, r.right) && is_value_in_range(p.y, r.top, r.bottom))
		return true;
	else
		return false;
}
BOOL RT_GRAPH_ARRAY::PreTranslateMessage(MSG* pMsg)
{
	
	// 마우스 왼쪽 버튼으로 줌 기능 구현
	if (pMsg->message == WM_LBUTTONDOWN)
	{
		GetCursorPos(&LB_down_pos);
		ScreenToClient(&LB_down_pos);

		int g_idx = 0;
		for (auto&g : serialized_graph_stack)
		{
			CRect r_g = serialized_graph_stack[g_idx].pGraph->get_graph_rect();
			if (is_point_in_rect(LB_down_pos, r_g))
			{
				LB_down = true;
				SetTimer(0, 50, NULL);
				graph_selected = g_idx;
				break;
			}
			g_idx++;
		}
	}
	if (pMsg->message == WM_LBUTTONUP)
	{
		KillTimer(0);
		if (LB_down)
		{
			LB_down = false;

			CPoint lb;
			GetCursorPos(&lb);
			ScreenToClient(&lb);

			if (graph_selected < (int)serialized_graph_stack.size())
			{
				if (serialized_graph_stack[graph_selected].pGraph)
				{
					CRect r_g = serialized_graph_stack[graph_selected].pGraph->get_graph_rect();
					saturate(lb.x, r_g.left, r_g.right);
					saturate(lb.y, r_g.top, r_g.bottom);

					if ((lb.x > LB_down_pos.x) && (lb.y > LB_down_pos.y))
					{
						float scale_x = (float)(lb.x - LB_down_pos.x) / (float)r_g.Width();
						float scale_y = (float)(lb.y - LB_down_pos.y) / (float)r_g.Height();

						float c_x = ((float)(lb.x + LB_down_pos.x) / 2.0f - (float)(r_g.left)) / (float)r_g.Width();
						float c_y = -((float)(lb.y + LB_down_pos.y) / 2.0f - (float)(r_g.bottom)) / (float)r_g.Height();
						if (is_x_synchronized || is_y_synchronized)
						{
							int idx = 0;
							for (auto& g : serialized_graph_stack)
							{
								if (idx == graph_selected)
									g.pGraph->zoom(true, c_x, c_y, scale_x, scale_y);
								else
								{
									g.pGraph->zoom(true, 
										is_x_synchronized ? c_x: 0.5f, 
										is_y_synchronized ? c_y: 0.5f,
										is_x_synchronized ? scale_x: 1.0f,
										is_y_synchronized ? scale_y: 1.0f);
								}
								idx++;
							}
						} 
						else // 독립적이면
							serialized_graph_stack[graph_selected].pGraph->zoom(true, c_x, c_y, scale_x, scale_y);
					}
					else
					{
						if (is_x_synchronized || is_y_synchronized)
						{
							for (auto& g : serialized_graph_stack)
							{
								g.pGraph->zoom(false);
							}
						}
						else
							serialized_graph_stack[graph_selected].pGraph->zoom(false);
					}
					draw();
				}
			}
		}
	}

	// 마우스 오른쪽 버튼으로 팬 기능 구현
	if (pMsg->message == WM_RBUTTONDOWN)
	{
		GetCursorPos(&RB_down_pos);
		ScreenToClient(&RB_down_pos);

		int g_idx = 0;
		for (auto&g : serialized_graph_stack)
		{
			CRect r_g = serialized_graph_stack[g_idx].pGraph->get_graph_rect();
			if (is_point_in_rect(RB_down_pos, r_g))
			{
				RB_down = true;
				SetTimer(0, 50, NULL);
				graph_selected = g_idx;
				break;
			}
			g_idx++;
		}
	}

	if (pMsg->message == WM_RBUTTONUP)
	{
		KillTimer(0);
		if (RB_down)
		{
			RB_down = false;

			CPoint rb;
			GetCursorPos(&rb);
			ScreenToClient(&rb);

			if (graph_selected < (int)serialized_graph_stack.size())
			{
				if (serialized_graph_stack[graph_selected].pGraph)
				{
					CRect r_g = serialized_graph_stack[graph_selected].pGraph->get_graph_rect();
					saturate(rb.x, r_g.left, r_g.right);
					saturate(rb.y, r_g.top, r_g.bottom);

					float c_x = -(float)(rb.x - RB_down_pos.x) / (float)r_g.Width() + 0.5f;
					float c_y =  (float)(rb.y - RB_down_pos.y) / (float)r_g.Height() + 0.5f;
					if (is_x_synchronized || is_y_synchronized)
					{
						int idx = 0;
						for (auto& g : serialized_graph_stack)
						{
							if (idx == graph_selected)
								g.pGraph->zoom(true, c_x, c_y, 1.0f, 1.0f);
							else
							{
								g.pGraph->zoom(true,
									is_x_synchronized ? c_x : 0.5f,
									is_y_synchronized ? c_y : 0.5f,
									1.0f,1.0f);
							}
							idx++;
						}
					} 
					else // 독립적이면
						serialized_graph_stack[graph_selected].pGraph->zoom(true, c_x, c_y, 1.0f, 1.0f);

					draw();
				}
			}
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

BOOL RT_GRAPH_ARRAY::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{

	// Step 1: 해당 그래프를 찾는다. 그래프 영역이 아니면 바로 리턴
	CPoint p_at_this = pt;
	bool p_in_this = false;
	float scale_w = 1.0;
	ScreenToClient(&p_at_this);

	int g_idx = 0;
	for (auto&g : serialized_graph_stack)
	{
		CRect r_g = serialized_graph_stack[g_idx].pGraph->get_graph_rect();
		if (is_point_in_rect(p_at_this, r_g))
		{
			p_in_this = true;
			SetTimer(0, 50, NULL);
			graph_selected = g_idx;
			break;
		}
		g_idx++;
	}

	if (p_in_this)
	{
		// Step 2: 스케일을 계산
		scale_w = 1.0f + (float)zDelta / (float)WHEEL_DELTA * 0.05f;

		// Step 3: 그래프 업데이트

		if (graph_selected < (int)serialized_graph_stack.size())
		{
			if (serialized_graph_stack[graph_selected].pGraph)
			{
				CRect r_g = serialized_graph_stack[graph_selected].pGraph->get_graph_rect();
				saturate(p_at_this.x, r_g.left, r_g.right);
				saturate(p_at_this.y, r_g.top, r_g.bottom);

				if (is_x_synchronized || is_y_synchronized)
				{
					int idx = 0;
					for (auto& g : serialized_graph_stack)
					{
						if (idx == graph_selected)
							g.pGraph->zoom(true, 0.5f, 0.5f, scale_w, scale_w);
						else
						{
							g.pGraph->zoom(true, 0.5f, 0.5f,
								is_x_synchronized ? scale_w : 1.0f,
								is_y_synchronized ? scale_w : 1.0f);
						}
						idx++;
					}
				}
				else // 독립적이면
					serialized_graph_stack[graph_selected].pGraph->zoom(true, 0.5f, 0.5f, scale_w, scale_w);

				draw();
			}
		}
		// Step 4: 화면 반영 (OnPaint 기동)
		Invalidate();
	}
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void RT_GRAPH_ARRAY::OnTimer(UINT_PTR nIDEvent)
{
	Invalidate();
	CWnd::OnTimer(nIDEvent);
}

void RT_GRAPH_ARRAY::OnDestroy()
{
	CWnd::OnDestroy();
	KillTimer(0);
}