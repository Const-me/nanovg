#include "Context.h"
#include "truevision.h"

int FontStash2::Context::debugDumpAtlas( const char* grayscale, const char* cleartype ) const
{
	if( !Truevision::saveGrayscale( texData.data(), params.width, params.height, grayscale ) )
		return 0;
	if( !Truevision::saveColor( cleartypeTexture.data(), params.width, params.height, cleartype ) )
		return 0;
	return 1;
}