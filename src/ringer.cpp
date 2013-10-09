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

#define DEFAULTTHREADNUMBER 2
#define DEFAULTBUFFERSIZE IVSIZE
#define DEFAULTMUTATIONINTERVAL IVSIZE

//TODO workaround
#define threadCountInt DEFAULTTHREADNUMBER

//#define NDEBUG

#ifdef MULTICORE
typedef struct
{
	char* data;
	unsigned int dataOffset;
	unsigned int dataLength;
	bool encode;
	Ring* ring;
	pthread_mutex_t* threadWait;
	pthread_mutex_t* mainWait;
	unsigned int id;
} ThreadArguments;

void* threadedJob(void* threadArgument)
{
	pthread_mutex_t* threadWait = ((ThreadArguments*)threadArgument)->threadWait;
	pthread_mutex_t* mainWait = ((ThreadArguments*)threadArgument)->mainWait;
	unsigned int id = ((ThreadArguments*)threadArgument)->id;
	while (true)
	{
		pthread_mutex_lock(threadWait);
		char* data = ((ThreadArguments*)threadArgument)->data;
		unsigned int dataOffset = ((ThreadArguments*)threadArgument)->dataOffset;
		unsigned int dataLength = ((ThreadArguments*)threadArgument)->dataLength;
		bool encode = ((ThreadArguments*)threadArgument)->encode;
		Ring* ring = ((ThreadArguments*)threadArgument)->ring;
		if (encode)
		{
			ring->encode((unsigned char*)(data+dataOffset), dataLength, dataOffset);
		}
		else
		{
			ring->decode((unsigned char*)(data+dataOffset), dataLength, dataOffset);
		}
		pthread_mutex_unlock(mainWait);
	}
	pthread_exit(NULL);
}
#endif

