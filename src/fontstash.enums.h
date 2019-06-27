#pragma once

enum FONSflags
{
	FONS_ZERO_TOPLEFT = 1,
	FONS_ZERO_BOTTOMLEFT = 2,
};

enum FONSglyphBitmap
{
	FONS_GLYPH_BITMAP_OPTIONAL = 1,
	FONS_GLYPH_BITMAP_REQUIRED = 2,
};

enum FONSerrorCode
{
	// Font atlas is full.
	FONS_ATLAS_FULL = 1,
	// Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
	FONS_SCRATCH_FULL = 2,
	// Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
	FONS_STATES_OVERFLOW = 3,
	// Trying to pop too many states fonsPopState().
	FONS_STATES_UNDERFLOW = 4,
};

enum FONSalign
{
	// Horizontal align
	FONS_ALIGN_LEFT = 1 << 0,	// Default
	FONS_ALIGN_CENTER = 1 << 1,
	FONS_ALIGN_RIGHT = 1 << 2,
	// Vertical align
	FONS_ALIGN_TOP = 1 << 3,
	FONS_ALIGN_MIDDLE = 1 << 4,
	FONS_ALIGN_BOTTOM = 1 << 5,
	FONS_ALIGN_BASELINE = 1 << 6, // Default
};