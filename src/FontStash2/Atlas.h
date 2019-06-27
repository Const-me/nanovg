#pragma once
#include <vector>

namespace FontStash2
{
	// Atlas based on Skyline Bin Packer by Jukka Jylänki
	class Atlas
	{
	public:

		struct Node
		{
			short x, y, width;
			Node( int _x, int _y, int _w ) :
				x( (short)_x ),
				y( (short)_y ),
				width( (short)_w )
			{ }
		};

		// fons__allocAtlas
		Atlas( int w, int h, int nnodes );

		bool addRect( int rw, int rh, int* rx, int* ry );

		void expand( int w, int h );

		void reset( int w, int h );

		int getMaxY() const;

		const std::vector<Node>& atlasNodes() const
		{
			return nodes;
		}
		
	private:
		int width, height;
		std::vector<Node> nodes;

		// fons__atlasRectFits
		int rectFits( int i, int w, int h ) const;

		void addSkylineLevel( int idx, int x, int y, int w, int h );

		void insertNode( int idx, int x, int y, int w );
		void removeNode( int idx );
	};
}