#include "RamTexture.h"
#include "Font.h"
#include "blur.h"
#include "truevision.h"

namespace FontStash2
{
	template<class T>
	bool RamTexture<T>::resize( int width, int height )
	{
		try
		{
			texture.resize( width * height );
		}
		catch( const std::exception& )
		{
			return false;
		}
		std::fill( texture.begin(), texture.end(), 0 );
		return true;
	}

	template<class T>
	bool RamTexture<T>::expand( int oldWidth, int oldHeight, int width, int height )
	{
		std::vector<T> data;
		try
		{
			data.resize( width * height );
		}
		catch( const std::exception& )
		{
			return false;
		}

		for( int i = 0; i < oldHeight; i++ )
		{
			T* dst = &data[ i * width ];
			const T* src = &texture[ i * oldWidth ];
			std::copy_n( src, oldWidth, dst );
			if( width > oldWidth )
				std::fill_n( dst + oldWidth, width - oldWidth, 0 );
		}
		if( height > oldHeight )
			std::fill_n( &data[ oldHeight * width ], ( height - oldHeight ) * width, 0 );

		texture.swap( data );
		return true;
	}

	template<class T>
	void RamTexture<T>::addWhiteRect( int width, int gx, int gy, int w, int h )
	{
		constexpr T white = ~( (T)0 );
		T *dst = &texture[ gx + gy * width ];
		for( int y = 0; y < h; y++ )
		{
			for( int x = 0; x < w; x++ )
				dst[ x ] = white;
			dst += width;
		}
	}

	template<class T>
	bool RamTexture<T>::addGlyph( Font& font, int textureWidth, const GlyphValue* glyph, int pad )
	{
		const int x = glyph->x0;
		const int y = glyph->y0;
		const int w = glyph->x1 - x;
		const int h = glyph->y1 - y;
		T* dst = &texture[ x + pad + ( y + pad )* textureWidth ];
		font.renderGlyphBitmap( dst, w - pad * 2, h - pad * 2, textureWidth );

		// Make sure there is one pixel empty border.
		dst = &texture[ x + y * textureWidth ];
		for( int y = 0; y < h; y++ )
		{
			dst[ y * textureWidth ] = 0;
			dst[ w - 1 + y * textureWidth ] = 0;
		}
		for( int x = 0; x < w; x++ )
		{
			dst[ x ] = 0;
			dst[ x + ( h - 1 ) * textureWidth ] = 0;
		}

		// Debug code to color the glyph background
		/*	unsigned char* fdst = &stash->texData[glyph->x0 + glyph->y0 * stash->params.width];
		for (y = 0; y < gh; y++) {
			for (x = 0; x < gw; x++) {
				int a = (int)fdst[x+y*stash->params.width] + 20;
				if (a > 255) a = 255;
				fdst[x+y*stash->params.width] = a;
			}
		} */
		return true;
	}

#ifdef NANOVG_CLEARTYPE
	template<>
	bool RamTexture<uint32_t>::save( int w, int h, const char* path ) const
	{
		return Truevision::saveColor( texture.data(), w, h, path );
	}

	template class RamTexture<uint32_t>;
#else
	template<>
	void RamTexture<uint8_t>::blurRectangle( int textureWidth, int x, int y, int w, int h, short iblur )
	{
		unsigned char* bdst = &texture[ x + y * textureWidth ];
		FontStash2::blur( bdst, w, h, textureWidth, iblur );
	}

	template<>
	bool RamTexture<uint8_t>::save( int w, int h, const char* path ) const
	{
		return Truevision::saveGrayscale( texture.data(), w, h, path );
	}

	template class RamTexture<uint8_t>;
#endif
}