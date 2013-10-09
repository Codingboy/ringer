#include "ring.hpp"

#ifndef NDEBUG
#include <cassert>
#endif

Ring::Ring(const unsigned char* c, unsigned int length, const unsigned char* pos, unsigned int posLength, unsigned int mutationInterval)
{
#ifndef NDEBUG
	assert(length > 0);
	assert(length <= MAPSIZE);
	assert(posLength > 0);
	assert(posLength <= IVSIZE);
#ifndef MULTICORE
	assert(mutationInterval%IVSIZE == 0);
#endif
#endif
	this->posLength = posLength;
	for (unsigned int i=0; i<posLength; i++)
	{
		this->initPos[i] = pos[i];
	}
	this->mutationInterval = mutationInterval;
	for (unsigned int i=0; i<length; i++)
	{
		this->pw[i] = c[i];
	}
	this->pwLength = length;
	reinit();
}

Ring::~Ring()
{
}

void Ring::reinit()
{
	this->actualPos = 0;
	for (unsigned int i=0; i<MAPSIZE; i++)
	{
		this->map[i] = i;
	}
#ifndef MULTICORE
	this->operationsSinceMutation = 0;
#endif
	for (unsigned int i=0; i<posLength; i++)
	{
		this->pos[i] = this->initPos[i];
	}
	mutate();
}

void Ring::shuffle()
{
	for (unsigned int i=0; i<MAPSIZE; i++)
	{
		unsigned char index = this->map[i];
		unsigned char tmp = this->map[index];
		this->map[index] = this->map[i];
		this->map[i] = tmp;
	}
	for (int i=0; i<MAPSIZE; i++)
	{
		decodeMap[this->map[i]] = i;
	}
}

void Ring::mutate()
{
	unsigned int count = 0;
	int prev[MAPSIZE];
	int next[MAPSIZE];
	unsigned char mapCopy[MAPSIZE];
	for (int i=0; i<MAPSIZE; i++)
	{
		mapCopy[i] = this->map[i];
		next[i] = (i+1)%MAPSIZE;
		int index = i-1;
		if (index < 0)
		{
			index += MAPSIZE;
		}
		prev[i] = index;
	}
	int actual = 0;
	int unchanged = 0;
	unsigned int actualPWEntry = 0;
	while (count < MAPSIZE)
	{
		int actualPWValue = ((unsigned int)(pw[actualPWEntry])+1)%(MAPSIZE-count);
		while (unchanged < actualPWValue)
		{
			actual = next[actual];
#ifndef NDEBUG
			assert(actual != -1);
#endif
			unchanged++;
		}
		unchanged = 0;
		actualPWEntry = (actualPWEntry + 1)%pwLength;
		this->map[actual] = mapCopy[count];
		count++;
		int nextMapEntry = next[actual];
		int prevMapEntry = prev[actual];
#ifndef NDEBUG
		assert(nextMapEntry != -1);
		assert(prevMapEntry != -1);
#endif
		next[prevMapEntry] = nextMapEntry;
		prev[nextMapEntry] = prevMapEntry;
#ifndef NDEBUG
		next[actual] = -1;
		prev[actual] = -1;
#endif
		actual = nextMapEntry;
	}
	for (int i=0; i<MAPSIZE; i++)
	{
		decodeMap[this->map[i]] = i;
	}
}

unsigned char Ring::encode(unsigned char c, unsigned int posIndex)
{
#ifndef NDEBUG
	assert(c < MAPSIZE);
	assert(posIndex < IVSIZE);
#endif
#ifndef MULTICORE
	posIndex = this->actualPos;
	this->actualPos = (posIndex + 1)%this->posLength;
#endif
	unsigned int index = (c + this->pos[posIndex])%MAPSIZE;
	unsigned char ret = this->map[index];
	this->pos[posIndex] = ((this->pos[posIndex] + 1)^c)%MAPSIZE;
#ifndef MULTICORE
	if (this->mutationInterval != 0)
	{
		this->operationsSinceMutation++;
		if (this->operationsSinceMutation == this->mutationInterval)
		{
			shuffle();
			this->operationsSinceMutation = 0;
		}
	}
#endif
	return ret;
}

unsigned char Ring::decode(unsigned char c, unsigned int posIndex)
{
#ifndef NDEBUG
	assert(c < MAPSIZE);
	assert(posIndex < IVSIZE);
#endif
#ifndef MULTICORE
	posIndex = this->actualPos;
	this->actualPos = (posIndex + 1)%this->posLength;
#endif
	int ret = decodeMap[c] - this->pos[posIndex];
	if (ret < 0)
	{
		ret += MAPSIZE;
	}
	this->pos[posIndex] = ((this->pos[posIndex] + 1)^ret)%MAPSIZE;
#ifndef MULTICORE
	if (this->mutationInterval != 0)
	{
		this->operationsSinceMutation++;
		if (this->operationsSinceMutation == this->mutationInterval)
		{
			shuffle();
			this->operationsSinceMutation = 0;
		}
	}
#endif
	return ret;
}

void Ring::encode(unsigned char* c, unsigned int length, unsigned int posIndex)
{
#ifndef NDEBUG
	assert(length > 0);
#ifdef MULTICORE
	assert(posIndex < IVSIZE);
#endif
#endif
	for (unsigned int i=0; i<length; i++)
	{
#ifdef MULTICORE
		c[i] = encode(c[i], posIndex+i);
#else
		c[i] = encode(c[i], 0);
#endif
	}
}

void Ring::decode(unsigned char* c, unsigned int length, unsigned int posIndex)
{
#ifndef NDEBUG
	assert(length > 0);
#ifdef MULTICORE
	assert(posIndex < IVSIZE);
#endif
#endif
	for (unsigned int i=0; i<length; i++)
	{
#ifdef MULTICORE
		c[i] = decode(c[i], posIndex+i);
#else
		c[i] = decode(c[i], 0);
#endif
	}
}