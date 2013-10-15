#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cassert>
#include "ring.hpp"
#include <stdint.h>
#include <QFile>

#define DEFAULTBUFFERSIZE 1024
#define DEFAULTMUTATIONINTERVAL 16

int main(int argc, char* argv[])
{
	char inputFile[256+1];
	char outputFile[256+1];
	char key[MAPSIZE+1];
	char salt[IVSIZE];
	char mutationInterval[4+1];
	char buffer[6+1];
	char keyFile[256+1];
	bool inputSet = false;
	bool outputSet = false;
	bool keySet = false;
	bool encode = false;
	bool decode = false;
	bool mutationIntervalSet = false;
	bool bufferSet = false;
	bool keyFileSet = false;
	bool verbose = false;
	bool removeSet = false;
	for (int i=0; i<argc; i++)//collect arguments from cmdline
	{
		if (strcmp(argv[i], "-in") == 0 || strcmp(argv[i], "-i") == 0)
		{
			i++;
			if (i < argc)
			{
				if (strlen(argv[i]) <= 256)
				{
					strcpy(inputFile, argv[i]);
					inputSet = true;
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-out") == 0 || strcmp(argv[i], "-o") == 0)
		{
			i++;
			if (i < argc)
			{
				if (strlen(argv[i]) <= 256)
				{
					strcpy(outputFile, argv[i]);
					outputSet = true;
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-password") == 0 || strcmp(argv[i], "-p") == 0)
		{
			i++;
			if (i < argc)
			{
				if (strlen(argv[i]) <= 256)
				{
					strcpy(key, argv[i]);
					keySet = true;
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-key") == 0 || strcmp(argv[i], "-k") == 0)
		{
			i++;
			if (i < argc)
			{
				if (strlen(argv[i]) <= 256)
				{
					strcpy(keyFile, argv[i]);
					keyFileSet = true;
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-encode") == 0 || strcmp(argv[i], "-e") == 0)
		{
			encode = true;
			continue;
		}
		if (strcmp(argv[i], "-decode") == 0 || strcmp(argv[i], "-d") == 0)
		{
			decode = true;
			continue;
		}
		if (strcmp(argv[i], "-verbose") == 0 || strcmp(argv[i], "-v") == 0)
		{
			verbose = true;
			continue;
		}
		if (strcmp(argv[i], "-remove") == 0 || strcmp(argv[i], "-r") == 0)
		{
			removeSet = true;
			continue;
		}
		if (strcmp(argv[i], "-mutation") == 0 || strcmp(argv[i], "-m") == 0)
		{
			i++;
			if (i < argc)
			{
				if (strlen(argv[i]) <= 4)
				{
					strcpy(mutationInterval, argv[i]);
					mutationIntervalSet = true;
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-buffer") == 0 || strcmp(argv[i], "-b") == 0)
		{
			i++;
			if (i < argc)
			{
				if (strlen(argv[i]) <= 6)
				{
					strcpy(buffer, argv[i]);
					bufferSet = true;
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			printf("%s [-e|-d] [-i <inputfile>] [-o <outputfile>] [-p <password> | -k <keyfile>] [-m <mutationInterval>] [-b <buffersize>] [-v] [-r] [-h]\n", argv[0]);
			printf("\n");
			printf("Encodes or decodes a single file.\n");
			printf("\n");
			printf("-e\n");
			printf("-encode\n");
			printf("\tdefault\n");
			printf("\toptional\n");
			printf("\tSpecifies the mode as \"encode\".\n");
			printf("\n");
			printf("-d\n");
			printf("-decode\n");
			printf("\toptional\n");
			printf("\tSpecifies the mode as \"decode\".\n");
			printf("\n");
			printf("-i <inputfile>\n");
			printf("-in <inputfile>\n");
			printf("\tmust\n");
			printf("\tSpecifies the inputfile.\n");
			printf("\n");
			printf("-o <outputfile>\n");
			printf("-out <outputfile>\n");
			printf("\tmust\n");
			printf("\tSpecifies the outputfile.\n");
			printf("\n");
			printf("-p <password>\n");
			printf("-password <password>\n");
			printf("\tmust\n");
			printf("\tSpecifies the password.\n");
			printf("\n");
			printf("-k <keyfile>\n");
			printf("-key <keyfile>\n");
			printf("\toptional (alternative for -p)\n");
			printf("\tSpecifies a keyfile.\n");
			printf("\n");
			printf("-m <mutationInterval>\n");
			printf("-mutation <mutationInterval>\n");
			printf("\tdefault: %u\n", DEFAULTMUTATIONINTERVAL);
			printf("\toptional\n");
			printf("\tSpecifies the mutationInterval.\n");
			printf("\n");
			printf("-b <buffersize>\n");
			printf("-buffer <buffersize>\n");
			printf("\tdefault: %u\n", DEFAULTBUFFERSIZE);
			printf("\toptional\n");
			printf("\tSpecifies the buffersize.\n");
			printf("\n");
			printf("-v\n");
			printf("-verbose\n");
			printf("\toptional\n");
			printf("\tMakes the program verbose.\n");
			printf("\n");
			printf("-r\n");
			printf("-remove\n");
			printf("\toptional\n");
			printf("\tRemoves the inputfile after reading.\n");
			printf("\n");
			printf("-h\n");
			printf("-help\n");
			printf("\toptional\n");
			printf("\tPrints this helpmessage and quits the program.\n");
			return 0;
		}
	}

	//evaluate arguments
	if (!inputSet)
	{
		printf("You need to specify an inputfile!\n");
		return -EINVAL;
	}
	if (keySet && keyFileSet)
	{
		printf("You shall not specify a password and a keyfile!\n");
		return -EINVAL;
	}
	if (!keySet && !keyFileSet)
	{
		printf("You need to specify a password or a keyfile!\n");
		return -EINVAL;
	}
	if (encode && decode)
	{
		printf("You may only specify one mode!\n");
		return -EINVAL;
	}
	if (keyFileSet)
	{
		QFile in(keyFile);
		if (!in.open(QIODevice::ReadOnly))
		{
			printf("ERROR: can not open keyfile\n");
			errno = EIO;
			return -errno;
		}
		unsigned int fileSize = in.size();
		if (fileSize > MAPSIZE)
		{
			printf("ERROR: keyfile is to large to contain a single key -> invalid\n");
			in.close();
			errno = EMSGSIZE;
			return -EMSGSIZE;
		}
		if (in.read((char*)(&key), fileSize) != fileSize)
		{
			in.close();
			errno = EIO;
			return -EIO;
		}
		key[fileSize] = '\0';
		in.close();
		printf("Password read from keyfile.\n");
	}
	if (!encode && !decode)
	{
		encode = true;
		printf("No mode specified... using \"encodemode\".\n");
	}
	if (!outputSet)
	{
		strcpy(outputFile, inputFile);
		if (encode)
		{
			strcat(outputFile, ".enc");
		}
		else
		{
			strcat(outputFile, ".dec");
		}
		printf("No outputfile specified... using \"%s\".\n", outputFile);
	}
	if (!mutationIntervalSet && encode)
	{
		sprintf(mutationInterval, "%u", DEFAULTMUTATIONINTERVAL);
		printf("No mutationInterval specified... using \"%s\".\n", mutationInterval);
	}
	if (!bufferSet)
	{
		sprintf(buffer, "%u", DEFAULTBUFFERSIZE);
		printf("No buffersize specified... using \"%s\".\n", buffer);
	}

	//setup
	if (encode)
	{
		QFile in("/dev/urandom");
		if (!in.open(QIODevice::ReadOnly))
		{
			printf("ERROR: can not open \"/dev/urandom\"\n");
			errno = EIO;
			return errno;
		}
		if (in.read((char*)(&salt), IVSIZE) != IVSIZE)
		{
			in.close();
			errno = EIO;
			return -EIO;
		}
		in.close();
		printf("Salt generated\n");
	}
	unsigned int bufferInt = atoi(buffer);
	unsigned int mutationIntervalInt;
	QFile in(inputFile);
	if (!in.open(QIODevice::ReadOnly))
	{
		printf("ERROR: can not open inputfile\n");
		errno = EIO;
		return -errno;
	}
	QFile out(outputFile);
	if (!in.open(QIODevice::WriteOnly))
	{
		printf("ERROR: can not open outputfile\n");
		in.close();
		errno = EIO;
		return errno;
	}
	unsigned int fileSize = in.size();
	if (encode)
	{
		mutationIntervalInt = atoi(mutationInterval);
		uint32_t mutationInterval32 = mutationIntervalInt;
		if (out.write((const char*)(&mutationInterval32), sizeof(mutationInterval32)) != sizeof(mutationInterval32))
		{
			in.close();
			out.close();
			errno = EIO;
			return -EIO;
		}
		if (out.write((const char*)(&salt), IVSIZE) != IVSIZE)
		{
			in.close();
			out.close();
			errno = EIO;
			return -EIO;
		}
		printf("Salt saved\n");
	}
	else
	{
		uint32_t mutationInterval32;
		if (in.read((char*)(&mutationInterval32), sizeof(mutationInterval32)) != sizeof(mutationInterval32))
		{
			in.close();
			out.close();
			errno = EIO;
			return -EIO;
		}
		mutationIntervalInt = mutationInterval32;
		fileSize -= sizeof(mutationInterval32);
		printf("Mutationinterval loaded\n");
		if (in.read((char*)(&salt), IVSIZE) != IVSIZE)
		{
			in.close();
			out.close();
			errno = EIO;
			return -EIO;
		}
		fileSize -= IVSIZE;
		printf("Salt loaded\n");
	}
	Ring ring((const unsigned char*)key, strlen(key), (const unsigned char*)salt, IVSIZE, mutationIntervalInt);
	unsigned int readBytes = 0;
	char buf[bufferInt];
	char verboseMessage[1+8+1+6+1];

	//encode/decode
	while (readBytes < fileSize)
	{
		if (verbose)//print
		{
			float progress = (100.0*readBytes)/fileSize;
			if (encode)
			{
				sprintf(verboseMessage, "\rencoding %.1f%%", progress);
			}
			else
			{
				sprintf(verboseMessage, "\rdecoding %.1f%%", progress);
			}
			printf("%s", verboseMessage);
		}
		//read
		unsigned int bytesToRead = bufferInt;
		if (readBytes+bytesToRead > fileSize)
		{
			bytesToRead = fileSize - readBytes;
		}
		unsigned int readRet = in.read((char*)(&buf), bytesToRead);
		readBytes += readRet;
#ifndef NDEBUG
		assert(readRet == bytesToRead);
#endif
		if (encode)
		{
			ring.encode((unsigned char*)buf, readRet);
		}
		else
		{
			ring.decode((unsigned char*)buf, readRet);
		}
		//write
		unsigned int writtenBytes = 0;
		while (writtenBytes < readRet)
		{
			unsigned int writeRet = out.write((const char*)((&buf)+writtenBytes), readRet);
			writtenBytes += writeRet;
		}
	}

	//finishing
	if (verbose)
	{
		if (encode)
		{
			sprintf(verboseMessage, "\rencoding %.1f%%", 100.0);
		}
		else
		{
			sprintf(verboseMessage, "\rdecoding %.1f%%", 100.0);
		}
		printf("%s\n", verboseMessage);
	}
	in.close();
	out.close();
	if (removeSet)
	{
		if (remove(inputFile))
		{
			perror("WARNING: could not delete inputfile");
		}
		else
		{
			printf("Inputfile deleted\n");
		}
	}
	printf("done\n");
	return 0;
}
