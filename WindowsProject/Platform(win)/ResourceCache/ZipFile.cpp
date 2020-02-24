#include"GameCodeStd.h"
#include"ZipFile.h"
#include<cctype>
#include<zlib.h>
#include<string>

#pragma pack(push)
#pragma pack(1)

struct
	ZipFile::TZipLocalHeader
{
	enum 
	{
		SIGNATURE = 0x04034b50
	};
	DWORD sig;
	WORD version;
	WORD flag;
	WORD compression;
	WORD modTime;
	WORD modDate;
	DWORD crc32;
	DWORD cSize;
	DWORD ucSize;
	WORD fNameLen;
	WORD xtraLen;

};

struct ZipFile::TZipDirHeader
{
	enum
	{
		SIGNATURE= 0x06054b50
	};
	DWORD sig;
	WORD nDisk;
	WORD nStartDisk;
	WORD nDirEntries;
	WORD totalDirEntries;
	DWORD dirSize;
	DWORD dirOffSet;
	WORD cmntLen;
};

struct ZipFile::TZipDirFileHeader
{
	enum
	{
		SIGNATURE= 0x02014b50
	};
	DWORD sig;
	WORD verMade;
	WORD verNeeded;
	WORD flag;
	WORD compression;
	WORD modTime;
	WORD modDate;
	DWORD crc32;
	DWORD cSize;
	DWORD ucSize;
	WORD fNameLen;
	WORD xtraLen;
	WORD cmntLen;
	WORD diskStart;
	WORD intAttr;
	DWORD extAttr;
	DWORD hdrOffSet;

	char* GetName() const { return (char*)(this + 1); }
	char* GetExtra() const { return GetName() + fNameLen; }
	char* GetComment() const { return GetExtra() + xtraLen; }
};
#pragma pack(pop)

bool ZipFile::Init(const std::wstring& resFileName)
{
	//Open file
	End();
	_wfopen_s(&m_pFile, resFileName.c_str(), _T("rb"));
	if (!m_pFile)
		return false;

	//Looking for Zip file header
	TZipDirHeader zipHeader;
	fseek(m_pFile, -(int)sizeof(zipHeader), SEEK_END);
	long headerOffSet = ftell(m_pFile);
	memset(&zipHeader, 0, sizeof(zipHeader));
	fread(&zipHeader, sizeof(zipHeader), 1, m_pFile);

	if (zipHeader.sig != TZipDirHeader::SIGNATURE)
		return false;

	//Looking for directory
	fseek(m_pFile, headerOffSet - zipHeader.dirSize, SEEK_SET);
	m_pDirData = New char[zipHeader.dirSize + zipHeader.nDirEntries * sizeof(*m_ppDir)];
	if (!m_pDirData)
		return false;
	memset(m_pDirData, 0, zipHeader.dirSize + zipHeader.nDirEntries * sizeof(*m_ppDir));
	fread(m_pDirData, zipHeader.dirSize, 1, m_pFile);

	//Process each entry
	char* pFileHeader = m_pDirData;
	m_ppDir = (const TZipDirFileHeader**)(m_pDirData + zipHeader.dirSize);

	bool success = true;

	for (int i = 0; i < zipHeader.nDirEntries&& success; ++i)
	{
		TZipDirFileHeader& fileHeader = *(TZipDirFileHeader*)pFileHeader;
		m_ppDir[i] = &fileHeader;

		if (fileHeader.sig != TZipDirFileHeader::SIGNATURE)
			success = false;
		else
		{
			pFileHeader += sizeof(fileHeader);

			//Convert UNIX slashes to DOS backslashes
			for (int j = 0; j < fileHeader.fNameLen; ++j)
				if (pFileHeader[j] == '/')
					pFileHeader[j] = '\\';

			//Save each entry in filename-index map
			char fileName[MAX_PATH];
			memcpy(fileName, pFileHeader, fileHeader.fNameLen);
			fileName[fileHeader.fNameLen] = 0;
			_strlwr_s(fileName, MAX_PATH);
			std::string spath = fileName;
			m_ZipContentsMap[spath] = i;
			pFileHeader += fileHeader.fNameLen + fileHeader.xtraLen + fileHeader.cmntLen;

		}
	}
	if (!success)
	{
		SAFE_DELETE_ARRAY(m_pDirData);
	}
	else
	{
		m_NumEntries = zipHeader.nDirEntries;
	}

	return success;


}


int ZipFile::Find(const std::string& path) const
{
	std::string lowerCase = path;
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), std::tolower);
	ZipContentsMap::const_iterator it = m_ZipContentsMap.find(lowerCase);
	if (it == m_ZipContentsMap.end())
		return -1;

	return it->second;
}


