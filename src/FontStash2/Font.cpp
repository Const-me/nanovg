#include <assert.h>
#include "Font.h"
#include "../fontstash.enums.h"
#include "logger.h"
#include "debugSaveGlyphs.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <math.h>

#ifdef NANOVG_CLEARTYPE
#ifndef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
#error You must use FreeType build with FT_CONFIG_OPTION_SUBPIXEL_RENDERING
#endif
#endif

namespace FontStash2
{
	static FT_Library ftLibrary = nullptr;

	bool freetypeInit()
	{
		if( nullptr != ftLibrary )
			return true;
		const FT_Error ftError = FT_Init_FreeType( &ftLibrary );
		return ftError == 0;
	}

	bool freetypeDone()
	{
		if( nullptr == ftLibrary )
			return true;
		const FT_Error ftError = FT_Done_FreeType( ftLibrary );
		ftLibrary = nullptr;
		return ftError == 0;
	}

	constexpr FT_Int32 loadFlagsNormal = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT;
	constexpr FT_Int32 loadFlagsClearType = loadFlagsNormal | FT_LOAD_TARGET_LCD;
#ifdef NANOVG_CLEARTYPE
	constexpr FT_Int32 loadFlags = loadFlagsClearType;
#else
	constexpr FT_Int32 loadFlags = loadFlagsNormal;
#endif
}

using namespace FontStash2;

Font::Font( int maxFallbacks ) :
	maxFallbackFonts( maxFallbacks )
{ }

bool Font::tryAddFallback( int i )
{
	if( (int)fallbacks.size() < maxFallbackFonts )
	{
		fallbacks.push_back( i );
		return true;
	}
	return false;
}

bool Font::initialize( const char* name, std::vector<uint8_t>& buffer )
{
	clear();

	const FT_Error ftError = FT_New_Memory_Face( ftLibrary, buffer.data(), (FT_Long)buffer.size(), 0, &font );
	if( ftError != 0 )
	{
		// logError( "FT_New_Memory_Face failed" );
		return false;
	}

	const int ascent = font->ascender;
	const int descent = font->descender;
	const int lineGap = font->height - ( ascent - descent );

	const int fh = ascent - descent;
	ascender = (float)ascent / (float)fh;
	descender = (float)descent / (float)fh;
	lineh = (float)( fh + lineGap ) / (float)fh;

	strncpy( this->name, name, sizeof( this->name ) );
	this->name[ sizeof( this->name ) - 1 ] = '\0';
	data.swap( buffer );

	return true;
}

void Font::clear()
{
	if( nullptr != font )
	{
		FT_Done_Face( font );
		font = nullptr;
	}
	glyphs.clear();
	fallbacks.clear();
}

void Font::reset()
{
	glyphs.clear();
}

bool Font::hasName( const char* str ) const
{
	return 0 == strcmp( str, name );
}

GlyphValue* Font::lookupGlyph( const GlyphKey & k ) const
{
	auto it = glyphs.find( k );
	if( it == glyphs.end() )
		return nullptr;
	return const_cast<GlyphValue*>( &it->second );
}

int Font::getGlyphKernAdvance( int glyph1, int glyph2 ) const
{
	FT_Vector ftKerning;
	FT_Get_Kerning( font, glyph1, glyph2, FT_KERNING_DEFAULT, &ftKerning );
	return (int)( ( ftKerning.x + 32 ) >> 6 );  // Round up and convert to integer
}

float Font::getPixelHeightScale( float size ) const
{
	return size / ( font->ascender - font->descender );
}

uint32_t Font::getGlyphIndex( unsigned int codepoint ) const
{
	return FT_Get_Char_Index( font, codepoint );
}

bool Font::buildGlyphBitmap( int glyph, float size, int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1 ) const
{
	FT_Error ftError = FT_Set_Pixel_Sizes( font, 0, (FT_UInt)( size * (float)font->units_per_EM / (float)( font->ascender - font->descender ) ) );
	if( ftError ) return false;
	ftError = FT_Load_Glyph( font, glyph, loadFlags );
	if( ftError ) return false;

	FT_Fixed advFixed;
	ftError = FT_Get_Advance( font, glyph, FT_LOAD_NO_SCALE, &advFixed );
	if( ftError ) return false;
	FT_GlyphSlot ftGlyph = font->glyph;
	*advance = (int)advFixed;
	*lsb = (int)ftGlyph->metrics.horiBearingX;
	*x0 = ftGlyph->bitmap_left;
#ifdef NANOVG_CLEARTYPE
	assert( 0 == ( ftGlyph->bitmap.width % 3 ) );
	*x1 = *x0 + ftGlyph->bitmap.width / 3;
#else
	*x1 = *x0 + ftGlyph->bitmap.width;
#endif
	*y0 = -ftGlyph->bitmap_top;
	*y1 = *y0 + ftGlyph->bitmap.rows;
	logDebug( "Font::buildGlyphBitmap: glyph %i, size %f, outHeight %i", glyph, size, ftGlyph->bitmap.rows );

	debugSaveGlyph( ftGlyph, glyph, size, "gray" );
	return true;
}

