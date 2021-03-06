#pragma once
#include <stdint.h>
#include <vector>
#include "PlexAlloc/Allocator.hpp"
#include <unordered_map>

// We don't need to include FreeType here. Forward declaration is enough, reduce compilation time.
typedef struct FT_FaceRec_* FT_Face;

namespace FontStash2
{
	struct GlyphValue
	{
		uint32_t index;
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
		// Source data, i.e. .TTF file in RAM. Apparently, FreeType2 library doesn't copy it to internal structures.
		std::vector<uint8_t> data;

		// Values from font->ascender/descender, scaled relatively to line height 
		float ascender, descender;
		float lineh;

		// Key for the hash map
		struct GlyphKey
		{
			unsigned int codepoint;
			short size;
#ifndef NANOVG_CLEARTYPE
			short blur;
#endif
			bool operator == ( const GlyphKey &k ) const
			{
				return codepoint == k.codepoint && size == k.size 
#ifndef NANOVG_CLEARTYPE
					&& blur == k.blur
#endif
				;
			}

			GlyphKey() = default;

#ifdef NANOVG_CLEARTYPE
			GlyphKey( unsigned int cp, short s, short b ) :
				codepoint( cp ), size( s ) { }
#else
			GlyphKey( unsigned int cp, short s, short b ) :
				codepoint( cp ), size( s ), blur( b ) { }
#endif
		};

		// Hasher for the above structure, for std::unordered_map
		struct GlyphKeyHash
		{
			std::size_t operator()( const GlyphKey& k ) const
			{
				std::size_t hash = 17;
				hash = hash * 31 + k.codepoint;
				hash = hash * 31 + (uint16_t)k.size;
#ifndef NANOVG_CLEARTYPE
				hash = hash * 31 + (uint16_t)k.blur;
#endif
				return hash;
			}
		};

		// The main hash map which maps (codepoint, size, blur) tuples into GlyphValue structures.
		// Using PlexAlloc from https://github.com/Const-me/CollectionMicrobench to reduce RAM allocations and make it more cache friendly.
		// Original C code used a fixed size hash map with 256 buckets, with linked list for every bucket, and elements stored in a single vector.
		// Original is probably slightly faster for small atlases with ~64 glyphs, but this one scales much better.
		using TAlloc = PlexAlloc::Allocator<std::pair<const GlyphKey, GlyphValue>, 128>;
		using TGlyphsMap = std::unordered_map<GlyphKey, GlyphValue, GlyphKeyHash, std::equal_to<GlyphKey>, TAlloc>;
		TGlyphsMap glyphs;

		// Indices of fall back fonts
		const int maxFallbackFonts;
		std::vector<int> fallbacks;

		void clear();

		GlyphValue* lookupGlyph( const GlyphKey & k ) const;

	public:

		Font( int maxFallbacks );
		~Font() { clear(); }

		// Load FreeType font
		bool initialize( const char* name, std::vector<uint8_t>& buffer );

		// Add index of a fall back font
		bool tryAddFallback( int i );

		// True if the argument is equal to the string in this->name
		bool hasName( const char* str ) const;

		// Lookup a glyph, returns nullptr if not found
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

		uint32_t getGlyphIndex( unsigned int codepoint ) const;

		const std::vector<int> &getFallbackFonts() const
		{
			return fallbacks;
		}

		bool buildGlyphBitmap( int glyph, float size, int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1 ) const;

		GlyphValue* allocGlyph( unsigned int codepoint, short isize, short blur );

#ifdef NANOVG_CLEARTYPE
		void renderGlyphBitmap( uint32_t *output, int outWidth, int outHeight, int outStride ) const;
#else
		void renderGlyphBitmap( unsigned char *output, int outWidth, int outHeight, int outStride ) const;
#endif

		float getVertAlign( bool zeroTopLeft, int align, short isize ) const;

		void fonsVertMetrics( short isize, float* ascender, float* descender, float* lineh ) const;

		void fonsLineBounds( bool zeroTopLeft, short isize, float y, float* miny, float* maxy ) const;
	};

	bool freetypeInit();

	bool freetypeDone();
}