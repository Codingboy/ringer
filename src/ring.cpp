#include "ring.hpp"

#ifndef NDEBUG
#include <cassert>
#endif

#include <cstdio>

Ring::Ring(const unsigned char* c, unsigned int length, const unsigned char* pos, unsigned int posLength, unsigned int mutationInterval)
{
#ifndef NDEBUG
	assert(length > 0);
	assert(length <= MAPSIZE);
	assert(posLength > 0);
	assert(posLength <= IVSIZE);
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
#ifdef KNOWNPLAINTEXTATTACK
for (unsigned int i=0; i<256; i++)
{
	mapSpecified[i] = true;
}
printf("map\n");
for (int i=0; i<8; i++)
{
	for (int j=0; j<4; j++)
	{
		for (int k=0; k<8; k++)
		{
			if (map[i*4*8*1+j*8+k*1] < 100)
			{
				printf("0");
				if (map[i*4*8*1+j*8+k*1] < 10)
				{
					printf("0");
				}
			}
			printf("%u ", map[i*4*8*1+j*8+k*1]);
		}
		printf("    ");
	}
	printf("\n");
}
printf("\n");
printf("decodemap\n");
for (int i=0; i<8; i++)
{
	for (int j=0; j<4; j++)
	{
		for (int k=0; k<8; k++)
		{
			if (decodeMap[i*4*8*1+j*8+k*1] < 100)
			{
				printf("0");
				if (decodeMap[i*4*8*1+j*8+k*1] < 10)
				{
					printf("0");
				}
			}
			printf("%u ", decodeMap[i*4*8*1+j*8+k*1]);
		}
		printf("    ");
	}
	printf("\n");
}
printf("\n");
printf("salt\n");
for (int i=0; i<4; i++)
{
	for (int j=0; j<4; j++)
	{
		for (int k=0; k<8; k++)
		{
			if (pos[i*4*8*1+j*8+k*1] < 100)
			{
				printf("0");
				if (pos[i*4*8*1+j*8+k*1] < 10)
				{
					printf("0");
				}
			}
			printf("%u ", pos[i*4*8*1+j*8+k*1]);
		}
		printf("    ");
	}
	printf("\n");
}
#endif
}

Ring::~Ring()
{
}

void Ring::reinit()
{
	this->last = 0;
	this->actualPos = 0;
	for (unsigned int i=0; i<MAPSIZE; i++)
	{
		this->map[i] = i;
	}
	this->operationsSinceMutation = 0;
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
		unsigned char index = (this->map[i] ^ this->pos[i%this->posLength])%MAPSIZE;
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

unsigned char Ring::encode(unsigned char c)
{
#ifndef NDEBUG
	assert(c < MAPSIZE);
#endif
	unsigned int posIndex = this->actualPos;
	this->actualPos = (posIndex + 1)%this->posLength;
	unsigned int index = (c ^ this->pos[posIndex])%MAPSIZE;
	unsigned char ret = (this->map[index] ^ this->last)%MAPSIZE;
	this->pos[posIndex] = ret;
	if (this->mutationInterval != 0)
	{
		this->operationsSinceMutation++;
		if (this->operationsSinceMutation == this->mutationInterval)
		{
			shuffle();
			this->operationsSinceMutation = 0;
		}
	}
	this->last = c;
	return ret;
}

unsigned char Ring::decode(unsigned char c)
{
#ifndef NDEBUG
	assert(c < MAPSIZE);
#endif
	unsigned int posIndex = this->actualPos;
	this->actualPos = (posIndex + 1)%this->posLength;
#ifdef KNOWNPLAINTEXTATTACK
	if (!mapSpecified[(c ^ this->last)%MAPSIZE])
	{
		return '?';
	}
#endif
	unsigned char ret = (decodeMap[(c ^ this->last)%MAPSIZE] ^ this->pos[posIndex])%MAPSIZE;
	this->pos[posIndex] = c;
	if (this->mutationInterval != 0)
	{
		this->operationsSinceMutation++;
		if (this->operationsSinceMutation == this->mutationInterval)
		{
			shuffle();
			this->operationsSinceMutation = 0;
		}
	}
	this->last = ret;
	return ret;
}

void Ring::encode(unsigned char* c, unsigned int length)
{
#ifndef NDEBUG
	assert(length > 0);
#endif
	for (unsigned int i=0; i<length; i++)
	{
		c[i] = encode(c[i]);
	}
}

void Ring::decode(unsigned char* c, unsigned int length)
{
#ifndef NDEBUG
	assert(length > 0);
#endif
	for (unsigned int i=0; i<length; i++)
	{
		c[i] = decode(c[i]);
	}
}
