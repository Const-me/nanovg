#pragma once
#include <Window/GlWnd.h>
#include <nanovg.h>

class AppWindow : public CGlWnd
{
public:
	AppWindow() = default;
	~AppWindow() = default;

	HRESULT create( int nCmdShow );

private:

	void OnFinalMessage( HWND wnd ) override
	{
		// Gracefully exit the application when this window is destroyed.
		PostQuitMessage( 0 );
	}

	HRESULT createResources( CString& failReason ) override;

	void initScene( CSize viewportSize ) override;

	void drawScene( float SecsElapsed ) override;

	NVGcontext* vg = nullptr;
	float pxRatio;
	Vector2 windowSize;
};