void encodeDir(QDir dir)
{
	QList<QFileInfo> files = dir.entryInfoList();
	for (int i=0; i<files.length(); i++)
	{
		QFileInfo file = files.at(i);
		if (file.isFile() && file.isReadable())
		{
			encodeFile(QFile(file.absoluteFilePath()));
		}
		else if (file.isDir() && file.isReadable)
		{
			encodeDir(QDir(file.absoluteFilePath()));
		}
	}
}

void decodeDir(QDir dir)
{
	QList<QFileInfo> files = dir.entryInfoList();
	for (int i=0; i<files.length(); i++)
	{
		QFileInfo file = files.at(i);
		if (file.isFile() && file.isReadable())
		{
			decodeFile(QFile(file.absoluteFilePath()));
		}
		else if (file.isDir() && file.isReadable)
		{
			decodeDir(QDir(file.absoluteFilePath()));
		}
	}
}

void encodeFile(QFile file)
{
	
}

void encodeHome()
{
	encodeDir(QDir.home());
}

void decodeHome()
{
	decodeDir(QDir.home());
}
