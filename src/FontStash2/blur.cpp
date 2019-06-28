#include "blur.h"
#include <math.h>

// Based on Exponential blur, Jani Huhtanen, 2006
#define APREC 16
#define ZPREC 7

static void fons__blurCols( unsigned char* dst, int w, int h, int dstStride, int alpha )
{
	int x, y;
	for( y = 0; y < h; y++ ) {
		int z = 0; // force zero border
		for( x = 1; x < w; x++ ) {
			z += ( alpha * ( ( (int)( dst[ x ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ x ] = (unsigned char)( z >> ZPREC );
		}
		dst[ w - 1 ] = 0; // force zero border
		z = 0;
		for( x = w - 2; x >= 0; x-- ) {
			z += ( alpha * ( ( (int)( dst[ x ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ x ] = (unsigned char)( z >> ZPREC );
		}
		dst[ 0 ] = 0; // force zero border
		dst += dstStride;
	}
}

static void fons__blurRows( unsigned char* dst, int w, int h, int dstStride, int alpha )
{
	int x, y;
	for( x = 0; x < w; x++ ) {
		int z = 0; // force zero border
		for( y = dstStride; y < h*dstStride; y += dstStride ) {
			z += ( alpha * ( ( (int)( dst[ y ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ y ] = (unsigned char)( z >> ZPREC );
		}
		dst[ ( h - 1 )*dstStride ] = 0; // force zero border
		z = 0;
		for( y = ( h - 2 )*dstStride; y >= 0; y -= dstStride ) {
			z += ( alpha * ( ( (int)( dst[ y ] ) << ZPREC ) - z ) ) >> APREC;
			dst[ y ] = (unsigned char)( z >> ZPREC );
		}
		dst[ 0 ] = 0; // force zero border
		dst++;
	}
}

void FontStash2::blur( unsigned char* dst, int w, int h, int dstStride, int blur )
{
	int alpha;
	float sigma;

	if( blur < 1 )
		return;
	// Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
	sigma = (float)blur * 0.57735f; // 1 / sqrt(3)
	alpha = (int)( ( 1 << APREC ) * ( 1.0f - expf( -2.3f / ( sigma + 1.0f ) ) ) );
	fons__blurRows( dst, w, h, dstStride, alpha );
	fons__blurCols( dst, w, h, dstStride, alpha );
	fons__blurRows( dst, w, h, dstStride, alpha );
	fons__blurCols( dst, w, h, dstStride, alpha );
	//	fons__blurrows( dst, w, h, dstStride, alpha );
	//	fons__blurcols( dst, w, h, dstStride, alpha );
}