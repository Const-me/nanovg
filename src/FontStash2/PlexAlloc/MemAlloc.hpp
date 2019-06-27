// Copyright (c) 2017 Konstantin, http://const.me
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files( the "Software" ), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

namespace PlexAlloc
{
	// Allocate aligned block of memory
	inline void* alignedMalloc( size_t size, size_t alignment )
	{
#ifdef _MSC_VER
		return _aligned_malloc( size, alignment );
#else
		return aligned_alloc( alignment, size );
#endif
	}

	// Free aligned block of memory
	inline void alignedFree( void* ptr )
	{
#ifdef _MSC_VER
		_aligned_free( ptr );
#else
		free( ptr );
#endif
	}
}