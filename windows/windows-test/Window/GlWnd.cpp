#include "stdafx.h"
#include "GlWnd.h"
#include <Utils/MiscUtils.h>

#pragma comment(lib, "opengl32.lib")

CGlWnd::CGlWnd()
{
	m_bSuppressDeviceOperations = false;
	m_bFullscreen = false;
}

HRESULT CGlWnd::initOpenGL()
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dd318286(v=vs.85).aspx
	PIXELFORMATDESCRIPTOR pfd
	{
		sizeof( PIXELFORMATDESCRIPTOR ),  // size of this pfd  
		1,                                // version number  
		PFD_DRAW_TO_WINDOW |              // support window  
		PFD_SUPPORT_OPENGL |              // support OpenGL  
		PFD_DOUBLEBUFFER,                 // double buffered  
		PFD_TYPE_RGBA,                    // RGBA type  
		24,                               // 24-bit color depth  
		0, 0, 0, 0, 0, 0,                 // color bits ignored  
		0,                                // no alpha buffer  
		0,                                // shift bit ignored  
		0,                                // no accumulation buffer  
		0, 0, 0, 0,                       // accum bits ignored  
		32,                               // 32-bit z-buffer      
		0,                                // no stencil buffer  
		0,                                // no auxiliary buffer  
		PFD_MAIN_PLANE,                   // main layer  
		0,                                // reserved  
		0, 0, 0                           // layer masks ignored  
	};

	m_dc = GetDC();
	if( NULL == m_dc )
		return reportInitializationError( true, L"GetDC failed" );

	// get the device context's best, available pixel format match  
	int iPixelFormat = ChoosePixelFormat( m_dc, &pfd );
	if( 0 == iPixelFormat )
		return reportInitializationError( true, L"ChoosePixelFormat failed" );

	// make that match the device context's current pixel format  
	if( !SetPixelFormat( m_dc, iPixelFormat, &pfd ) )
		return reportInitializationError( true, L"SetPixelFormat failed" );

	m_context = wglCreateContext( m_dc );
	if( nullptr == m_context )
		return reportInitializationError( true, L"wglCreateContext failed" );

	if( !wglMakeCurrent( m_dc, m_context ) )
		return reportInitializationError( true, L"wglMakeCurrent failed" );

	GLenum err = glewInit();
	if( GLEW_OK != err )
	{
		// Problem: glewInit failed, something is seriously wrong.
		return reportInitializationError( false, L"glewInit failed: %S", glewGetErrorString( err ) );
	}

	if( !GLEW_VERSION_3_0 )
		return reportInitializationError( false, L"This demo requires OpenGL 3.0 or above" );

	CString failReason;
	HRESULT hr = createResources( failReason );
	if( FAILED( hr ) )
	{
		CString message = L"Can't load the required resources.";
		if( failReason.GetLength() > 0 )
		{
			message += "\n";
			message += failReason;
		}
		ReportError( this, MB_ICONERROR, hr, message );
		return hr;
	}

	CRect rc;
	GetClientRect( rc );
	initScene( rc.Size() );

	return S_OK;
}

HRESULT CGlWnd::reportInitializationError( bool lastWin32, LPCTSTR pszFormat, ... )
{
	va_list argList;
	va_start( argList, pszFormat );
	CString msg;
	msg.FormatV( pszFormat, argList );
	va_end( argList );

	HRESULT hr = E_FAIL;
	if( lastWin32 )
	{
		hr = getLastHr();
		msg.Append( L"\r\n" );
		msg.Append( ErrorMessage( hr ) );
	}

	MessageBox( msg, L"Initialization failed", MB_OK | MB_ICONERROR );
	return hr;
}

HRESULT CGlWnd::toggleFullscreen()
{
	// http://stackoverflow.com/a/7195779/126995
	static const LONG_PTR fullscreenStyle = WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;

	if( m_bFullscreen )
	{
		// Restore style and placement
		if( 0 == SetWindowLongPtr( GWL_EXSTYLE, m_savedExtStyle ) )
			return getLastHr();
		if( 0 == SetWindowLongPtr( GWL_STYLE, m_savedStyle ) )
			return getLastHr();
		if( !SetWindowPlacement( &m_savedPlacement ) )
			return getLastHr();

		InvalidateRect( nullptr );

		m_bFullscreen = false;
	}
	else
	{
		// Save style and placement
		ZeroMemory( &m_savedPlacement, sizeof( m_savedPlacement ) );
		m_savedPlacement.length = sizeof( WINDOWPLACEMENT );
		if( !GetWindowPlacement( &m_savedPlacement ) )
			return getLastHr();

		m_savedStyle = GetWindowLongPtr( GWL_STYLE );
		if( 0 == m_savedStyle )
			return getLastHr();

		m_savedExtStyle = GetWindowLongPtr( GWL_EXSTYLE );
		if( 0 == m_savedExtStyle )
			return getLastHr();

		// Find monitor rectangle
		HMONITOR hm = MonitorFromWindow( m_hWnd, MONITOR_DEFAULTTOPRIMARY );
		MONITORINFO mi = { 0 };
		mi.cbSize = sizeof( MONITORINFO );
		if( !GetMonitorInfo( hm, &mi ) )
			return getLastHr();

		// Set style and placement
		if( 0 == SetWindowLongPtr( GWL_STYLE, fullscreenStyle ) )
			return getLastHr();
		if( 0 == SetWindowLongPtr( GWL_EXSTYLE, m_savedExtStyle | WS_EX_TOPMOST ) )
			return getLastHr();
		// Set position
		SetWindowPos( HWND_TOPMOST, &mi.rcMonitor, SWP_FRAMECHANGED | SWP_SHOWWINDOW );

		m_bFullscreen = true;
	}
	return true;
}

void CGlWnd::renderImpl()
{
	QWORD ttNow;
	if( 0 == m_ttFreq )
	{
		m_ttFreq = GetPerfFrequency();
		m_ttLast = GetPerfCounter();
		ttNow = m_ttLast;
	}
	else
		ttNow = GetPerfCounter();

	const float fltSecsElapsed = static_cast<float>( ttNow - m_ttLast ) / static_cast<float>( m_ttFreq );

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	drawScene( fltSecsElapsed );

	// wglSwapIntervalEXT( 1 );
	SwapBuffers( m_dc );

	m_ttLast = ttNow;
}