GlyphValue* Font::allocGlyph( unsigned int codepoint, short isize, short blur )
{
	const GlyphKey key{ codepoint, isize, blur };
	return &glyphs[ key ];
}

#ifdef NANOVG_CLEARTYPE

// Pack 3 grayscale sub-pixel bytes into a single RGBA pixel.
inline uint32_t packCleartypeSubpixels( const uint8_t* triple )
{
	uint32_t res = triple[ 0 ];
	res |= ( ( (uint32_t)triple[ 1 ] ) << 8 );
	res |= ( ( (uint32_t)triple[ 2 ] ) << 16 );
	// On PC GPUs, it's much faster to do in pixel shader. On slow embedded ARM this is probably not the case.
	// Ideally we might want max, not bitwise OR, but OR is much faster to compute in scalar code, too bad scalar parts of CPUs don't have instructions like pmaxub (PC) / umax (ARM).
	// This code only runs while building textures, i.e. much less frequent than each frame.
	const uint32_t all = triple[ 0 ] | triple[ 1 ] | triple[ 2 ];
	res |= ( all << 24 );
	return res;
}

void Font::renderGlyphBitmap( uint32_t *output, int outWidth, int outHeight, int outStride ) const
{
	FT_GlyphSlot ftGlyph = font->glyph;
	const int rgbWidth = ftGlyph->bitmap.width / 3;
	const uint8_t* sourceLine = ftGlyph->bitmap.buffer;
	const size_t sourceStride = ftGlyph->bitmap.pitch;

	for( uint32_t y = 0; y < ftGlyph->bitmap.rows; y++ )
	{
		const uint8_t* src = sourceLine;
		uint32_t *dest = output;
		for( int x = 0; x < rgbWidth; x++ )
		{
			*dest = packCleartypeSubpixels( src );
			src += 3;
			dest++;
		}
		sourceLine += sourceStride;
		output += outStride;
	}
}

#else

void Font::renderGlyphBitmap( unsigned char *output, int outWidth, int outHeight, int outStride ) const
{
	FT_GlyphSlot ftGlyph = font->glyph;
	const uint8_t* sourceLine = ftGlyph->bitmap.buffer;	// Read pointer
	const size_t sourceStride = ftGlyph->bitmap.pitch;	// Bytes to skip between source lines
	const uint32_t height = ftGlyph->bitmap.rows;	// Rows to copy

	const size_t lineWidth = ftGlyph->bitmap.width;	// Bytes to copy per each line

	for( uint32_t y = 0; y < height; y++ )
	{
		std::copy_n( sourceLine, lineWidth, output );
		sourceLine += sourceStride;
		output += outStride;
	}
}
#endif

float Font::getVertAlign( bool zeroTopLeft, int align, short isize ) const
{
	if( zeroTopLeft )
	{
		if( align & FONS_ALIGN_TOP )
			return ascender * (float)isize / 10.0f;
		if( align & FONS_ALIGN_MIDDLE )
			return ( ascender + descender ) / 2.0f * (float)isize / 10.0f;
		if( align & FONS_ALIGN_BASELINE )
			return 0.0f;
		if( align & FONS_ALIGN_BOTTOM )
			return descender * (float)isize / 10.0f;
	}
	else
	{
		if( align & FONS_ALIGN_TOP )
			return -ascender * (float)isize / 10.0f;
		if( align & FONS_ALIGN_MIDDLE )
			return -( ascender + descender ) / 2.0f * (float)isize / 10.0f;
		if( align & FONS_ALIGN_BASELINE )
			return 0.0f;
		if( align & FONS_ALIGN_BOTTOM )
			return -descender * (float)isize / 10.0f;
	}
	return 0.0;
}

void Font::fonsVertMetrics( short isize, float* ascender, float* descender, float* lineh ) const
{
	if( ascender )
		*ascender = this->ascender * isize / 10.0f;
	if( descender )
		*descender = this->descender * isize / 10.0f;
	if( lineh )
		*lineh = this->lineh * isize / 10.0f;
}

void Font::fonsLineBounds( bool zeroTopLeft, short isize, float y, float* miny, float* maxy ) const
{
	if( zeroTopLeft )
	{
		*miny = y - ascender * (float)isize / 10.0f;
		*maxy = *miny + lineh * isize / 10.0f;
	}
	else
	{
		*maxy = y + descender * (float)isize / 10.0f;
		*miny = *maxy - lineh * isize / 10.0f;
	}
}