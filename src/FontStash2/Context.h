#include <memory>
#include "Atlas.h"
#include "Font.h"
#include "RamTexture.h"
#include "../fontstash.h"

#ifndef FONS_SCRATCH_BUF_SIZE
#	define FONS_SCRATCH_BUF_SIZE 96000
#endif
#ifndef FONS_HASH_LUT_SIZE
#	define FONS_HASH_LUT_SIZE 256
#endif
#ifndef FONS_INIT_FONTS
#	define FONS_INIT_FONTS 4
#endif
#ifndef FONS_INIT_GLYPHS
#	define FONS_INIT_GLYPHS 256
#endif
#ifndef FONS_INIT_ATLAS_NODES
#	define FONS_INIT_ATLAS_NODES 256
#endif
#ifndef FONS_VERTEX_COUNT
#	define FONS_VERTEX_COUNT 1024
#endif
#ifndef FONS_MAX_STATES
#	define FONS_MAX_STATES 20
#endif
#ifndef FONS_MAX_FALLBACKS
#	define FONS_MAX_FALLBACKS 20
#endif

namespace FontStash2
{
	struct FONSstate
	{
		int font;
		int align;
		float size;
		unsigned int color;
		float blur;
		float spacing;
	};

	class Context
	{
	public:
		FONSparams params;
		float itw, ith;

		RamTexture<uint8_t> texData;
		RamTexture<uint32_t> cleartypeTexture;
		int dirtyRect[ 4 ];
		std::vector<std::unique_ptr<FontStash2::Font>> fonts;
		FontStash2::Atlas atlas;

		float verts[ FONS_VERTEX_COUNT * 2 ];
		float tcoords[ FONS_VERTEX_COUNT * 2 ];
		unsigned int colors[ FONS_VERTEX_COUNT ];
		int nverts = 0;

		std::vector<uint8_t> scratch;
		FONSstate states[ FONS_MAX_STATES ];
		int nstates = 0;
		void( *handleError )( void* uptr, int error, int val );
		void* errorUptr = nullptr;

		void addWhiteRect( int w, int h );

		Context( FONSparams* params );
		Context() = default;

		bool initStuff();

		FONSstate* getState();

		GlyphValue* getGlyph( FONSfont& font, unsigned int codepoint, short isize, short iblur, int bitmapOption );

		float getVertAlign( FONSfont& font, int align, short isize ) const
		{
			return font.getVertAlign( params.flags & FONS_ZERO_TOPLEFT, align, isize );
		}

		void getQuad( FONSfont& font, int prevGlyphIndex, GlyphValue* glyph, float scale, float spacing, float* x, float* y, FONSquad* q );

		void flush();
		void vertex( float x, float y, float s, float t, unsigned int c );

		void pushState();
		void popState();
		void clearState();

		int addFont( const char* name, std::vector<uint8_t>& data );

		int debugDumpAtlas( const char* grayscale, const char* cleartype ) const;
	};
}