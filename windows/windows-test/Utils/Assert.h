#pragma once

// Check HRESULT, if failed return the failed HRESULT
#define CHECK( hr )                                          \
{                                                            \
	const HRESULT _hr = ( hr );                              \
	if( FAILED( _hr ) )                                      \
	{                                                        \
		AtlTrace( "%s (%s, line %i): %S",                    \
				__func__, __FILE__, (int)__LINE__, (const wchar_t*)( ErrorMessage( _hr ) ) );    \
		return _hr;                                          \
	}                                                        \
}