void ZipFile::End()
{
	m_ZipContentsMap.clear();
	SAFE_DELETE_ARRAY(m_pDirData);
	m_NumEntries = 0;
}


std::string ZipFile::GetFileName(int i) const
{
	std::string fileName = "";
	if (i >= 0 && i < m_NumEntries)
	{
		char name[MAX_PATH];
		memcpy(name, m_ppDir[i]->GetName(), m_ppDir[i]->fNameLen);
		name[m_ppDir[i]->fNameLen] = '\0';
		fileName = name;
	}
	return fileName;
}


int ZipFile::GetFileLen(int i) const
{
	if (i < 0 || i >= m_NumEntries)
		return -1;
	else
		return m_ppDir[i]->ucSize;
}


bool ZipFile::ReadFile(int i, void* pBuf)
{
	if (pBuf == NULL || i < 0 || i >= m_NumEntries)
		return false;

	//Look for local header
	fseek(m_pFile, m_ppDir[i]->hdrOffSet, SEEK_SET);
	TZipLocalHeader localHeader;

	memset(&localHeader, 0, sizeof(localHeader));
	fread(&localHeader, sizeof(localHeader), 1, m_pFile);
	if (localHeader.sig != TZipLocalHeader::SIGNATURE)
		return false;
	fseek(m_pFile, localHeader.fNameLen + localHeader.xtraLen, SEEK_CUR);

	if (localHeader.compression == Z_NO_COMPRESSION)
	{
		//Uncompressed, read directly
		fread(pBuf, localHeader.cSize, 1, m_pFile);
		return true;
	}
	else if
		(localHeader.compression != Z_DEFLATED)
		return false;

	char* pCompressed = New char[localHeader.cSize];
	if (!pCompressed)
		return false;

	memset(pCompressed, 0, localHeader.cSize);
	fread(pCompressed, localHeader.cSize, 1, m_pFile);

	bool ret = true;

	//Decompress
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)pCompressed;
	stream.avail_in = (UINT)localHeader.cSize;
	stream.next_out = (Bytef*)pBuf;
	stream.avail_out = localHeader.ucSize;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	err = inflateInit2(&stream, -MAX_WBITS);
	if (err == Z_OK)
	{
		err = inflate(&stream, Z_FINISH);
		inflateEnd(&stream);
		if (err = Z_STREAM_END)
			err = Z_OK;
		inflateEnd(&stream);
	}
	if (err != Z_OK)
		ret = false;
	
	delete[] pCompressed;
	return ret;
}


bool ZipFile::ReadLargeFile(int i, void* pBuf, void (*progressCallBack)(int, bool&))
{
	if (pBuf == NULL || i < 0 || i >= m_NumEntries)
		return false;

	//Look for local header
	fseek(m_pFile, m_ppDir[i]->hdrOffSet, SEEK_SET);
	TZipLocalHeader localHeader;

	memset(&localHeader, 0, sizeof(localHeader));
	fread(&localHeader, sizeof(localHeader), 1, m_pFile);
	if (localHeader.sig != TZipLocalHeader::SIGNATURE)
		return false;
	fseek(m_pFile, localHeader.fNameLen + localHeader.xtraLen, SEEK_CUR);

	if (localHeader.compression == Z_NO_COMPRESSION)
	{
		//Uncompressed, read directly
		fread(pBuf, localHeader.cSize, 1, m_pFile);
		return true;
	}
	else if
		(localHeader.compression != Z_DEFLATED)
		return false;

	char* pCompressed = New char[localHeader.cSize];
	if (!pCompressed)
		return false;

	memset(pCompressed, 0, localHeader.cSize);
	fread(pCompressed, localHeader.cSize, 1, m_pFile);

	bool ret = true;

	//Decompress
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)pCompressed;
	stream.avail_in = (UINT)localHeader.cSize;
	stream.next_out = (Bytef*)pBuf;
	stream.avail_out = 128 * 1024; //Read 128k at a time
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	err = inflateInit2(&stream, -MAX_WBITS);
	if (err == Z_OK)
	{
		UINT count = 0;
		bool cancel = false;
		while (stream.total_in < (UINT)localHeader.cSize && !cancel)
		{
			err = inflate(&stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END)
			{
				err = Z_OK;
				break;
			}
			else if (err != Z_OK)
			{
				ASSERT(0 && "Decompressing failed");
				break;
			}
			
			count += (128 * 1024);
			stream.avail_out = 128 * 1024;
			stream.next_in += stream.total_out;
			progressCallBack(count * 100 / localHeader.cSize, cancel);
		}
		inflateEnd(&stream);
	}
	if (err != Z_OK)
		ret = false;

	delete[] pCompressed;
	return ret;
}

