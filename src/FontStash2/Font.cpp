#include "Font.h"
#include "../fontstash.enums.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <math.h>

#ifndef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
#error You must use FreeType build with FT_CONFIG_OPTION_SUBPIXEL_RENDERING
#endif

using namespace FontStash2;

Font::Font( int maxFallbacks )
{
	fallbacks.reserve( maxFallbacks );
}

bool Font::tryAddFallback( int i )
{
	if( fallbacks.size() < fallbacks.capacity() )
	{
		fallbacks.push_back( i );
		return true;
	}
	return false;
}

bool Font::initialize( FT_Library ftLibrary, const char* name, std::vector<uint8_t>& buffer )
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

int Font::getGlyphIndex( unsigned int codepoint ) const
{
	return FT_Get_Char_Index( font, codepoint );
}

bool Font::buildGlyphBitmap( int glyph, float size, float scale,
	int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1 ) const
{
	FT_Error ftError;
	FT_GlyphSlot ftGlyph;
	FT_Fixed advFixed;

	ftError = FT_Set_Pixel_Sizes( font, 0, (FT_UInt)( size * (float)font->units_per_EM / (float)( font->ascender - font->descender ) ) );
	if( ftError ) return false;
	ftError = FT_Load_Glyph( font, glyph, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT );
	if( ftError ) return false;
	ftError = FT_Get_Advance( font, glyph, FT_LOAD_NO_SCALE, &advFixed );
	if( ftError ) return false;
	ftGlyph = font->glyph;
	*advance = (int)advFixed;
	*lsb = (int)ftGlyph->metrics.horiBearingX;
	*x0 = ftGlyph->bitmap_left;
	*x1 = *x0 + ftGlyph->bitmap.width;
	*y0 = -ftGlyph->bitmap_top;
	*y1 = *y0 + ftGlyph->bitmap.rows;
	return true;
}

GlyphValue* Font::allocGlyph( unsigned int codepoint, short isize, short blur )
{
	const GlyphKey key{ codepoint, isize, blur };
	return &glyphs[ key ];
}

void Font::renderGlyphBitmap( unsigned char *output, int outWidth, int outHeight, int outStride ) const
{
	FT_GlyphSlot ftGlyph = font->glyph;
	int ftGlyphOffset = 0;
	for( uint32_t y = 0; y < ftGlyph->bitmap.rows; y++ )
		for( uint32_t x = 0; x < ftGlyph->bitmap.width; x++ )
			output[ ( y * outStride ) + x ] = ftGlyph->bitmap.buffer[ ftGlyphOffset++ ];
}

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
		*maxy = *miny + lineh*isize / 10.0f;
	}
	else
	{
		*maxy = y + descender * (float)isize / 10.0f;
		*miny = *maxy - lineh*isize / 10.0f;
	}
}