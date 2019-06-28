#include <memory>
#include "fontstash.h"
#include "FontStash2/Atlas.h"
#include "FontStash2/utf8.h"

using FONSglyph = FontStash2::GlyphValue;

#define FONS_NOTUSED(v)  (void)sizeof(v)

struct FONSttFontImpl
{
	FT_Face font;
};
typedef struct FONSttFontImpl FONSttFontImpl;

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

static int fons__mini( int a, int b )
{
	return a < b ? a : b;
}

static int fons__maxi( int a, int b )
{
	return a > b ? a : b;
}

struct FONSstate
{
	int font;
	int align;
	float size;
	unsigned int color;
	float blur;
	float spacing;
};
typedef struct FONSstate FONSstate;

using FONSatlas = FontStash2::Atlas;

class FONScontext
{
public:
	FONSparams params;
	float itw, ith;

	// unsigned char* texData;
	std::vector<uint8_t> texData;
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

	FONScontext( FONSparams* params );
	FONScontext() = default;

	bool initStuff();

	FONSstate* getState();

	FONSglyph* getGlyph( FONSfont& font, unsigned int codepoint, short isize, short iblur, int bitmapOption );

	float getVertAlign( FONSfont& font, int align, short isize ) const
	{
		return font.getVertAlign( params.flags & FONS_ZERO_TOPLEFT, align, isize );
	}

	void getQuad( FONSfont& font, int prevGlyphIndex, FONSglyph* glyph, float scale, float spacing, float* x, float* y, FONSquad* q );

	void flush();
	void vertex( float x, float y, float s, float t, unsigned int c );

	void pushState();
	void popState();
	void clearState();

	int addFont( const char* name, std::vector<uint8_t>& data );
};

// ===== Constructor and destructor =====
FONScontext* fonsCreateInternal( FONSparams* params )
{
	try
	{
		auto up = std::make_unique< FONScontext>( params );
		if( !up->initStuff() )
			return nullptr;
		return up.release();
	}
	catch( std::exception& )
	{
		return nullptr;
	}
}

void fonsDeleteInternal( FONScontext* stash )
{
	delete stash;
	FontStash2::freetypeDone();
}

void fonsSetErrorCallback( FONScontext* stash, void( *callback )( void* uptr, int error, int val ), void* uptr )
{
	if( nullptr == stash )
		return;
	stash->handleError = callback;
	stash->errorUptr = uptr;
}

// ===== Atlas =====
void fonsGetAtlasSize( FONScontext* stash, int* width, int* height )
{
	if( stash == NULL )
		return;
	*width = stash->params.width;
	*height = stash->params.height;
}

int fonsExpandAtlas( FONScontext* stash, int width, int height )
{
	if( stash == NULL )
		return 0;

	width = fons__maxi( width, stash->params.width );
	height = fons__maxi( height, stash->params.height );

	if( width == stash->params.width && height == stash->params.height )
		return 1;

	// Flush pending glyphs.
	stash->flush();

	// Create new texture
	if( stash->params.renderResize != NULL ) {
		if( stash->params.renderResize( stash->params.userPtr, width, height ) == 0 )
			return 0;
	}

	std::vector<uint8_t> data;
	try
	{
		data.resize( width * height );
	}
	catch( std::exception& )
	{
		return 0;
	}

	for( int i = 0; i < stash->params.height; i++ )
	{
		unsigned char* dst = &data[ i*width ];
		unsigned char* src = &stash->texData[ i*stash->params.width ];
		memcpy( dst, src, stash->params.width );
		if( width > stash->params.width )
			memset( dst + stash->params.width, 0, width - stash->params.width );
	}
	if( height > stash->params.height )
		memset( &data[ stash->params.height * width ], 0, ( height - stash->params.height ) * width );

	stash->texData.swap( data );

	// Increase atlas size
	stash->atlas.expand( width, height );

	// Add existing data as dirty.
	const int maxy = stash->atlas.getMaxY();
	stash->dirtyRect[ 0 ] = 0;
	stash->dirtyRect[ 1 ] = 0;
	stash->dirtyRect[ 2 ] = stash->params.width;
	stash->dirtyRect[ 3 ] = maxy;

	stash->params.width = width;
	stash->params.height = height;
	stash->itw = 1.0f / stash->params.width;
	stash->ith = 1.0f / stash->params.height;

	return 1;
}

