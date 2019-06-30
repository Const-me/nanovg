//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#pragma once
#include <stdint.h>
#include "fontstash.enums.h"

namespace FontStash2
{
	class Context;
	class Font;
}
using FONScontext = FontStash2::Context;
using FONSfont = FontStash2::Font;

#define FONS_INVALID -1

struct FONSparams
{
	int width, height;
	unsigned char flags;
	void* userPtr;
	int( *renderCreate )( void* uptr, int width, int height );
	int( *renderResize )( void* uptr, int width, int height );
	void( *renderUpdate )( void* uptr, int* rect, const unsigned char* data );
	void( *renderDraw )( void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts );
	void( *renderDelete )( void* uptr );
};

struct FONSquad
{
	float x0, y0, s0, t0;
	float x1, y1, s1, t1;
};

struct FONStextIter
{
	float x, y, nextx, nexty, scale, spacing;
	unsigned int codepoint;
	short isize, iblur;
	FONSfont* font;
	int prevGlyphIndex;
	const char* str;
	const char* next;
	const char* end;
	unsigned int utf8state;
	int bitmapOption;
};

// Constructor and destructor
FONScontext* fonsCreateInternal( FONSparams* params );
void fonsDeleteInternal( FONScontext* s );

void fonsSetErrorCallback( FONScontext* s, void( *callback )( void* uptr, int error, int val ), void* uptr );
// Returns current atlas size
void fonsGetAtlasSize( FONScontext* s, int* width, int* height );
// Expands the atlas size
int fonsExpandAtlas( FONScontext* s, int width, int height );
// Resets the whole stash
int fonsResetAtlas( FONScontext* stash, int width, int height );

// Add fonts
int fonsAddFont( FONScontext* s, const char* name, const char* path );
int fonsAddFontMem( FONScontext* s, const char* name, unsigned char* data, int ndata, int freeData );
int fonsGetFontByName( FONScontext* s, const char* name );

// State handling
void fonsPushState( FONScontext* s );
void fonsPopState( FONScontext* s );
void fonsClearState( FONScontext* s );

// State setting
void fonsSetSize( FONScontext* s, float size );
void fonsSetColor( FONScontext* s, unsigned int color );
void fonsSetSpacing( FONScontext* s, float spacing );
void fonsSetBlur( FONScontext* s, float blur );
void fonsSetAlign( FONScontext* s, int align );
void fonsSetFont( FONScontext* s, int font );

// Draw text
float fonsDrawText( FONScontext* s, float x, float y, const char* string, const char* end );

// Measure text
float fonsTextBounds( FONScontext* s, float x, float y, const char* string, const char* end, float* bounds );
void fonsLineBounds( FONScontext* s, float y, float* miny, float* maxy );
void fonsVertMetrics( FONScontext* s, float* ascender, float* descender, float* lineh );

// Text iterator
int fonsTextIterInit( FONScontext* stash, FONStextIter* iter, float x, float y, const char* str, const char* end, int bitmapOption );
int fonsTextIterNext( FONScontext* stash, FONStextIter* iter, struct FONSquad* quad );

// Pull texture changes
#ifdef NANOVG_CLEARTYPE
const uint32_t* fonsGetTextureData( FONScontext* stash, int* width, int* height );
#else
const uint8_t* fonsGetTextureData( FONScontext* stash, int* width, int* height );
#endif
int fonsValidateTexture( FONScontext* s, int* dirty );

// Draws the stash texture for debugging
void fonsDrawDebug( FONScontext* s, float x, float y );

int fonsAddFallbackFont( FONScontext* stash, int base, int fallback );

int fonsDebugDumpAtlas( FONScontext* stash, const char* path );