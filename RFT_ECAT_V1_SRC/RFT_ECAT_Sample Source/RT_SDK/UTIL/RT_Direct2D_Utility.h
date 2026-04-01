#ifndef _RT_DIRECT2D_UTILITY_H_
#define _RT_DIRECT2D_UTILITY_H_

#include <d2d1.h>
//#include <d2d1_1helper.h>
////#pragma comment(lib, "d2d1.lib")

// COLORREF로 정의된 색을 Direct2D의 색으로 변경
D2D1::ColorF get_D2D1_color(COLORREF color);
D2D1_RECT_F resized_rect(const D2D1_RECT_F& rect, float x, float y);

#endif
