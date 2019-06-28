#include <stdio.h>
#include <vector>
#include "truevision.h"

namespace Truevision
{
	enum struct eColorMapType : uint8_t
	{
		None = 0,
		Present = 1,
	};

	enum struct eImageType : uint8_t
	{
		None = 0,
		Palette = 1,
		RGB = 2,
		Grayscale = 3,
	};

#pragma pack( 1 )
	struct TgaHeader
	{
		uint8_t IDLength = 0;
		eColorMapType ColorMapType = eColorMapType::None;
		eImageType ImageType;
		uint16_t CMapStart = 0;
		uint16_t CMapLength = 0;
		uint8_t CMapDepth = 0;
		uint16_t XOffset = 0, YOffset = 0;
		uint16_t Width, Height;
		uint8_t PixelDepth;
		// By default, rows order in TGAs is bottom to top. Not what we want. That's why setting bit #5 of the ImageDescriptor header field.
		uint8_t ImageDescriptor = 0b00100000;
	};

	// A simple wrapper around FILE* handle from <stdio.h>
	// Too bad C++ standard library doesn't do a good job about I/O.
	class WriteFileHandle
	{
		FILE* const m_handle;

	public:

		WriteFileHandle( const char* path ) :
			m_handle( fopen( path, "wb" ) )
		{ }

		~WriteFileHandle()
		{
			if( nullptr != m_handle )
				fclose( m_handle );
		}

		operator bool() const
		{
			return nullptr != m_handle;
		}

		bool write( const void* pv, size_t cb )
		{
			if( nullptr == m_handle )
				return false;
			const size_t written = fwrite( pv, 1, cb, m_handle );
			return written == cb;
		}

		template<class E>
		bool write( const E& e )
		{
			return write( &e, sizeof( E ) );
		}
	};

	bool saveGrayscale( const uint8_t* source, int w, int h, const char* path )
	{
		TgaHeader header;
		header.ImageType = eImageType::Grayscale;
		header.Width = (uint16_t)w;
		header.Height = (uint16_t)h;
		header.PixelDepth = 8;

		WriteFileHandle file{ path };
		if( !file )
			return false;
		if( !file.write( header ) )
			return false;
		if( !file.write( source, w*h ) )
			return false;
		return true;
	}

	// Pack 32-bit RGBA image into 24 bit, discarding the alpha.
	static void pack24( const uint32_t* source, uint8_t *dest, size_t count )
	{
		for( size_t i = 0; i < count; i++ )
		{
			const uint32_t src = *source;
			dest[ 0 ] = (uint8_t)src;
			dest[ 1 ] = (uint8_t)( src >> 8 );
			dest[ 2 ] = (uint8_t)( src >> 16 );

			source++;
			dest += 3;
		}
	}

	bool saveColor( const uint32_t* source, int w, int h, const char* path )
	{
		std::vector<uint8_t> rgb24;
		try
		{
			rgb24.resize( w * h * 3 );
		}
		catch( std::exception& )
		{
			return false;
		}
		pack24( source, rgb24.data(), w * h );

		TgaHeader header;
		header.ImageType = eImageType::RGB;
		header.Width = (uint16_t)w;
		header.Height = (uint16_t)h;
		header.PixelDepth = 24;

		WriteFileHandle file{ path };
		if( !file )
			return false;
		if( !file.write( header ) )
			return false;
		if( !file.write( rgb24.data(), w * h * 3 ) )
			return false;
		return true;
	}
}