int fons__tt_init( FONScontext *context )
{
	FT_Error ftError;
	FONS_NOTUSED( context );
	ftError = FT_Init_FreeType( &ftLibrary );
	return ftError == 0;
}

int fons__tt_done( FONScontext *context )
{
	FT_Error ftError;
	FONS_NOTUSED( context );
	ftError = FT_Done_FreeType( ftLibrary );
	return ftError == 0;
}

int fons__tt_loadFont( FONScontext *context, FONSttFontImpl *font, unsigned char *data, int dataSize )
{
	FT_Error ftError;
	FONS_NOTUSED( context );

	//font->font.userdata = stash;
	ftError = FT_New_Memory_Face( ftLibrary, (const FT_Byte*)data, dataSize, 0, &font->font );
	return ftError == 0;
}

void fons__tt_getFontVMetrics( FONSttFontImpl *font, int *ascent, int *descent, int *lineGap )
{
	*ascent = font->font->ascender;
	*descent = font->font->descender;
	*lineGap = font->font->height - ( *ascent - *descent );
}

float fons__tt_getPixelHeightScale( FONSttFontImpl *font, float size )
{
	return size / ( font->font->ascender - font->font->descender );
}

int fons__tt_getGlyphIndex( FONSttFontImpl *font, int codepoint )
{
	return FT_Get_Char_Index( font->font, codepoint );
}

int fons__tt_buildGlyphBitmap( FONSttFontImpl *font, int glyph, float size, float scale,
	int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1 )
{
	FT_Error ftError;
	FT_GlyphSlot ftGlyph;
	FT_Fixed advFixed;
	FONS_NOTUSED( scale );

	ftError = FT_Set_Pixel_Sizes( font->font, 0, (FT_UInt)( size * (float)font->font->units_per_EM / (float)( font->font->ascender - font->font->descender ) ) );
	if( ftError ) return 0;
	ftError = FT_Load_Glyph( font->font, glyph, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT );
	if( ftError ) return 0;
	ftError = FT_Get_Advance( font->font, glyph, FT_LOAD_NO_SCALE, &advFixed );
	if( ftError ) return 0;
	ftGlyph = font->font->glyph;
	*advance = (int)advFixed;
	*lsb = (int)ftGlyph->metrics.horiBearingX;
	*x0 = ftGlyph->bitmap_left;
	*x1 = *x0 + ftGlyph->bitmap.width;
	*y0 = -ftGlyph->bitmap_top;
	*y1 = *y0 + ftGlyph->bitmap.rows;
	return 1;
}

void fons__tt_renderGlyphBitmap( FONSttFontImpl *font, unsigned char *output, int outWidth, int outHeight, int outStride,
	float scaleX, float scaleY, int glyph )
{
	FT_GlyphSlot ftGlyph = font->font->glyph;
	int ftGlyphOffset = 0;
	int x, y;
	FONS_NOTUSED( outWidth );
	FONS_NOTUSED( outHeight );
	FONS_NOTUSED( scaleX );
	FONS_NOTUSED( scaleY );
	FONS_NOTUSED( glyph );	// glyph has already been loaded by fons__tt_buildGlyphBitmap

	for( y = 0; y < ftGlyph->bitmap.rows; y++ ) {
		for( x = 0; x < ftGlyph->bitmap.width; x++ ) {
			output[ ( y * outStride ) + x ] = ftGlyph->bitmap.buffer[ ftGlyphOffset++ ];
		}
	}
}

int fons__tt_getGlyphKernAdvance( FONSttFontImpl *font, int glyph1, int glyph2 )
{
	FT_Vector ftKerning;
	FT_Get_Kerning( font->font, glyph1, glyph2, FT_KERNING_DEFAULT, &ftKerning );
	return (int)( ( ftKerning.x + 32 ) >> 6 );  // Round up and convert to integer
}