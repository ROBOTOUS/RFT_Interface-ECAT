#include "RT_Direct2D_Utility.h"

D2D1::ColorF get_D2D1_color(COLORREF color)
{
	UINT32 r = GetRValue(color);
	UINT32 g = GetGValue(color);
	UINT32 b = GetBValue(color);

	return D2D1::ColorF( ((r << 16) & 0xFF0000) | ((g << 8) & 0xFF00) | (b & 0xFF) );
}
D2D1_RECT_F resized_rect(const D2D1_RECT_F& rect, float x, float y)
{
	D2D1_RECT_F r;

	r.left = rect.left + x;
	r.right = rect.right - x;
	r.top = rect.top - y;
	r.bottom = rect.bottom + y;

	return r;
}

// End of File
