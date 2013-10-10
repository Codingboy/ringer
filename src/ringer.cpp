#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ring.hpp"

#ifdef MULTICORE
#include <pthread.h>
#endif

#define DEFAULTBUFFERSIZE 64
#define DEFAULTMUTATIONINTERVAL IVSIZE

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
		FILE* in = fopen(keyFile, "r");
		if (!in)
		{
			perror("ERROR: can not open keyfile");
			return errno;
		}
		struct stat st;
		if (stat((const char*)keyFile, &st))
		{
			perror("ERROR: could not read filesize of keyfile");
			fclose(in);
			return errno;
		}
		unsigned int fileSize = st.st_size;
		if (fileSize > MAPSIZE)
		{
			printf("ERROR: keyfile is to large to contain a single key -> invalid\n");
			fclose(in);
			errno = -EMSGSIZE;
			return -EMSGSIZE;
		}
		if (fread(&key, 1, fileSize, in) != fileSize)
		{
			fclose(in);
			errno = -EIO;
			return -EIO;
		}
		key[fileSize] = '\0';
		fclose(in);
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
	if (!mutationIntervalSet)
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
		FILE* in = fopen("/dev/urandom", "r");
		if (!in)
		{
			perror("ERROR: can not open \"/dev/urandom\"");
			return errno;
		}
		if (fread(&salt, 1, IVSIZE, in) != IVSIZE)
		{
			fclose(in);
			errno = -EIO;
			return -EIO;
		}
		if (fclose(in))
		{
			perror("WARNING: could not close \"/dev/urandom\"");
		}
		printf("Salt generated\n");
	}
	unsigned int bufferInt = atoi(buffer);
	unsigned int mutationIntervalInt = atoi(mutationInterval);
	FILE* in = fopen(inputFile, "r");
	if (!in)
	{
		perror("ERROR: can not open inputfile");
		return errno;
	}
	FILE* out = fopen(outputFile, "w");
	if (!out)
	{
		perror("ERROR: can not open outputfile");
		fclose(in);
		return errno;
	}
	if (encode)
	{
		if (fwrite(&salt, 1, IVSIZE, out) != IVSIZE)
		{
			fclose(in);
			fclose(out);
			errno = -EIO;
			return -EIO;
		}
		printf("Salt saved\n");
	}
	else
	{
		if (fread(&salt, 1, IVSIZE, in) != IVSIZE)
		{
			fclose(in);
			fclose(out);
			errno = -EIO;
			return -EIO;
		}
		printf("Salt loaded\n");
	}
	Ring ring((const unsigned char*)key, strlen(key), (const unsigned char*)salt, IVSIZE, mutationIntervalInt);
	struct stat st;
	if (stat((const char*)inputFile, &st))
	{
		perror("ERROR: could not read filesize of inputfile");
		fclose(in);
		fclose(out);
		return errno;
	}
	unsigned int fileSize = st.st_size;
	if (!encode)
	{
		fileSize -= IVSIZE;//exclude the salt
	}
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
		unsigned int readRet = fread(&buf, 1, bytesToRead, in);
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
			unsigned int writeRet = fwrite((&buf)+writtenBytes, 1, readRet, out);
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
	if (fclose(in))
	{
		perror("WARNING: could not close inputfile");
	}
	if (fclose(out))
	{
		perror("WARNING: could not close outputfile");
	}
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