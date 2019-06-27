#include "stdafx.h"
#include "AppWindow.h"

#define NANOVG_GL2_IMPLEMENTATION	// Use GL2 implementation.
#include <nanovg_gl.h>
#pragma comment( lib, "freetype.lib" )

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
	vg = nvgCreateGL2( NVG_ANTIALIAS | NVG_STENCIL_STROKES );
	if( nullptr == vg )
	{
		failReason = L"nvgCreateGL2 failed";
		return E_FAIL;
	}
	return S_OK;
}

void AppWindow::initScene( CSize viewportSize )
{
	windowSize.x = (float)viewportSize.cx;
	windowSize.y = (float)viewportSize.cy;
	pxRatio = windowSize.x / windowSize.y;
	glViewport( 0, 0, viewportSize.cx, viewportSize.cy );
}

void AppWindow::drawScene( float SecsElapsed )
{
	nvgBeginFrame( vg, windowSize.x, windowSize.y, pxRatio );
	nvgFillColor( vg, nvgRGB( 0xCC, 0xCC, 0xCC ) );
	nvgRect( vg, 0, 0, windowSize.x, windowSize.y );
	nvgFill( vg );
	nvgEndFrame( vg );
}