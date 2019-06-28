#pragma once

#ifdef NDEBUG
inline void logDebug( const char* pszFormat, ... ) { }
inline void logInfo( const char* pszFormat, ... ) { }
inline void logWarning( const char* pszFormat, ... ) { }
inline void logError( const char* pszFormat, ... ) { }
#else
#define logDebug( s, ... ) printf( s "\n", __VA_ARGS__ )
#define logInfo( s, ... ) printf( s "\n", __VA_ARGS__ )
#define logWarning( s, ... ) printf( s "\n", __VA_ARGS__ )
#define logError( s, ... ) printf( s "\n", __VA_ARGS__ )
#endif