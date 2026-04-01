

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH에 대해 이 파일을 포함하기 전에 'stdafx.h'를 포함합니다."
#endif

#include "resource.h"		// 주 기호입니다.


// CRFT_IF_ECATApp:
// 이 클래스의 구현에 대해서는 RFT_IF_ECAT.cpp을 참조하십시오.
//

class CRFTS_InterfaceApp : public CWinApp
{
public:
	CRFTS_InterfaceApp();

// 재정의입니다.
public:
	virtual BOOL InitInstance();

// 구현입니다.

	DECLARE_MESSAGE_MAP()
};

extern CRFTS_InterfaceApp theApp;