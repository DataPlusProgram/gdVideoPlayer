#include "simplePool.h"
#include <iostream>
PoolEntry* SimplePool::fetch(int dataSize)
{
	for (auto i=0; i < pool.size(); i++)
	{
		if (pool[i]->free == true)
		{	
			

			pool[i]->free = false;
			return pool[i];
		}
	}

	//nothing free in pool so create new entry

	PoolEntry* newEntry = new PoolEntry();

	godot::PoolByteArray ret;
	//int size = width * height * 4;
	ret.resize(dataSize);
	
	
	newEntry->free = false;
	newEntry->id = pool.size();
	newEntry->data = ret;
	//newEntry.ptr = newEntry.write->ptr();
	pool.push_back(newEntry);

	return newEntry;
}

void SimplePool::free(int id)
{
	pool[id]->free = true;
}

SimplePool::SimplePool(godot::Vector2 dimensions)
{
	this->dimensions = dimensions;
}