int fonsResetAtlas( FONScontext* stash, int width, int height )
{
	if( stash == NULL ) return 0;

	// Flush pending glyphs.
	stash->flush();

	// Create new texture
	if( stash->params.renderResize != NULL )
	{
		if( stash->params.renderResize( stash->params.userPtr, width, height ) == 0 )
			return 0;
	}

	// Reset atlas
	stash->atlas.reset( width, height );

	// Clear texture data.
	stash->texData.resize( width * height );
	memset( stash->texData.data(), 0, width * height );

	// Reset dirty rect
	stash->dirtyRect[ 0 ] = width;
	stash->dirtyRect[ 1 ] = height;
	stash->dirtyRect[ 2 ] = 0;
	stash->dirtyRect[ 3 ] = 0;

	// Reset cached glyphs
	for( auto& f : stash->fonts )
		f->reset();

	stash->params.width = width;
	stash->params.height = height;
	stash->itw = 1.0f / stash->params.width;
	stash->ith = 1.0f / stash->params.height;

	// Add white rect at 0,0 for debug drawing.
	stash->addWhiteRect( 2, 2 );
	return 1;
}

// ===== Add fonts =====
int fonsAddFont( FONScontext* stash, const char* name, const char* path )
{
	// Read in the font data.
	FILE* const fp = fopen( path, "rb" );
	if( fp == NULL )
		return FONS_INVALID;
	fseek( fp, 0, SEEK_END );
	const int dataSize = (int)ftell( fp );
	fseek( fp, 0, SEEK_SET );

	std::vector<uint8_t> data;
	try
	{
		data.resize( dataSize );
	}
	catch( std::exception& )
	{
		fclose( fp );
		return FONS_INVALID;
	}
	const size_t readed = fread( data.data(), 1, dataSize, fp );
	fclose( fp );
	if( readed != dataSize )
		return FONS_INVALID;

	return stash->addFont( name, data );
}

int fonsAddFontMem( FONScontext* stash, const char* name, unsigned char* data, int dataSize, int freeData )
{
	std::vector<uint8_t> dataVector{ data, data + dataSize };
	if( freeData )
		free( data );

	return stash->addFont( name, dataVector );
}

int fonsGetFontByName( FONScontext* s, const char* name )
{
	const int count = (int)s->fonts.size();
	for( int i = 0; i < count; i++ )
		if( s->fonts[ i ]->hasName( name ) )
			return i;
	return FONS_INVALID;
}

// State handling
void fonsPushState( FONScontext* s )
{
	if( nullptr == s )
		return;
	s->pushState();
}

void fonsPopState( FONScontext* s )
{
	if( nullptr == s )
		return;
	s->popState();
}

void fonsClearState( FONScontext* s )
{
	if( nullptr == s )
		return;
	s->clearState();
}

// ===== State setting =====
void fonsSetSize( FONScontext* stash, float size )
{
	stash->getState()->size = size;
}

void fonsSetColor( FONScontext* stash, unsigned int color )
{
	stash->getState()->color = color;
}

void fonsSetSpacing( FONScontext* stash, float spacing )
{
	stash->getState()->spacing = spacing;
}

void fonsSetBlur( FONScontext* stash, float blur )
{
	stash->getState()->blur = blur;
}

void fonsSetAlign( FONScontext* stash, int align )
{
	stash->getState()->align = align;
}

void fonsSetFont( FONScontext* stash, int font )
{
	stash->getState()->font = font;
}

