#pragma once
//This class is based on a similiar class provided by Javier Arevalo

#include<stdio.h>
#include<map>

typedef std::map<std::string, int> ZipContentsMap;

class ZipFile
{
	struct TZipDirHeader;
	struct TZipDirFileHeader;
	struct TZipLocalHeader;

	FILE* m_pFile;
	char* m_pDirData;
	int m_NumEntries;
	const TZipDirFileHeader** m_ppDir;

public:
	ZipContentsMap m_ZipContentsMap;

public:
	ZipFile()
	{
		m_NumEntries = 0;
		m_pFile = NULL;
		m_pDirData = NULL;
	}

	void End();

	virtual ~ZipFile()
	{
		End();
		fclose(m_pFile);
	}

	bool Init(const std::wstring& resFileName);

	int GetNumFile() const { return m_NumEntries; }
	std::string GetFileName(int i) const;
	int GetFileLen(int i) const;
	bool ReadFile(int i, void* pBuf);
	bool ReadLargeFile(int i, void* pBuf, void(*progressCallBack)(int, bool&));
	int Find(const std::string& path) const;




};