int main(int argc, char* argv[])
{
	char inputFile[256+1];
	char outputFile[256+1];
	char key[MAPSIZE+1];
	char salt[IVSIZE];
	char mutationInterval[4+1];
	char buffer[6+1];
	char keyFile[256+1];
	char threadCount[1+1];
	bool threadCountSet = false;
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
		if (strcmp(argv[i], "-cpucount") == 0 || strcmp(argv[i], "-c") == 0)
		{
			i++;
			if (i < argc)
			{
				if (strlen(argv[i]) <= 1)
				{
					strcpy(threadCount, argv[i]);
					threadCountSet = true;
				}
			}
			continue;
		}
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
	if (!threadCountSet)
	{
		sprintf(threadCount, "%u", DEFAULTTHREADNUMBER);
	}
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
	//unsigned int threadCountInt = atoi(threadCount);//TODO workaround
assert(threadCountInt==1 || threadCountInt==2 || threadCountInt==4 || threadCountInt==8);
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

#ifdef MULTICORE
	//thread setup
	pthread_t threads[threadCountInt];
	ThreadArguments ta[threadCountInt];
	char buf2[bufferInt];
#ifndef NDEBUG
	assert(IVSIZE%threadCountInt == 0);
	assert(mutationIntervalInt%bufferInt == 0);
#endif
	pthread_mutex_t threadWait[threadCountInt];
	pthread_mutex_t mainWait[threadCountInt];
	pthread_attr_t threadAttributes[threadCountInt];
	for (unsigned int i=0; i<threadCountInt; i++)
	{
		//dummy, inited in mainloop
		ta[i].data = NULL;
		ta[i].dataOffset = 0;
		ta[i].dataLength = 0;
		//real
		pthread_mutex_init(&threadWait[i], NULL);
		pthread_mutex_lock(&threadWait[i]);
		pthread_mutex_init(&mainWait[i], NULL);
		ta[i].threadWait = &threadWait[i];
		ta[i].mainWait = &mainWait[i];
		ta[i].encode = encode;
		ta[i].id = i;
		ta[i].ring = &ring;
		if (int ret = pthread_attr_setstacksize(&threadAttributes[i], 1024*1024*8))
		{
			errno = ret;
			perror("ERROR: could not set stacksize");
			return ret;
		}
		if (int ret = pthread_create(&threads[i], &threadAttributes[i], threadedJob, (void*)&ta[i]))
		{
			errno = ret;
			perror("ERROR: thread could not be created");
			return ret;
		}
	}
#endif

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
		//read1
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
#ifdef MULTICORE
		//init threads1
		unsigned int assignedData = 0;
		for (unsigned int i=0; i<threadCountInt; i++)
		{
			ta[i].data = buf;
			ta[i].dataOffset = i*(IVSIZE/threadCountInt);
			if (assignedData + IVSIZE/threadCountInt <= readRet)
			{
				ta[i].dataLength = IVSIZE/threadCountInt;
			}
			else
			{
				ta[i].dataLength = readRet-assignedData;
			}
			assignedData += ta[i].dataLength;
			pthread_mutex_lock(&mainWait[i]);
			pthread_mutex_unlock(&threadWait[i]);//start thread i
		}
		/*
		 * threads are working here1
		 */
#ifdef MULTICORE2
		//read2
		unsigned int bytesToRead2 = bufferInt;
		if (readBytes+bytesToRead2 > fileSize)
		{
			bytesToRead = fileSize - readBytes;
		}
		unsigned int readRet2 = fread(&buf2, 1, bytesToRead, in);
		readBytes += readRet2;
#ifndef NDEBUG
		assert(readRet2 == bytesToRead);
#endif
#endif
		for (unsigned int i=0; i<threadCountInt; i++)//wait until all threads have finished theire work
		{
			pthread_mutex_lock(&mainWait[i]);//wait for thread i
			pthread_mutex_unlock(&mainWait[i]);
		}
		/*
		 * threads are not working anylonger1
		 */
		if (ring.mutationInterval != 0)
		{
			ring.operationsSinceMutation += readRet;
			if (ring.operationsSinceMutation >= ring.mutationInterval)
			{
				ring.shuffle();
				ring.operationsSinceMutation = 0;
			}
		}
#ifdef MULTICORE2
		//init threads2
		assignedData = 0;
		for (unsigned int i=0; i<threadCountInt; i++)
		{
			ta[i].data = buf2;
			ta[i].dataOffset = i*(IVSIZE/threadCountInt);
			if (assignedData + IVSIZE/threadCountInt <= readRet2)
			{
				ta[i].dataLength = IVSIZE/threadCountInt;
			}
			else
			{
				ta[i].dataLength = readRet2-assignedData;
			}
			assignedData += ta[i].dataLength;
			pthread_mutex_lock(&mainWait[i]);
			pthread_mutex_unlock(&threadWait[i]);//start thread i
		}
		/*
		 * threads are working here2
		 */
#endif
		//write1
		unsigned int writtenBytes = 0;
		while (writtenBytes < readRet)
		{
			unsigned int writeRet = fwrite((&buf)+writtenBytes, 1, readRet, out);
			writtenBytes += writeRet;
		}
#ifdef MULTICORE2
		for (unsigned int i=0; i<threadCountInt; i++)//wait until all threads have finished theire work
		{
			pthread_mutex_lock(&mainWait[i]);//wait for thread i
			pthread_mutex_unlock(&mainWait[i]);
		}
		/*
		 * threads are not working anylonger2
		 */
		if (ring.mutationInterval != 0)
		{
			ring.operationsSinceMutation += readRet2;
			if (ring.operationsSinceMutation >= ring.mutationInterval)
			{
				ring.shuffle();
				ring.operationsSinceMutation = 0;
			}
		}
		//write2
		writtenBytes = 0;
		while (writtenBytes < readRet2)
		{
			unsigned int writeRet = fwrite((&buf2)+writtenBytes, 1, readRet2, out);
			writtenBytes += writeRet;
		}
#endif
#else
		if (encode)
		{
			ring.encode((unsigned char*)buf, readRet, 0);
		}
		else
		{
			ring.decode((unsigned char*)buf, readRet, 0);
		}
		//write
		unsigned int writtenBytes = 0;
		while (writtenBytes < readRet)
		{
			unsigned int writeRet = fwrite((&buf)+writtenBytes, 1, readRet, out);
			writtenBytes += writeRet;
		}
#endif
	}

	//finishing
#ifdef MULTICORE
	for (unsigned int i=0; i<threadCountInt; i++)
	{
		pthread_mutex_destroy(&mainWait[i]);
		pthread_mutex_destroy(&threadWait[i]);
		pthread_cancel(threads[i]);
		pthread_attr_destroy(&threadAttributes[i]);
	}
#endif
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