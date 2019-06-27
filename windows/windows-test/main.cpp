#include "stdafx.h"
#include "AppWindow.h"

int main()
{
	AppWindow w;
	HRESULT hr = w.create( SW_SHOWNORMAL );
	if( FAILED( hr ) )
	{
		// Most likely the user already saw a message box displayed from within the CMainWnd::OnCreate before it returned (-1).
		ReportError( NULL, MB_ICONERROR, hr, L"CMainWnd::Create() failed." );
		return 0;
	}

	// Game-style message loop that constantly renders those window.
	// Very inefficient for NanoVG content, I'm just copy-pasting from my older project: https://github.com/Const-me/GL3Windows
	MSG msg;
	do
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != 0 )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			// No messages to process, run idle rendering.
			// By default, OpenGL's SwapBuffers (final part of the render implementation) is good enough at keeping rendering in sync with monitor's refresh
			w.render();
			msg.message = WM_NULL;
		}
	} while( WM_QUIT != msg.message );

	return (int)msg.wParam;

	return 0;
}