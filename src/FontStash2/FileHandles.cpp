#include "FileHandles.h"

bool ReadFileHandle::readAllBytes( std::vector<uint8_t> &data )
{
	data.clear();
	if( nullptr == m_handle )
		return false;

	fseek( m_handle, 0, SEEK_END );
	const size_t dataSize = (size_t)ftell( m_handle );
	fseek( m_handle, 0, SEEK_SET );

	try
	{
		data.resize( dataSize );
	}
	catch( std::exception& )
	{
		return false;
	}

	const size_t readed = fread( data.data(), 1, dataSize, m_handle );
	return readed == dataSize;
}

bool WriteFileHandle::write( const void* pv, size_t cb )
{
	if( nullptr == m_handle )
		return false;
	const size_t written = fwrite( pv, 1, cb, m_handle );
	return written == cb;
}