// ===== Draw text =====
float fonsDrawText( FONScontext* stash, float x, float y, const char* str, const char* end )
{
	FONSstate* state = stash->getState();
	unsigned int codepoint;
	unsigned int utf8state = 0;
	FONSglyph* glyph = NULL;
	FONSquad q;
	int prevGlyphIndex = -1;
	short isize = (short)( state->size*10.0f );
	short iblur = (short)state->blur;
	float scale, width;

	if( stash == NULL ) return x;
	if( state->font < 0 || state->font >= stash->fonts.size() ) return x;
	FONSfont &font = *stash->fonts[ state->font ];
	if( font.empty() ) return x;

	scale = font.getPixelHeightScale( (float)isize / 10.0f );

	if( end == NULL )
		end = str + strlen( str );

	// Align horizontally
	if( state->align & FONS_ALIGN_LEFT )
	{
		// empty
	}
	else if( state->align & FONS_ALIGN_RIGHT )
	{
		width = fonsTextBounds( stash, x, y, str, end, NULL );
		x -= width;
	}
	else if( state->align & FONS_ALIGN_CENTER )
	{
		width = fonsTextBounds( stash, x, y, str, end, NULL );
		x -= width * 0.5f;
	}
	// Align vertically.
	y += stash->getVertAlign( font, state->align, isize );

	for( ; str != end; ++str )
	{
		if( FontStash2::decodeUTF8( &utf8state, &codepoint, *(const unsigned char*)str ) )
			continue;
		glyph = stash->getGlyph( font, codepoint, isize, iblur, FONS_GLYPH_BITMAP_REQUIRED );
		if( glyph != NULL )
		{
			stash->getQuad( font, prevGlyphIndex, glyph, scale, state->spacing, &x, &y, &q );

			if( stash->nverts + 6 > FONS_VERTEX_COUNT )
				stash->flush();

			stash->vertex( q.x0, q.y0, q.s0, q.t0, state->color );
			stash->vertex( q.x1, q.y1, q.s1, q.t1, state->color );
			stash->vertex( q.x1, q.y0, q.s1, q.t0, state->color );

			stash->vertex( q.x0, q.y0, q.s0, q.t0, state->color );
			stash->vertex( q.x0, q.y1, q.s0, q.t1, state->color );
			stash->vertex( q.x1, q.y1, q.s1, q.t1, state->color );
		}
		prevGlyphIndex = glyph != NULL ? glyph->index : -1;
	}
	stash->flush();

	return x;
}

// ===== Measure text =====
float fonsTextBounds( FONScontext* stash, float x, float y, const char* str, const char* end, float* bounds )
{
	FONSstate* state = stash->getState();
	unsigned int codepoint;
	unsigned int utf8state = 0;
	FONSquad q;
	FONSglyph* glyph = NULL;
	int prevGlyphIndex = -1;
	short isize = (short)( state->size*10.0f );
	short iblur = (short)state->blur;
	float scale;
	;
	float startx, advance;
	float minx, miny, maxx, maxy;

	if( stash == NULL ) return 0;
	if( state->font < 0 || state->font >= stash->fonts.size() ) return 0;
	FONSfont &font = *stash->fonts[ state->font ];
	if( font.empty() ) return 0;

	scale = font.getPixelHeightScale( (float)isize / 10.0f );

	// Align vertically.
	y += stash->getVertAlign( font, state->align, isize );

	minx = maxx = x;
	miny = maxy = y;
	startx = x;

	if( end == NULL )
		end = str + strlen( str );

	for( ; str != end; ++str )
	{
		if( FontStash2::decodeUTF8( &utf8state, &codepoint, *(const unsigned char*)str ) )
			continue;
		glyph = stash->getGlyph( font, codepoint, isize, iblur, FONS_GLYPH_BITMAP_OPTIONAL );
		if( glyph != NULL )
		{
			stash->getQuad( font, prevGlyphIndex, glyph, scale, state->spacing, &x, &y, &q );
			if( q.x0 < minx ) minx = q.x0;
			if( q.x1 > maxx ) maxx = q.x1;
			if( stash->params.flags & FONS_ZERO_TOPLEFT ) {
				if( q.y0 < miny ) miny = q.y0;
				if( q.y1 > maxy ) maxy = q.y1;
			}
			else {
				if( q.y1 < miny ) miny = q.y1;
				if( q.y0 > maxy ) maxy = q.y0;
			}
		}
		prevGlyphIndex = glyph != NULL ? glyph->index : -1;
	}

	advance = x - startx;

	// Align horizontally
	if( state->align & FONS_ALIGN_LEFT ) {
		// empty
	}
	else if( state->align & FONS_ALIGN_RIGHT ) {
		minx -= advance;
		maxx -= advance;
	}
	else if( state->align & FONS_ALIGN_CENTER ) {
		minx -= advance * 0.5f;
		maxx -= advance * 0.5f;
	}

	if( bounds ) {
		bounds[ 0 ] = minx;
		bounds[ 1 ] = miny;
		bounds[ 2 ] = maxx;
		bounds[ 3 ] = maxy;
	}

	return advance;
}

