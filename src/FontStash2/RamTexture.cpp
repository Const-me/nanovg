#include "RamTexture.h"
#include "Font.h"
#include "blur.h"
using namespace FontStash2;

template<>
bool RamTexture<uint8_t>::addGlyph( Font& font, int textureWidth, const GlyphValue* glyph, int pad )
{
	const int x = glyph->x0;
	const int y = glyph->y0;
	const int w = glyph->x1 - x;
	const int h = glyph->y1 - y;
	uint8_t* dst = &texture[ x + pad + ( y + pad )* textureWidth ];
	font.renderGlyphBitmap( dst, w - pad * 2, h - pad * 2, textureWidth );

	// Make sure there is one pixel empty border.
	dst = &texture[ x + y * textureWidth ];
	for( int y = 0; y < h; y++ )
	{
		dst[ y * textureWidth ] = 0;
		dst[ w - 1 + y * textureWidth ] = 0;
	}
	for( int x = 0; x < w; x++ )
	{
		dst[ x ] = 0;
		dst[ x + ( h - 1 ) * textureWidth ] = 0;
	}

	// Debug code to color the glyph background
	/*	unsigned char* fdst = &stash->texData[glyph->x0 + glyph->y0 * stash->params.width];
	for (y = 0; y < gh; y++) {
		for (x = 0; x < gw; x++) {
			int a = (int)fdst[x+y*stash->params.width] + 20;
			if (a > 255) a = 255;
			fdst[x+y*stash->params.width] = a;
		}
	} */
	return true;
}

template<>
bool RamTexture<uint32_t>::addCleartypeGlyph( Font& font, float size, int textureWidth, const GlyphValue* glyph, int pad )
{
	const int x = glyph->x0;
	const int y = glyph->y0;
	const int w = glyph->x1 - x;
	const int h = glyph->y1 - y;

	uint32_t* dst = &texture[ x + pad + ( y + pad )* textureWidth ];
	if( !font.renderCleartypeBitmap( glyph, size, dst, w - pad * 2, h - pad * 2, textureWidth ) )
		return false;

	// Make sure there is one pixel empty border.
	dst = &texture[ x + y * textureWidth ];
	for( int y = 0; y < h; y++ )
	{
		dst[ y * textureWidth ] = 0;
		dst[ w - 1 + y * textureWidth ] = 0;
	}
	for( int x = 0; x < w; x++ )
	{
		dst[ x ] = 0;
		dst[ x + ( h - 1 ) * textureWidth ] = 0;
	}
	return true;
}

template<>
void RamTexture<uint8_t>::blurRectangle( int textureWidth, int x, int y, int w, int h, short iblur )
{
	unsigned char* bdst = &texture[ x + y * textureWidth ];
	FontStash2::blur( bdst, w, h, textureWidth, iblur );
}