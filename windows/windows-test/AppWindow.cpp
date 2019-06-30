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
	const NVGcolor lightGray = nvgRGB( 0xCC, 0xCC, 0xCC );
	const NVGcolor black = nvgRGB( 0, 0, 0 );
	const NVGcolor white = nvgRGB( 0xFF, 0xFF, 0xFF );
	const float w = windowSize.x;
	const float h = windowSize.y;

	nvgBeginFrame( vg, w, h, 1.0f );
	nvgFillColor( vg, lightGray );
	nvgBeginPath( vg );
	nvgRect( vg, 0, 0, w, h );
	nvgFill( vg );

	nvgFontFace( vg, "Consolas" );
	nvgTextAlign( vg, NVG_ALIGN_TOP | NVG_ALIGN_CENTER );
	nvgFontSize( vg, 48 );

	nvgTextColor( vg, black, lightGray );
	nvgText( vg, w * 0.5f, h * 0.5f, "Black on light", nullptr );

	nvgFillColor( vg, black );
	nvgBeginPath( vg );
	nvgRect( vg, 0, 0, w, h * 0.33f );
	nvgFill( vg );

	nvgTextColor( vg, white, black );
	nvgText( vg, w * 0.5f, h * 0.25f, "White on black", nullptr );

	nvgFillColor( vg, black );
	nvgBeginPath( vg );
	nvgRect( vg, 0, h * 0.6667f, w, h * 0.3333f );
	nvgFill( vg );

	nvgTextColor( vg, white, black );
	nvgRotate( vg, nvgDegToRad( 180.0 ) );
	nvgTranslate( vg, -w, -h );
	nvgText( vg, w * 0.5f, h * 0.25f, "White on black", nullptr );

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