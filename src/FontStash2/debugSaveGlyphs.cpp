#include <vector>
#include <algorithm>
#include "debugSaveGlyphs.h"
#ifdef _MSC_VER
#include "truevision.h"
#include "logger.h"
#include <ft2build.h>
#include FT_FREETYPE_H

static const char* saveDirectory = R"(C:\Temp\2remove\VG\glyphs)";

static bool saveGlyph( const uint8_t* data, const FT_GlyphSlot glyph, const char* path )
{
	const int w = glyph->bitmap.width;
	const int h = glyph->bitmap.rows;
	if( Truevision::saveGrayscale( data, w, h, path ) )
	{
		logDebug( "Saved %s, %ix%i", path, w, h );
		return true;
	}
	else
	{
		logDebug( "Error writing %s, %ix%i", path, w, h );
		return false;
	}
}

bool FontStash2::debugSaveGlyph( const FT_GlyphSlot data, uint32_t index, float size, const char* suffix )
{
	if( nullptr == data->bitmap.buffer )
		return false;

	char path[ 256 ];
	sprintf( path, "%s\\%i-%f-%s.tga", saveDirectory, index, size, suffix );

	if( data->bitmap.pitch == data->bitmap.width )
		return saveGlyph( data->bitmap.buffer, data, path );

	std::vector<uint8_t> buffer;
	buffer.resize( data->bitmap.width * data->bitmap.rows );
	const uint8_t* src = data->bitmap.buffer;
	uint8_t* dest = buffer.data();
	for( uint32_t y = 0; y < data->bitmap.rows; y++ )
	{
		std::copy_n( src, data->bitmap.width, dest );
		src += data->bitmap.pitch;
		dest += data->bitmap.width;
	}

	return saveGlyph( buffer.data(), data, path );
}
#endif