void fonsLineBounds( FONScontext* stash, float y, float* miny, float* maxy )
{
	FONSstate* const state = stash->getState();

	if( stash == NULL )
		return;
	if( state->font < 0 || state->font >= stash->fonts.size() )
		return;
	FONSfont &font = *stash->fonts[ state->font ];
	const short isize = (short)( state->size*10.0f );
	if( font.empty() ) return;
	y += stash->getVertAlign( font, state->align, isize );
	font.fonsLineBounds( stash->params.flags & FONS_ZERO_TOPLEFT, isize, y, miny, maxy );
}

void fonsVertMetrics( FONScontext* stash, float* ascender, float* descender, float* lineh )
{
	FONSstate* state = stash->getState();
	if( stash == NULL ) return;
	if( state->font < 0 || state->font >= stash->fonts.size() )
		return;
	FONSfont& font = *stash->fonts[ state->font ];
	const short isize = (short)( state->size * 10.0f );
	if( font.empty() )
		return;
	font.fonsVertMetrics( isize, ascender, descender, lineh );
}

// ===== Text iterator =====
int fonsTextIterInit( FONScontext* stash, FONStextIter* iter,
	float x, float y, const char* str, const char* end, int bitmapOption )
{
	FONSstate* state = stash->getState();
	float width;

	memset( iter, 0, sizeof( *iter ) );

	if( stash == NULL ) return 0;
	if( state->font < 0 || state->font >= stash->fonts.size() ) return 0;
	iter->font = stash->fonts[ state->font ].get();
	if( iter->font->empty() ) return 0;

	iter->isize = (short)( state->size*10.0f );
	iter->iblur = (short)state->blur;
	iter->scale = iter->font->getPixelHeightScale( (float)iter->isize / 10.0f );

	// Align horizontally
	if( state->align & FONS_ALIGN_LEFT ) {
		// empty
	}
	else if( state->align & FONS_ALIGN_RIGHT ) {
		width = fonsTextBounds( stash, x, y, str, end, NULL );
		x -= width;
	}
	else if( state->align & FONS_ALIGN_CENTER ) {
		width = fonsTextBounds( stash, x, y, str, end, NULL );
		x -= width * 0.5f;
	}
	// Align vertically.
	y += stash->getVertAlign( *iter->font, state->align, iter->isize );

	if( end == NULL )
		end = str + strlen( str );

	iter->x = iter->nextx = x;
	iter->y = iter->nexty = y;
	iter->spacing = state->spacing;
	iter->str = str;
	iter->next = str;
	iter->end = end;
	iter->codepoint = 0;
	iter->prevGlyphIndex = -1;
	iter->bitmapOption = bitmapOption;

	return 1;
}

int fonsTextIterNext( FONScontext* stash, FONStextIter* iter, FONSquad* quad )
{
	FONSglyph* glyph = NULL;
	const char* str = iter->next;
	iter->str = iter->next;

	if( str == iter->end )
		return 0;

	for( ; str != iter->end; str++ )
	{
		if( FontStash2::decodeUTF8( &iter->utf8state, &iter->codepoint, *(const unsigned char*)str ) )
			continue;
		str++;
		// Get glyph and quad
		iter->x = iter->nextx;
		iter->y = iter->nexty;
		glyph = stash->getGlyph( *iter->font, iter->codepoint, iter->isize, iter->iblur, iter->bitmapOption );
		// If the iterator was initialized with FONS_GLYPH_BITMAP_OPTIONAL, then the UV coordinates of the quad will be invalid.
		if( glyph != NULL )
			stash->getQuad( *iter->font, iter->prevGlyphIndex, glyph, iter->scale, iter->spacing, &iter->nextx, &iter->nexty, quad );
		iter->prevGlyphIndex = glyph != NULL ? glyph->index : -1;
		break;
	}
	iter->next = str;

	return 1;
}

