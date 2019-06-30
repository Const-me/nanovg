#include "stdafx.h"
#include "AppWindow.h"

#define NANOVG_GL2_IMPLEMENTATION	// Use GL2 implementation.
#include <nanovg_gl.h>
#pragma comment( lib, "freetype.lib" )
#include <fontstash.h>

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

	consolasFont = nvgCreateFont( vg, "Consolas", R"(C:\Windows\Fonts\consola.ttf)" );
	if( FONS_INVALID == consolasFont )
	{
		failReason = L"Unable to load the font";
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
	bResized = true;
}

void AppWindow::drawScene( float SecsElapsed )
{
	nvgBeginFrame( vg, windowSize.x, windowSize.y, pxRatio );
	nvgFillColor( vg, nvgRGB( 0xCC, 0xCC, 0xCC ) );
	nvgRect( vg, 0, 0, windowSize.x, windowSize.y );
	nvgFill( vg );

	nvgFontFace( vg, "Consolas" );
	nvgTextAlign( vg, NVG_ALIGN_TOP | NVG_ALIGN_CENTER );
	nvgFontSize( vg, 48 );
	nvgFillColor( vg, nvgRGB( 0, 0, 0 ) );
	nvgText( vg, windowSize.x * 0.5f, windowSize.y * 0.5f, "Hello, World", nullptr );

	nvgEndFrame( vg );

	if( bResized )
	{
		bResized = false;
		static int sn = 0;
		sn++;
		CStringA path;
		path.Format( R"(C:\Temp\2remove\VG\atlas-%03i.tga)", sn );
		nvgDebugDumpFontAtlas( vg, path );
	}
}