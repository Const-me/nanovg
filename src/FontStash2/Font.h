#pragma once
#include <stdint.h>
#include <vector>
#include "PlexAlloc/Allocator.hpp"
#include <unordered_map>

typedef struct FT_LibraryRec_  *FT_Library;
typedef struct FT_FaceRec_*  FT_Face;

namespace FontStash2
{
	struct GlyphKey
	{
		unsigned int codepoint;
		short size;
		short blur;

		bool operator == ( const GlyphKey &k ) const
		{
			return codepoint == k.codepoint && size == k.size && blur == k.blur;
		}

		GlyphKey() = default;

		GlyphKey( unsigned int cp, short s, short b ) :
			codepoint( cp ), size( s ), blur( b ) { }
	};

	struct GlyphKeyHash
	{
		std::size_t operator()( const GlyphKey& k ) const
		{
			std::size_t hash = 17;
			hash = hash * 31 + k.codepoint;
			hash = hash * 31 + (uint16_t)k.size;
			hash = hash * 31 + (uint16_t)k.blur;
			return hash;
		}
	};

	struct GlyphValue
	{
		int index;
		short x0, y0, x1, y1;
		short xadv, xoff, yoff;

		bool hasBitmap() const
		{
			return x0 >= 0 && y0 >= 0;
		}
	};

	class Font
	{
		FT_Face font = nullptr;
		char name[ 64 ];
		std::vector<uint8_t> data;

		// Values from font->ascender/descender, scaled relatively to line height 
		float ascender, descender;
		float lineh;

		using TAlloc = PlexAlloc::Allocator<std::pair<const GlyphKey, GlyphValue>, 256>;
		using TGlyphsMap = std::unordered_map<GlyphKey, GlyphValue, GlyphKeyHash, std::equal_to<GlyphKey>, TAlloc>;
		TGlyphsMap glyphs;

		std::vector<int> fallbacks;

		void clear();

	public:

		Font( int maxFallbacks );
		~Font() { clear(); }

		bool initialize( FT_Library ftLibrary, const char* name, std::vector<uint8_t>& buffer );

		bool tryAddFallback( int i );

		bool hasName( const char* str ) const;

		GlyphValue* lookupGlyph( const GlyphKey & k ) const;

		GlyphValue* lookupGlyph( unsigned int codepoint, short isize, short blur ) const
		{
			return lookupGlyph( GlyphKey{ codepoint, isize, blur } );
		}

		int getGlyphKernAdvance( int glyph1, int glyph2 ) const;

		bool empty() const
		{
			return data.empty();
		}

		float getPixelHeightScale( float size ) const;

		void reset();

		int getGlyphIndex( unsigned int codepoint ) const;

		const std::vector<int> &getFallbackFonts() const
		{
			return fallbacks;
		}

		bool buildGlyphBitmap( int glyph, float size, float scale,
			int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1 ) const;

		GlyphValue* allocGlyph( unsigned int codepoint, short isize, short blur );

		void renderGlyphBitmap( unsigned char *output, int outWidth, int outHeight, int outStride ) const;

		float getVertAlign( bool zeroTopLeft, int align, short isize ) const;

		void fonsVertMetrics( short isize, float* ascender, float* descender, float* lineh ) const;

		void fonsLineBounds( bool zeroTopLeft, short isize, float y, float* miny, float* maxy ) const;
	};
}