// ===== Pull texture changes =====
const unsigned char* fonsGetTextureData( FONScontext* stash, int* width, int* height )
{
	if( width != NULL )
		*width = stash->params.width;
	if( height != NULL )
		*height = stash->params.height;
	return stash->texData.data();
}

int fonsValidateTexture( FONScontext* stash, int* dirty )
{
	if( stash->dirtyRect[ 0 ] < stash->dirtyRect[ 2 ] && stash->dirtyRect[ 1 ] < stash->dirtyRect[ 3 ] ) {
		dirty[ 0 ] = stash->dirtyRect[ 0 ];
		dirty[ 1 ] = stash->dirtyRect[ 1 ];
		dirty[ 2 ] = stash->dirtyRect[ 2 ];
		dirty[ 3 ] = stash->dirtyRect[ 3 ];
		// Reset dirty rect
		stash->dirtyRect[ 0 ] = stash->params.width;
		stash->dirtyRect[ 1 ] = stash->params.height;
		stash->dirtyRect[ 2 ] = 0;
		stash->dirtyRect[ 3 ] = 0;
		return 1;
	}
	return 0;
}

// ===== Miscellaneous =====
void fonsDrawDebug( FONScontext* stash, float x, float y )
{
	int w = stash->params.width;
	int h = stash->params.height;
	float u = w == 0 ? 0 : ( 1.0f / w );
	float v = h == 0 ? 0 : ( 1.0f / h );

	if( stash->nverts + 6 + 6 > FONS_VERTEX_COUNT )
		stash->flush();

	// Draw background
	stash->vertex( x + 0, y + 0, u, v, 0x0fffffff );
	stash->vertex( x + w, y + h, u, v, 0x0fffffff );
	stash->vertex( x + w, y + 0, u, v, 0x0fffffff );

	stash->vertex( x + 0, y + 0, u, v, 0x0fffffff );
	stash->vertex( x + 0, y + h, u, v, 0x0fffffff );
	stash->vertex( x + w, y + h, u, v, 0x0fffffff );

	// Draw texture
	stash->vertex( x + 0, y + 0, 0, 0, 0xffffffff );
	stash->vertex( x + w, y + h, 1, 1, 0xffffffff );
	stash->vertex( x + w, y + 0, 1, 0, 0xffffffff );

	stash->vertex( x + 0, y + 0, 0, 0, 0xffffffff );
	stash->vertex( x + 0, y + h, 0, 1, 0xffffffff );
	stash->vertex( x + w, y + h, 1, 1, 0xffffffff );

	// Drawbug draw atlas
	for( const auto& n : stash->atlas.atlasNodes() )
	{
		if( stash->nverts + 6 > FONS_VERTEX_COUNT )
			stash->flush();

		stash->vertex( x + n.x + 0, y + n.y + 0, u, v, 0xc00000ff );
		stash->vertex( x + n.x + n.width, y + n.y + 1, u, v, 0xc00000ff );
		stash->vertex( x + n.x + n.width, y + n.y + 0, u, v, 0xc00000ff );

		stash->vertex( x + n.x + 0, y + n.y + 0, u, v, 0xc00000ff );
		stash->vertex( x + n.x + 0, y + n.y + 1, u, v, 0xc00000ff );
		stash->vertex( x + n.x + n.width, y + n.y + 1, u, v, 0xc00000ff );
	}

	stash->flush();
}

int fonsAddFallbackFont( FONScontext* stash, int base, int fallback )
{
	FONSfont* baseFont = stash->fonts[ base ].get();
	return baseFont->tryAddFallback( fallback );
}

// ======== Implementatoin details ========
FONScontext::FONScontext( FONSparams* p ) :
	params( *p ),
	itw( 1.0f / (float)params.width ),
	ith( 1.0f / (float)params.height ),
	atlas( params.width, params.height, FONS_INIT_ATLAS_NODES )
{
	memset( states, 0, sizeof( states ) );
}

bool FONScontext::initStuff()
{
	if( !FontStash2::freetypeInit() )
		return false;

	// Allocate space for fonts
	fonts.reserve( FONS_INIT_FONTS );

	// Create texture for the cache
	texData.resize( params.width * params.height, 0 );

	dirtyRect[ 0 ] = params.width;
	dirtyRect[ 1 ] = params.height;
	dirtyRect[ 2 ] = 0;
	dirtyRect[ 3 ] = 0;

	// Add white rect at 0,0 for debug drawing.
	addWhiteRect( 2, 2 );

	pushState();
	clearState();

	return true;
}

