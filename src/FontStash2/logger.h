#pragma once

#ifdef NDEBUG
inline void logDebug( const char* pszFormat, ... ) { }
inline void logInfo( const char* pszFormat, ... ) { }
inline void logWarning( const char* pszFormat, ... ) { }
inline void logError( const char* pszFormat, ... ) { }
#else	// NDEBUG

#ifdef _MSC_VER
// Windows build, colorize the console with WinAPI
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

enum struct eLogColor : uint8_t
{
	Debug = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	Info = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	Warning = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
	Error = FOREGROUND_RED | FOREGROUND_INTENSITY,
};

class ConsoleColor
{
	uint16_t m_prev;

public:

	ConsoleColor( eLogColor color )
	{
		const HANDLE h = GetStdHandle( STD_OUTPUT_HANDLE );
		CONSOLE_SCREEN_BUFFER_INFO sbi;
		GetConsoleScreenBufferInfo( h, &sbi );
		m_prev = sbi.wAttributes;
		SetConsoleTextAttribute( h, (uint8_t)color );
	}
	~ConsoleColor()
	{
		SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), m_prev );
	}
};

#define logDebug( s, ... ) { ConsoleColor _cc( eLogColor::Debug ); printf( s "\n", __VA_ARGS__ ); }
#define logInfo( s, ... )  { ConsoleColor _cc( eLogColor::Info ); printf( s "\n", __VA_ARGS__ ); }
#define logWarning( s, ... )  { ConsoleColor _cc( eLogColor::Warning ); printf( s "\n", __VA_ARGS__ ); }
#define logError( s, ... )  { ConsoleColor _cc( eLogColor::Error ); printf( s "\n", __VA_ARGS__ ); }

#else	// _MSC_VER

#define logDebug( s, ... ) printf( s "\n", __VA_ARGS__ )
#define logInfo( s, ... ) printf( s "\n", __VA_ARGS__ )
#define logWarning( s, ... ) printf( s "\n", __VA_ARGS__ )
#define logError( s, ... ) printf( s "\n", __VA_ARGS__ )

#endif	// _MSC_VER

#endif	// NDEBUG