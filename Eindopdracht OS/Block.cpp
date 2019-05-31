#include "Block.h"

Block::Block()
{
	orderNr = 0;
	for (int i = 0; i < 1024; i++) sample[i] = 0;
}