void FONScontext::addWhiteRect( int w, int h )
{
	int x, y, gx, gy;
	unsigned char* dst;
	if( !atlas.addRect( w, h, &gx, &gy ) )
		return;

	// Rasterize
	dst = &texData[ gx + gy * params.width ];
	for( y = 0; y < h; y++ ) {
		for( x = 0; x < w; x++ )
			dst[ x ] = 0xff;
		dst += params.width;
	}

	dirtyRect[ 0 ] = fons__mini( dirtyRect[ 0 ], gx );
	dirtyRect[ 1 ] = fons__mini( dirtyRect[ 1 ], gy );
	dirtyRect[ 2 ] = fons__maxi( dirtyRect[ 2 ], gx + w );
	dirtyRect[ 3 ] = fons__maxi( dirtyRect[ 3 ], gy + h );
}


void FONScontext::pushState()
{
	if( nstates >= FONS_MAX_STATES )
	{
		if( handleError )
			handleError( errorUptr, FONS_STATES_OVERFLOW, 0 );
		return;
	}
	if( nstates > 0 )
		memcpy( &states[ nstates ], &states[ nstates - 1 ], sizeof( FONSstate ) );
	nstates++;
}

void FONScontext::popState()
{
	if( nstates <= 1 )
	{
		if( handleError )
			handleError( errorUptr, FONS_STATES_UNDERFLOW, 0 );
		return;
	}
	nstates--;
}

void FONScontext::clearState()
{
	FONSstate* state = getState();
	state->size = 12.0f;
	state->color = 0xffffffff;
	state->font = 0;
	state->blur = 0;
	state->spacing = 0;
	state->align = FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE;
}

FONSstate* FONScontext::getState()
{
	return &states[ nstates - 1 ];
}

int FONScontext::addFont( const char* name, std::vector<uint8_t>& data )
{
	try
	{
		// Create the object
		auto up = std::make_unique<FONSfont>( FONS_MAX_FALLBACKS );

		// Load the FreeType2 font
		if( !up->initialize( name, data ) )
			return FONS_INVALID;

		// Move the new object to the vector
		const int res = (int)fonts.size();
		fonts.emplace_back( std::move( up ) );
		return res;
	}
	catch( std::exception& )
	{
		return FONS_INVALID;
	}
}

// Based on Exponential blur, Jani Huhtanen, 2006

#define APREC 16
#define ZPREC 7

