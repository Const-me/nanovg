#pragma once

#ifdef NDEBUG
inline void logDebug( const char* pszFormat, ... ) { }
inline void logInfo( const char* pszFormat, ... ) { }
inline void logWarning( const char* pszFormat, ... ) { }
inline void logError( const char* pszFormat, ... ) { }
#else	// NDEBUG

#ifdef _MSC_VER
#include <stdint.h>
#define COLOR_LOG_WINAPI
#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.

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
	ConsoleColor( eLogColor color );
	~ConsoleColor();
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