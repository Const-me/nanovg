#include "Atlas.h"
#include <algorithm>
#include <limits.h>
using namespace FontStash2;

Atlas::Atlas( int w, int h, int nnodes )
{
	width = w;
	height = h;

	// Allocate space for skyline nodes
	nodes.reserve( nnodes );

	// Init root node
	nodes.emplace_back( Node{ 0, 0, w } );
}

void Atlas::reset( int w, int h )
{
	width = w;
	height = h;
	nodes.clear();

	// Init root node
	nodes.emplace_back( Node{ 0, 0, w } );
}

int Atlas::rectFits( int i, int w, int h ) const
{
	// Checks if there is enough space at the location of skyline span 'i',
	// and return the max height of all skyline spans under that at that location,
	// (think tetris block being dropped at that position). Or -1 if no space found.
	int x = nodes[ i ].x;
	int y = nodes[ i ].y;
	int spaceLeft;
	if( x + w > width )
		return -1;
	spaceLeft = w;
	while( spaceLeft > 0 )
	{
		if( i == (int)nodes.size() )
			return -1;
		y = std::max( y, (int)nodes[ i ].y );
		if( y + h > height )
			return -1;
		spaceLeft -= nodes[ i ].width;
		++i;
	}
	return y;
}

void Atlas::insertNode( int idx, int x, int y, int w )
{
	nodes.insert( nodes.begin() + idx, std::move( Node{ x, y, w } ) );
}

void Atlas::removeNode( int idx )
{
	nodes.erase( nodes.begin() + idx );
}

void Atlas::addSkylineLevel( int idx, int x, int y, int w, int h )
{
	int i;

	// Insert new node
	insertNode( idx, x, y + h, w );

	// Delete skyline segments that fall under the shadow of the new segment.
	for( i = idx + 1; i < (int)nodes.size(); i++ )
	{
		if( nodes[ i ].x < nodes[ i - 1 ].x + nodes[ i - 1 ].width )
		{
			int shrink = nodes[ i - 1 ].x + nodes[ i - 1 ].width - nodes[ i ].x;
			nodes[ i ].x += (short)shrink;
			nodes[ i ].width -= (short)shrink;
			if( nodes[ i ].width <= 0 )
			{
				removeNode( i );
				i--;
			}
			else
				break;
		}
		else
			break;
	}

	// Merge same height skyline segments that are next to each other.
	for( i = 0; i < (int)nodes.size() - 1; i++ )
	{
		if( nodes[ i ].y == nodes[ i + 1 ].y )
		{
			nodes[ i ].width += nodes[ i + 1 ].width;
			removeNode( i + 1 );
			i--;
		}
	}
}

bool Atlas::addRect( int rw, int rh, int* rx, int* ry )
{
	int besth = height, bestw = width, besti = -1;
	int bestx = -1, besty = -1, i;

	// Bottom left fit heuristic.
	const int nnodes = (int)nodes.size();
	for( i = 0; i < nnodes; i++ )
	{
		int y = rectFits( i, rw, rh );
		if( y != -1 )
		{
			if( y + rh < besth || ( y + rh == besth && nodes[ i ].width < bestw ) )
			{
				besti = i;
				bestw = nodes[ i ].width;
				besth = y + rh;
				bestx = nodes[ i ].x;
				besty = y;
			}
		}
	}

	if( besti == -1 )
		return false;

	// Perform the actual packing.
	addSkylineLevel( besti, bestx, besty, rw, rh );
	*rx = bestx;
	*ry = besty;
	return true;
}

void Atlas::expand( int w, int h )
{
	// Insert node for empty space
	if( w > width )
		insertNode( (int)nodes.size(), width, 0, w - width );
	width = w;
	height = h;
}

int Atlas::getMaxY() const
{
	int i = INT_MIN;
	for( auto& n : nodes )
		i = std::max( i, (int)n.y );
	return i;
}