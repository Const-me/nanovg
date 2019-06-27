#pragma once

// An abstract class that implements a top-level window, with OpenGL-rendered content inside
class CGlWnd:
	public CWindowImpl<CGlWnd, CWindow, CFrameWinTraits>
{
public:
	CGlWnd();
	~CGlWnd() { }

	DECLARE_WND_CLASS_EX( L"CGlWnd", CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC, -1 );

	BEGIN_MSG_MAP( CGlWnd )
		MESSAGE_HANDLER( WM_CREATE, OnCreate )
		MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
		MESSAGE_HANDLER( WM_PAINT, OnPaint )
		MESSAGE_HANDLER( WM_SIZE, OnSize )
		MESSAGE_HANDLER( WM_GETMINMAXINFO, OnGetMinMaxInfo )
		MESSAGE_HANDLER( WM_SYSKEYDOWN, OnSysKeyDown );
	MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBackground );
	END_MSG_MAP()

	inline void render()
	{
		if( m_bSuppressDeviceOperations )
			Sleep( 100 );
		else
			renderImpl();
	}

private:
	static const CPoint s_MinTrackSize;

	HRESULT initOpenGL();
	void renderImpl();
	HRESULT toggleFullscreen();

	// Basic windows messages
	LRESULT OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnPaint( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnGetMinMaxInfo( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnSysKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnEraseBackground( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

	bool m_bSuppressDeviceOperations;
	HDC m_dc;
	HGLRC m_context;

	// Stuff for toggle fullscreen functionality
	bool m_bFullscreen;	// true when fullscreen
	WINDOWPLACEMENT m_savedPlacement; // When fullscreen, contains saved window placement. When not fullscreen, meaningless.
	LONG_PTR m_savedStyle; // When fullscreen, contains saved window style. When not fullscreen, meaningless.
	LONG_PTR m_savedExtStyle; // When fullscreen, contains saved window extended style. When not fullscreen, meaningless.

	// High-resolution timestamps to calculate elapsed frame times.
	QWORD m_ttFreq, m_ttLast;

	HRESULT reportInitializationError( bool lastWin32, LPCTSTR pszFormat, ... );

	// Override this to create your D3D resources.
	// The function will be called only once: from WM_CREATE handler.
	virtual HRESULT createResources( CString& failReason ) { return S_OK; }

	// Destroy all your D3D resources.
	// The function will be called only once: from WM_DESTROY event handler.
	virtual void destroyResources() { }

	// Override this to prepare the scene. Will be called after resizing the window.
	virtual void initScene( CSize viewportSize ) { }

	// Override this to do the actual rendering.
	// SecsElapsed will be set to the number of seconds since last call, the common values are between 0.01 and 0.02
	virtual void drawScene( float SecsElapsed ) = 0;
};