static void fons__blurCols( unsigned char* dst, int w, int h, int dstStride, int alpha )
{
	int x, y;
	for( y = 0; y < h; y++ ) {
		int z = 0; // force zero border
		for( x = 1; x < w; x++ ) {
			z += ( alpha * ( ( (int)( dst[ x ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ x ] = (unsigned char)( z >> ZPREC );
		}
		dst[ w - 1 ] = 0; // force zero border
		z = 0;
		for( x = w - 2; x >= 0; x-- ) {
			z += ( alpha * ( ( (int)( dst[ x ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ x ] = (unsigned char)( z >> ZPREC );
		}
		dst[ 0 ] = 0; // force zero border
		dst += dstStride;
	}
}

static void fons__blurRows( unsigned char* dst, int w, int h, int dstStride, int alpha )
{
	int x, y;
	for( x = 0; x < w; x++ ) {
		int z = 0; // force zero border
		for( y = dstStride; y < h*dstStride; y += dstStride ) {
			z += ( alpha * ( ( (int)( dst[ y ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ y ] = (unsigned char)( z >> ZPREC );
		}
		dst[ ( h - 1 )*dstStride ] = 0; // force zero border
		z = 0;
		for( y = ( h - 2 )*dstStride; y >= 0; y -= dstStride ) {
			z += ( alpha * ( ( (int)( dst[ y ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ y ] = (unsigned char)( z >> ZPREC );
		}
		dst[ 0 ] = 0; // force zero border
		dst++;
	}
}

static void fons__blur( unsigned char* dst, int w, int h, int dstStride, int blur )
{
	int alpha;
	float sigma;

	if( blur < 1 )
		return;
	// Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
	sigma = (float)blur * 0.57735f; // 1 / sqrt(3)
	alpha = (int)( ( 1 << APREC ) * ( 1.0f - expf( -2.3f / ( sigma + 1.0f ) ) ) );
	fons__blurRows( dst, w, h, dstStride, alpha );
	fons__blurCols( dst, w, h, dstStride, alpha );
	fons__blurRows( dst, w, h, dstStride, alpha );
	fons__blurCols( dst, w, h, dstStride, alpha );
	//	fons__blurrows(dst, w, h, dstStride, alpha);
	//	fons__blurcols(dst, w, h, dstStride, alpha);
}

FONSglyph* FONScontext::getGlyph( FONSfont& font, unsigned int codepoint, short isize, short iblur, int bitmapOption )
{
	float size = isize / 10.0f;
	FONSfont* renderFont = &font;

	if( isize < 2 )
		return NULL;
	if( iblur > 20 ) iblur = 20;
	int pad = iblur + 2;

	// Reset allocator.
	scratch.clear();

	// Find code point and size.
	FONSglyph* glyph = font.lookupGlyph( codepoint, isize, iblur );
	if( nullptr != glyph )
		if( bitmapOption == FONS_GLYPH_BITMAP_OPTIONAL || glyph->hasBitmap() )
			return glyph;

	// Create a new glyph or rasterize bitmap data for a cached glyph.
	int g = font.getGlyphIndex( codepoint );
	// Try to find the glyph in fallback fonts.
	if( g == 0 )
	{
		for( int idxFallback : font.getFallbackFonts() )
		{
			FONSfont* fallbackFont = fonts[ idxFallback ].get();
			int fallbackIndex = fallbackFont->getGlyphIndex( codepoint );
			if( fallbackIndex != 0 )
			{
				g = fallbackIndex;
				renderFont = fallbackFont;
				break;
			}
		}
		// It is possible that we did not find a fallback glyph.
		// In that case the glyph index 'g' is 0, and we'll proceed below and cache empty glyph.
	}
	const float scale = renderFont->getPixelHeightScale( size );
	int advance, lsb, x0, y0, x1, y1;
	renderFont->buildGlyphBitmap( g, size, scale, &advance, &lsb, &x0, &y0, &x1, &y1 );
	const int gw = x1 - x0 + pad * 2;
	const int gh = y1 - y0 + pad * 2;

	// Determines the spot to draw glyph in the atlas.
	int gx, gy;
	if( bitmapOption == FONS_GLYPH_BITMAP_REQUIRED )
	{
		// Find free spot for the rect in the atlas
		bool added = atlas.addRect( gw, gh, &gx, &gy );
		if( !added && handleError != NULL )
		{
			// Atlas is full, let the user to resize the atlas (or not), and try again.
			handleError( errorUptr, FONS_ATLAS_FULL, 0 );
			added = atlas.addRect( gw, gh, &gx, &gy );
		}
		if( !added )
			return NULL;
	}
	else
	{
		// Negative coordinate indicates there is no bitmap data created.
		gx = -1;
		gy = -1;
	}

	// Init glyph.
	if( glyph == NULL )
		glyph = font.allocGlyph( codepoint, isize, iblur );

	glyph->index = g;
	glyph->x0 = (short)gx;
	glyph->y0 = (short)gy;
	glyph->x1 = (short)( glyph->x0 + gw );
	glyph->y1 = (short)( glyph->y0 + gh );
	glyph->xadv = (short)( scale * advance * 10.0f );
	glyph->xoff = (short)( x0 - pad );
	glyph->yoff = (short)( y0 - pad );

	if( bitmapOption == FONS_GLYPH_BITMAP_OPTIONAL )
		return glyph;

	// Rasterize
	unsigned char* dst = &texData[ ( glyph->x0 + pad ) + ( glyph->y0 + pad ) * params.width ];
	renderFont->renderGlyphBitmap( dst, gw - pad * 2, gh - pad * 2, params.width );

	// Make sure there is one pixel empty border.
	dst = &texData[ glyph->x0 + glyph->y0 * params.width ];
	for( int y = 0; y < gh; y++ )
	{
		dst[ y*params.width ] = 0;
		dst[ gw - 1 + y * params.width ] = 0;
	}
	for( int x = 0; x < gw; x++ )
	{
		dst[ x ] = 0;
		dst[ x + ( gh - 1 )*params.width ] = 0;
	}

	// Debug code to color the glyph background
/*	unsigned char* fdst = &stash->texData[glyph->x0 + glyph->y0 * stash->params.width];
	for (y = 0; y < gh; y++) {
		for (x = 0; x < gw; x++) {
			int a = (int)fdst[x+y*stash->params.width] + 20;
			if (a > 255) a = 255;
			fdst[x+y*stash->params.width] = a;
		}
	}*/

	// Blur
	if( iblur > 0 )
	{
		scratch.clear();
		unsigned char* bdst = &texData[ glyph->x0 + glyph->y0 * params.width ];
		fons__blur( bdst, gw, gh, params.width, iblur );
	}

	dirtyRect[ 0 ] = fons__mini( dirtyRect[ 0 ], glyph->x0 );
	dirtyRect[ 1 ] = fons__mini( dirtyRect[ 1 ], glyph->y0 );
	dirtyRect[ 2 ] = fons__maxi( dirtyRect[ 2 ], glyph->x1 );
	dirtyRect[ 3 ] = fons__maxi( dirtyRect[ 3 ], glyph->y1 );

	return glyph;
}

void FONScontext::getQuad( FONSfont& font, int prevGlyphIndex, FONSglyph* glyph,
	float scale, float spacing, float* x, float* y, FONSquad* q )
{
	float rx, ry, xoff, yoff, x0, y0, x1, y1;

	if( prevGlyphIndex != -1 )
	{
		float adv = font.getGlyphKernAdvance( prevGlyphIndex, glyph->index ) * scale;
		*x += (int)( adv + spacing + 0.5f );
	}

	// Each glyph has 2px border to allow good interpolation,
	// one pixel to prevent leaking, and one to allow good interpolation for rendering.
	// Inset the texture region by one pixel for correct interpolation.
	xoff = (short)( glyph->xoff + 1 );
	yoff = (short)( glyph->yoff + 1 );
	x0 = (float)( glyph->x0 + 1 );
	y0 = (float)( glyph->y0 + 1 );
	x1 = (float)( glyph->x1 - 1 );
	y1 = (float)( glyph->y1 - 1 );

	if( params.flags & FONS_ZERO_TOPLEFT )
	{
		rx = (float)(int)( *x + xoff );
		ry = (float)(int)( *y + yoff );
		q->y1 = ry + y1 - y0;
	}
	else
	{
		rx = (float)(int)( *x + xoff );
		ry = (float)(int)( *y - yoff );
		q->y1 = ry - y1 + y0;
	}
	q->x0 = rx;
	q->y0 = ry;
	q->x1 = rx + x1 - x0;

	q->s0 = x0 * itw;
	q->t0 = y0 * ith;
	q->s1 = x1 * itw;
	q->t1 = y1 * ith;

	*x += (int)( glyph->xadv / 10.0f + 0.5f );
}

void FONScontext::flush()
{
	// Flush texture
	if( dirtyRect[ 0 ] < dirtyRect[ 2 ] && dirtyRect[ 1 ] < dirtyRect[ 3 ] )
	{
		if( params.renderUpdate != NULL )
			params.renderUpdate( params.userPtr, dirtyRect, texData.data() );
		// Reset dirty rect
		dirtyRect[ 0 ] = params.width;
		dirtyRect[ 1 ] = params.height;
		dirtyRect[ 2 ] = 0;
		dirtyRect[ 3 ] = 0;
	}

	// Flush triangles
	if( nverts > 0 )
	{
		if( params.renderDraw != NULL )
			params.renderDraw( params.userPtr, verts, tcoords, colors, nverts );
		nverts = 0;
	}
}

void FONScontext::vertex( float x, float y, float s, float t, unsigned int c )
{
	verts[ nverts * 2 + 0 ] = x;
	verts[ nverts * 2 + 1 ] = y;
	tcoords[ nverts * 2 + 0 ] = s;
	tcoords[ nverts * 2 + 1 ] = t;
	colors[ nverts ] = c;
	nverts++;
}