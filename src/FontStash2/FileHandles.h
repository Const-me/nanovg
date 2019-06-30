#pragma once
#include <stdio.h>
#include <stdint.h>
#include <vector>

// A simple wrapper around FILE* handle from <stdio.h>
// Too bad C++ standard library doesn't do a good job about I/O.
class FileHandleBase
{
protected:
	FILE* const m_handle;

	FileHandleBase( FILE* h ) :
		m_handle( h ) { }

public:
	~FileHandleBase()
	{
		if( nullptr != m_handle )
			fclose( m_handle );
	}

	FileHandleBase( const FileHandleBase& ) = delete;
	FileHandleBase( FileHandleBase&& ) = delete;

	operator bool() const
	{
		return nullptr != m_handle;
	}
};

class ReadFileHandle : public FileHandleBase
{
public:
	ReadFileHandle( const char* path ) :
		FileHandleBase( fopen( path, "rb" ) )
	{ }

	~ReadFileHandle() = default;

	bool readAllBytes( std::vector<uint8_t> &data );
};

class WriteFileHandle : public FileHandleBase
{
public:
	WriteFileHandle( const char* path ) :
		FileHandleBase( fopen( path, "wb" ) )
	{ }

	~WriteFileHandle() = default;

	bool write( const void* pv, size_t cb );

	template<class E>
	bool writeVector( const std::vector<E>& vec )
	{
		return write( vec.data(), vec.size() * sizeof( E ) );
	}

	template<class E>
	bool writeStructure( const E& e )
	{
		return write( &e, sizeof( E ) );
	}
};