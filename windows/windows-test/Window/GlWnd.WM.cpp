#include "stdafx.h"
#include "GlWnd.h"

LRESULT CGlWnd::OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	bHandled = FALSE;

	HRESULT hr = initOpenGL();
	if( FAILED( hr ) )
	{
		bHandled = TRUE;
		return -1;
	}
	return 0;
}

LRESULT CGlWnd::OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	bHandled = FALSE;
	destroyResources();
	return 0;
}

LRESULT CGlWnd::OnPaint( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	if( false == m_bSuppressDeviceOperations )
	{
		renderImpl();
		ValidateRect( nullptr );
	}
	return 0;
}

const CPoint CGlWnd::s_MinTrackSize = CPoint( 320, 200 );

LRESULT CGlWnd::OnGetMinMaxInfo( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	MINMAXINFO* pMMI = reinterpret_cast<MINMAXINFO*>( lParam );
	pMMI->ptMinTrackSize = s_MinTrackSize;
	return 0;
}

LRESULT CGlWnd::OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	bHandled = FALSE;

	switch( wParam )
	{
	case SIZE_MAXIMIZED:
	case SIZE_RESTORED:
		m_bSuppressDeviceOperations = false;
		break;
	case SIZE_MINIMIZED:
		m_bSuppressDeviceOperations = true;
	default:
		return 0;
	}

	if( false == m_bSuppressDeviceOperations )
	{
		CSize sz( (DWORD)( lParam ) );

		// Resize OpenGL viewport
		glViewport( 0, 0, sz.cx, sz.cy );

		// Resize whatever weird stuff user needs resizing.
		initScene( sz );
	}

	return 0;
}

// Handle Alt+Enter to toggle full screen
LRESULT CGlWnd::OnSysKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	bHandled = FALSE;
	if( VK_RETURN == wParam && ( DWORD( lParam ) & 0x20000000 ) )
	{
		// Handle Alt+Enter to toggle fullscreen mode
		HRESULT hr = toggleFullscreen();
		if( FAILED( hr ) )
		{
			// TODO: log somewhere
			return 0;
		}
		bHandled = TRUE;
		return 0;
	}
	return 0;
}

LRESULT CGlWnd::OnEraseBackground( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	bHandled = TRUE;
	return TRUE;
}