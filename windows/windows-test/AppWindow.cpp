#include "stdafx.h"
#include "AppWindow.h"

HRESULT AppWindow::create( int nCmdShow )
{
	HWND res = __super::Create( NULL, NULL, L"NanoVG demo" );
	if( NULL == res )
		return getLastHr();

	ShowWindow( nCmdShow );
	return S_OK;
}

HRESULT AppWindow::createResources( CString& failReason )
{
	return S_FALSE;
}

void AppWindow::drawScene( float SecsElapsed )
{
}