#pragma once
#include <vector>
#include <algorithm>

namespace FontStash2
{
	class Font;
	struct GlyphValue;

	// A 2D texture in system RAM
	template<class T>
	class RamTexture
	{
		std::vector<T> texture;

	public:

		const T* data() const
		{
			return texture.data();
		}

		bool resize( int width, int height );

		bool expand( int oldWidth, int oldHeight, int width, int height );

		void addWhiteRect( int width, int gx, int gy, int w, int h );

		bool addGlyph( Font& font, int textureWidth, const GlyphValue* glyph, int pad );

		void blurRectangle( int textureWidth, int x, int y, int w, int h, short iblur );

		bool save( int w, int h, const char* path ) const;
	};
}