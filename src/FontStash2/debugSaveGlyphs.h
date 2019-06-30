#pragma once
#include <stdint.h>
typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;

namespace FontStash2
{
#ifdef _MSC_VER
	bool debugSaveGlyph( const FT_GlyphSlot data, uint32_t index, float size, const char* suffix );
#else
	inline bool debugSaveGlyph( const FT_GlyphSlot data, uint32_t index, float size, const char* suffix ) { }
#endif
}