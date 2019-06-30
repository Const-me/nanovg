#include "logger.h"

#ifdef COLOR_LOG_WINAPI
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

static HANDLE getConsoleHandle()
{
	static const HANDLE result = GetStdHandle( STD_OUTPUT_HANDLE );
	return result;
}

ConsoleColor::ConsoleColor( eLogColor color )
{
	const HANDLE h = getConsoleHandle();
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	GetConsoleScreenBufferInfo( h, &sbi );
	m_prev = sbi.wAttributes;
	SetConsoleTextAttribute( h, (uint8_t)color );
}

ConsoleColor::~ConsoleColor()
{
	SetConsoleTextAttribute( getConsoleHandle(), m_prev );
}
#endif