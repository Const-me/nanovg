#include "stdafx.h"
#include "AppWindow.h"

#define NANOVG_GL2_IMPLEMENTATION	// Use GL2 implementation.
#include <nanovg_gl.h>
#pragma comment( lib, "freetype.lib" )
#include <fontstash.h>

constexpr bool saveAtlasOnResize = false;

HRESULT AppWindow::create( int nCmdShow )
{
	CRect initialPosition
	{
		0, 0, 720, 640
	};
	HWND res = __super::Create( nullptr, &initialPosition, L"NanoVG demo" );
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

	calibriFont = nvgCreateFont( vg, "Calibri", R"(C:\Windows\Fonts\calibri.ttf)" );
	if( FONS_INVALID == calibriFont )
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

void AppWindow::drawAlphaAnimation( float SecsElapsed )
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

	m_time += SecsElapsed * 0.25f;
	if( m_time > 1 )
		m_time = 0;
	nvgGlobalAlpha( vg, (float)m_time );

	nvgFontFaceId( vg, consolasFont );
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
}

inline NVGcolor color( uint32_t bgr )
{
	return nvgRGB( (uint8_t)( bgr >> 16 ), (uint8_t)( bgr >> 8 ), (uint8_t)( bgr ) );
}

void AppWindow::drawPangrams()
{
	struct sColors
	{
		NVGcolor background, foreground;
		sColors( uint32_t bg, uint32_t text ) :
			background( color( bg ) ), foreground( color( text ) )
		{ }
	};

	static const std::array<sColors, 4> s_colors =
	{
		sColors{ 0x0E0B16, 0xFFFFFF },
		sColors{ 0xA239CA, 0xFFFFFF },
		sColors{ 0x4717F6, 0xFFFFFF },
		sColors{ 0xE7DFDD, 0x333333 },
	};

	static const char* pangrams =
		u8"The quick brown fox jumps over the lazy dog.\n"
		u8"Gojazni đačić s biciklom drži hmelj i finu vatu u džepu nošnje.\n"
		u8"Чуєш їх, доцю, га? Кумедна ж ти, прощайся без ґольфів!";
	static const char* pangramsEnd = pangrams + strlen( pangrams );

	const float w = windowSize.x;
	const float h = windowSize.y;

	nvgBeginFrame( vg, w, h, 1.0f );

	nvgFontFaceId( vg, calibriFont );
	nvgTextAlign( vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP );

	constexpr float pxHeight = 16;	// Height of the capital letters, without ascenders
	constexpr float nvgHeight = pxHeight * 1.6f;
	nvgFontSize( vg, nvgHeight );

	constexpr float heightMul = 1.0f / s_colors.size();
	const float heightPerSegment = (float)windowSize.y * heightMul;
	for( size_t i = 0; i < s_colors.size(); i++ )
	{
		const float y1 = roundf( i * heightPerSegment );
		const float y2 = roundf( ( i + 1 ) * heightPerSegment );

		// Background
		nvgFillColor( vg, s_colors[ i ].background );
		nvgBeginPath( vg );
		nvgRect( vg, 0, y1, w, y2 - y1 );
		nvgFill( vg );

		// Text
		const float textWidth = w - 20;
		std::array<float, 4> bounds;
		nvgTextBoxBounds( vg, 0, 0, textWidth, pangrams, pangramsEnd, bounds.data() );
		const float measuredWidth = bounds[ 2 ] - bounds[ 0 ];
		const float measuredHeight = bounds[ 3 ] - bounds[ 1 ];
		const float x1 = floorf( ( w - measuredWidth ) * 0.5f );
		const float ty1 = floorf( y1 + ( y2 - y1 - measuredHeight ) * 0.5f );

		nvgTextColor( vg, s_colors[ i ].foreground, s_colors[ i ].background );
		nvgTextBox( vg, x1, ty1, textWidth, pangrams, pangramsEnd );
	}

	nvgEndFrame( vg );
}

void AppWindow::drawScene( float SecsElapsed )
{
	// drawAlphaAnimation( SecsElapsed );
	drawPangrams();

	if constexpr( saveAtlasOnResize )
	{
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
}