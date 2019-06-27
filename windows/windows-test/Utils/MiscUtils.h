#pragma once

typedef unsigned __int64 QWORD;

inline HRESULT getLastHr() { return HRESULT_FROM_WIN32( GetLastError() ); }

inline HRESULT getLastGl()
{
	const GLenum e = glGetError();
	if( e == GL_NO_ERROR )
		return S_OK;
	// OpenGL error codes are small positive numbers. By negating them, we create an HRESULT that will be FAILED.
	// See ErrorMessage function that formats those errors.
	return -(int)( e );
}

// FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM) wrapper, also supports OpenGL errors.
__declspec( noinline ) CStringW ErrorMessage( HRESULT hr );

// CString::Format, then append FormatMessage(hr), then call MessageBox.
void ReportError( CWindow* pWnd, UINT uMbIconType, HRESULT hr, LPCTSTR pstrFormat, ... );

// Clamp a value.
// Why there's 'std::min', 'max' but no 'clamp' ? Let's fix it.
namespace std
{
	template< class T >
	inline T clamp( T val, T valMin, T valMax )
	{
		if( val < valMin )
			return valMin;
		if( val > valMax )
			return valMax;
		return val;
	}
}

// Wrapper around QueryPerformanceFrequency
inline QWORD GetPerfFrequency()
{
	QWORD res;
	QueryPerformanceFrequency( reinterpret_cast<LARGE_INTEGER*>( &res ) );
	return res;
}

// Wrapper around QueryPerformanceCounter
inline QWORD GetPerfCounter()
{
	QWORD res;
	QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER*>( &res ) );
	return res;
}

HRESULT loadResource( LPCWSTR lpName, LPCWSTR lpType, std::pair<DWORD, const BYTE*> &result );