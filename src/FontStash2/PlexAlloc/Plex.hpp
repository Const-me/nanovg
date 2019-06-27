// Copyright (c) 2017 Konstantin, http://const.me
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files( the "Software" ), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once
#include <array>
#include <type_traits>
#include "MemAlloc.hpp"

namespace PlexAlloc
{
	// This class allocates RAM in continuous batches, similar to CAtlPlex.
	// It only frees RAM when destroyed.
	// It doesn't call neither constructors nor destructors, i.e. allocate() returns uninitialized blocks of RAM.
	template <typename T, size_t nBlockSize = 10, size_t align = alignof ( T )>
	class Plex
	{
		struct CMemChunk
		{
			std::array<typename std::aligned_storage<sizeof( T ), align>::type, nBlockSize> items;
			CMemChunk* pNext;
		};

		CMemChunk* m_pChunk; // Most recently allocated a.k.a. the current chunk
		size_t m_usedInChunk; // Count of allocated items in the current chunk

		static CMemChunk* allocateChunk()
		{
			return (CMemChunk*)alignedMalloc( sizeof( CMemChunk ), align );
		}
		static void freeChunk( CMemChunk* p )
		{
			alignedFree( p );
		}

		// Reset the internal state, but don't free any memory.
		void reset() noexcept
		{
			m_pChunk = nullptr;
			m_usedInChunk = nBlockSize;
		}

		// Free all memory
		void clear()
		{
			while( nullptr != m_pChunk )
			{
				CMemChunk* const pNext = m_pChunk->pNext;
				freeChunk( m_pChunk );
				m_pChunk = pNext;
			}
			reset();
		}

	public:
		Plex() { reset(); }
		~Plex() { clear(); }
		Plex( const Plex & that ) = delete;
		Plex& operator=( const Plex & that ) = delete;
		Plex( Plex && that )
		{
			m_pChunk = that.m_pChunk;
			m_usedInChunk = that.m_usedInChunk;
			that.reset();
		}
		Plex& operator= ( Plex && that )
		{
			m_pChunk = that.m_pChunk;
			m_usedInChunk = that.m_usedInChunk;
			that.reset();
			return *this;
		}

		// Allocate a new item
		T* allocate()
		{
			const size_t nNewCount = m_usedInChunk + 1;
			if( nNewCount <= nBlockSize )
			{
				// Still have free space in the current chunk
				T* const result = reinterpret_cast<T*>( &m_pChunk->items[ m_usedInChunk ] );
				m_usedInChunk = nNewCount;
				return result;
			}

			// Allocate & initialize new chunk
			CMemChunk* const pNewChunk = allocateChunk();
			if( nullptr == pNewChunk )
				throw std::bad_alloc();

			pNewChunk->pNext = m_pChunk;
			m_pChunk = pNewChunk;
			m_usedInChunk = 1;
			return reinterpret_cast<T*>( pNewChunk->items.data() );
		}
	};
}