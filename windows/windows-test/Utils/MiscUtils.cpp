#include "stdafx.h"
#pragma comment( lib, "glu32.lib" )

__declspec( noinline ) CString ErrorMessage( HRESULT hr )
{
	CString res;

	if( FAILED( hr ) )
	{
		// Try printing an OpenGL error code
		const GLubyte *glCode = gluErrorString( -hr );
		if( nullptr != glCode )
		{
			res.Format( L"%S", glCode );
			return res;
		}
	}

	// System error
	LPVOID lpMsgBuf;
	if( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR)&lpMsgBuf, 0, NULL ) )
	{
		res.Format( L"%s", lpMsgBuf );
		LocalFree( lpMsgBuf );
		return res;
	}

	// Direct3D 9 error, if the header is included
#ifdef DIRECT3D_VERSION
	LPCTSTR strDX = DXGetErrorString( hr );
	if( strDX )
	{
		CString Res;
		Res.Format( L"%s (%s)", DXGetErrorDescription( hr ), strDX );
		return strDX;
	}
#endif

	res.Format( L"Unknown error 0x%.8X", hr );
	return res;
}

void ReportError( CWindow* pWnd, UINT uMbIconType, HRESULT hr, LPCTSTR pstrFormat, ... )
{
	CString str;

	va_list args;
	va_start( args, pstrFormat );

	if( nullptr == pWnd )
	{
		vfwprintf_s( stderr, pstrFormat, args );
		va_end( args );
		return;
	}

	str.FormatV( pstrFormat, args );
	va_end( args );

	if( FAILED( hr ) )
	{
		str += L"\r\n";
		str += ErrorMessage( hr );
	}

	uMbIconType &= MB_ICONMASK;
	::MessageBox( pWnd->m_hWnd, str, L"", MB_OK | uMbIconType );
}

HRESULT loadResource( LPCWSTR lpName, LPCWSTR lpType, std::pair<DWORD, const BYTE*> &result )
{
	HMODULE hm = ::GetModuleHandle( nullptr );
	if( nullptr == hm )
		CHECK( getLastHr() );

	const HRSRC res = FindResource( hm, lpName, lpType );
	if( NULL == res )
		CHECK( getLastHr() );

	const DWORD cb = SizeofResource( hm, res );
	if( 0 == cb )
		CHECK( E_INVALIDARG );

	const HGLOBAL gl = LoadResource( hm, res );
	if( NULL == gl )
		CHECK( E_INVALIDARG );

	const BYTE *source = (const BYTE*)LockResource( gl );

	result = std::make_pair( cb, source );
	return S_OK;
}