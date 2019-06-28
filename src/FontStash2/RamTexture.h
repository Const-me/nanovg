#pragma once
#include <vector>
#include <algorithm>

namespace FontStash2
{
	class Font;
	struct GlyphValue;

	template<class T>
	class RamTexture
	{
		std::vector<T> texture;

	public:

		const T* data() const
		{
			return texture.data();
		}

		bool resize( int width, int height )
		{
			try
			{
				texture.resize( width * height );
			}
			catch( std::exception& )
			{
				return false;
			}
			std::fill( texture.begin(), texture.end(), 0 );
			return true;
		}

		bool expand( int oldWidth, int oldHeight, int width, int height )
		{
			std::vector<T> data;
			try
			{
				data.resize( width * height );
			}
			catch( std::exception& )
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

		void addWhiteRect( int width, int gx, int gy, int w, int h )
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

		bool addGlyph( Font& font, int textureWidth, const GlyphValue* glyph, int pad );
		bool addCleartypeGlyph( Font& font, float size, int textureWidth, const GlyphValue* glyph, int pad );

		void blurRectangle( int textureWidth, int x, int y, int w, int h, short iblur );
	};
}