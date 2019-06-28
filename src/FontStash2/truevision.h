#pragma once
#include <stdint.h>

namespace Truevision
{
	bool saveGrayscale( const uint8_t* source, int w, int h, const char* path );

	bool saveColor( const uint32_t* source, int w, int h, const char* path );
}