#include "Context.h"

int FontStash2::Context::debugDumpAtlas( const char* path ) const
{
	return ramTexture.save( params.width, params.height, path );
}