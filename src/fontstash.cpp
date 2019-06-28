#include <memory>
#include "fontstash.h"
#include "FontStash2/Context.h"
#include "FontStash2/utf8.h"
using FontStash2::FONSstate;
using FontStash2::GlyphValue;

#define FONS_NOTUSED(v)  (void)sizeof(v)

// ===== Constructor and destructor =====
FONScontext* fonsCreateInternal( FONSparams* params )
{
	try
	{
		auto up = std::make_unique<FONScontext>( params );
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
	if( nullptr == stash )
		return;
	*width = stash->params.width;
	*height = stash->params.height;
}

int fonsExpandAtlas( FONScontext* stash, int width, int height )
{
	if( nullptr == stash )
		return 0;

	width = std::max( width, stash->params.width );
	height = std::max( height, stash->params.height );

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
	if( nullptr == stash )
		return 0;

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
	if( nullptr == stash )
		return FONS_INVALID;

	std::vector<uint8_t> dataVector{ data, data + dataSize };
	if( freeData )
		free( data );

	return stash->addFont( name, dataVector );
}

int fonsGetFontByName( FONScontext* s, const char* name )
{
	if( nullptr == s )
		return FONS_INVALID;

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
	if( nullptr == stash )
		return;
	stash->getState()->size = size;
}

void fonsSetColor( FONScontext* stash, unsigned int color )
{
	if( nullptr == stash )
		return;
	stash->getState()->color = color;
}

void fonsSetSpacing( FONScontext* stash, float spacing )
{
	if( nullptr == stash )
		return;
	stash->getState()->spacing = spacing;
}

void fonsSetBlur( FONScontext* stash, float blur )
{
	if( nullptr == stash )
		return;
	stash->getState()->blur = blur;
}

void fonsSetAlign( FONScontext* stash, int align )
{
	if( nullptr == stash )
		return;
	stash->getState()->align = align;
}

void fonsSetFont( FONScontext* stash, int font )
{
	if( nullptr == stash )
		return;
	stash->getState()->font = font;
}

// ===== Draw text =====
float fonsDrawText( FONScontext* stash, float x, float y, const char* str, const char* end )
{
	if( nullptr == stash )
		return 0;
	FONSstate* state = stash->getState();
	unsigned int codepoint;
	unsigned int utf8state = 0;
	GlyphValue* glyph = nullptr;
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
	if( nullptr == stash )
		return 0;

	FONSstate* state = stash->getState();
	unsigned int codepoint;
	unsigned int utf8state = 0;
	FONSquad q;
	GlyphValue* glyph = nullptr;
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
	if( nullptr == stash )
		return;
	FONSstate* const state = stash->getState();
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
	if( nullptr == stash )
		return;
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
	if( nullptr == stash )
		return 0;

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
	GlyphValue* glyph = nullptr;
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
		if( glyph != nullptr )
			stash->getQuad( *iter->font, iter->prevGlyphIndex, glyph, iter->scale, iter->spacing, &iter->nextx, &iter->nexty, quad );
		iter->prevGlyphIndex = glyph != nullptr ? glyph->index : -1;
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
	if( stash->dirtyRect[ 0 ] < stash->dirtyRect[ 2 ] && stash->dirtyRect[ 1 ] < stash->dirtyRect[ 3 ] )
	{
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