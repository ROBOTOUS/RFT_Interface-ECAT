#include "RT_Delay.h"

#include <chrono>
#include <thread>

void delay_ms(DWORD dwMillisecond)
{
	/* NOTE: if dwMillisecond > 100ms, real delay time become slower */
	MSG msg;

	std::chrono::milliseconds delay(dwMillisecond);
	auto end = std::chrono::high_resolution_clock::now() + delay;
	while (std::chrono::high_resolution_clock::now() < end)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void wait_ms(DWORD dwMillisecond)
{
	MSG msg;
	DWORD dwStart;
	dwStart = GetTickCount();

	while (GetTickCount() - dwStart < dwMillisecond)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
