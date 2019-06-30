#pragma once
#include <stdint.h>

// Utility functions to write Truevision TGA images
namespace Truevision
{
	// Save 8-bit grayscale image
	bool saveGrayscale( const uint8_t* source, int w, int h, const char* path );

	// Save 24-bit RGB image. The source is assumed to be 32 bit RGBA, it's converted into 24 bit by discarding the alpha bytes.
	bool saveColor( const uint32_t* source, int w, int h, const char* path );
}