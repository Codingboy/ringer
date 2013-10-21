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

void decodeDir(QDir dir, QMap<QFile, QString> map)
{
	QList<QFileInfo> files = dir.entryInfoList();
	for (int i=0; i<files.length(); i++)
	{
		QFileInfo file = files.at(i);
		if (file.isFile() && file.isReadable())
		{
			decodeFile(QFile(file.absolutePath()), map);
		}
		else if (file.isDir() && file.isReadable)
		{
			decodeDir(QDir(file.absoluteFilePath()), map);
		}
	}
}

void encodeFile(QFile file, QMap<QFile, QString> map)
{
	
}

void encodeHome()
{
	QFile pwFile(QDIR::homePath() + QDir::separator() + "pwFile");
assert(!pwFile.exists());
	pwFile.open(QIODevice::WriteOnly);
	encodeDir(QDir.home(), pwFile);
	pwFile.close();
//TODO encode pwFile
}

void decodeHome()
{
	decodeDir(QDir.home());
}
