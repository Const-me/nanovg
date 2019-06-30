#include <algorithm>
#include <string.h>
#include "Context.h"
#include "logger.h"
using namespace FontStash2;

Context::Context( FONSparams* p ) :
	params( *p ),
	itw( 1.0f / (float)params.width ),
	ith( 1.0f / (float)params.height ),
	atlas( params.width, params.height, FONS_INIT_ATLAS_NODES )
{
	memset( states, 0, sizeof( states ) );
}

bool Context::initStuff()
{
	if( !FontStash2::freetypeInit() )
		return false;

	// Allocate space for fonts
	fonts.reserve( FONS_INIT_FONTS );

	// Create texture for the cache
	ramTexture.resize( params.width, params.height );

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

void Context::addWhiteRect( int w, int h )
{
	int gx, gy;
	if( !atlas.addRect( w, h, &gx, &gy ) )
		return;

	// Rasterize
	ramTexture.addWhiteRect( params.width, gx, gy, w, h );

	dirtyRect[ 0 ] = std::min( dirtyRect[ 0 ], gx );
	dirtyRect[ 1 ] = std::min( dirtyRect[ 1 ], gy );
	dirtyRect[ 2 ] = std::max( dirtyRect[ 2 ], gx + w );
	dirtyRect[ 3 ] = std::max( dirtyRect[ 3 ], gy + h );
}


void Context::pushState()
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

void Context::popState()
{
	if( nstates <= 1 )
	{
		if( handleError )
			handleError( errorUptr, FONS_STATES_UNDERFLOW, 0 );
		return;
	}
	nstates--;
}

void Context::clearState()
{
	FONSstate* state = getState();
	state->size = 12.0f;
	state->color = 0xffffffff;
	state->font = 0;
	state->blur = 0;
	state->spacing = 0;
	state->align = FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE;
}

FONSstate* Context::getState()
{
	return &states[ nstates - 1 ];
}

int Context::addFont( const char* name, std::vector<uint8_t>& data )
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
	catch( const std::exception& )
	{
		return FONS_INVALID;
	}
}

GlyphValue* Context::getGlyph( FONSfont& font, unsigned int codepoint, short isize, short iblur, int bitmapOption )
{
#ifdef NANOVG_CLEARTYPE
	if( iblur != 0 )
	{
		iblur = 0;	// It's technically possible to implement clear type-aware blur, I just don't need it.
		static bool warned = false;
		if( !warned )
		{
			warned = false;
			logWarning( "ClearType build is currently incompatible with blurred fonts. Disabled the blur." );
		}
	}
	
#endif
	const float size = isize / 10.0f;
	FONSfont* renderFont = &font;

	if( isize < 2 )
		return NULL;
	if( iblur > 20 ) iblur = 20;
	const int pad = iblur + 2;

	// Reset allocator.
	scratch.clear();

	// Find code point and size.
	GlyphValue* glyph = font.lookupGlyph( codepoint, isize, iblur );
	if( nullptr != glyph )
		if( bitmapOption == FONS_GLYPH_BITMAP_OPTIONAL || glyph->hasBitmap() )
			return glyph;

	// Create a new glyph or rasterize bitmap data for a cached glyph.
	uint32_t g = font.getGlyphIndex( codepoint );
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
	renderFont->buildGlyphBitmap( g, size, &advance, &lsb, &x0, &y0, &x1, &y1 );
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
	ramTexture.addGlyph( font, params.width, glyph, pad );

#ifndef NANOVG_CLEARTYPE
	// Blur
	if( iblur > 0 )
	{
		scratch.clear();
		ramTexture.blurRectangle( params.width, glyph->x0, glyph->y0, gw, gh, iblur );
	}
#endif

	dirtyRect[ 0 ] = std::min( dirtyRect[ 0 ], (int)glyph->x0 );
	dirtyRect[ 1 ] = std::min( dirtyRect[ 1 ], (int)glyph->y0 );
	dirtyRect[ 2 ] = std::max( dirtyRect[ 2 ], (int)glyph->x1 );
	dirtyRect[ 3 ] = std::max( dirtyRect[ 3 ], (int)glyph->y1 );

	return glyph;
}

void Context::getQuad( FONSfont& font, int prevGlyphIndex, GlyphValue* glyph,
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

void Context::flush()
{
	// Flush texture
	if( dirtyRect[ 0 ] < dirtyRect[ 2 ] && dirtyRect[ 1 ] < dirtyRect[ 3 ] )
	{
		if( params.renderUpdate != NULL )
			params.renderUpdate( params.userPtr, dirtyRect, (const uint8_t*)ramTexture.data() );
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

void Context::vertex( float x, float y, float s, float t, unsigned int c )
{
	verts[ nverts * 2 + 0 ] = x;
	verts[ nverts * 2 + 1 ] = y;
	tcoords[ nverts * 2 + 0 ] = s;
	tcoords[ nverts * 2 + 1 ] = t;
	colors[ nverts ] = c;
	nverts++;
}