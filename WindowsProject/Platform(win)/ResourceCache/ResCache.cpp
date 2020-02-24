#include"GameCodeStd.h"
#include<cctype>
#include"ResCache.h"
#include"Utilities\String.h"
#include"GameCode\GameCode.h"





ResourceZipFile::~ResourceZipFile()
{
	SAFE_DELETE(m_pZipFile);
}

bool ResourceZipFile::VOpen()
{
	m_pZipFile = New ZipFile;
	if(m_pZipFile)
	{
		return m_pZipFile->Init(m_ResFileName.c_str());
	}

	return false;
}


int ResourceZipFile::VGetRawResourceSize(const Resource& r)
{
	int resourceNum = m_pZipFile->Find(r.m_Name.c_str());
	if (resourceNum = -1)
		return -1;
	return m_pZipFile->GetFileLen(resourceNum);

}


int ResourceZipFile::VGetRawResource(const Resource& r, char* buffer)
{
	int size = 0;
	optional<int> resourceNum = m_pZipFile->Find(r.m_Name.c_str());
	if (resourceNum.valid())
	{
		size = m_pZipFile->GetFileLen(*resourceNum);
		m_pZipFile->ReadFile(*resourceNum, buffer);
	}
	return size;
}


int ResourceZipFile::VGetNumResources() const
{
	return m_pZipFile == NULL ? 0 : m_pZipFile->GetNumFile();
}


std::string ResourceZipFile::VGetResourceName(int num) const
{
	std::string resName = "";
	if (m_pZipFile != NULL && num >= 0 && num < m_pZipFile->GetNumFile())
	{
		resName = m_pZipFile->GetFileName(num);
	}
	return resName;
}


DevelopmentResourceZipFile::DevelopmentResourceZipFile(const std::wstring resFileName, const Mode mode)
	:ResourceZipFile(resFileName)
{
	m_Mode = mode;

	TCHAR dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dir);
	m_AssetsDir = dir;
	int lastSlash = m_AssetsDir.find_last_of(L"\\");
	m_AssetsDir = m_AssetsDir.substr(0, lastSlash);
	m_AssetsDir += L"\\Assets\\";

}


int DevelopmentResourceZipFile::Find(const std::string& path)
{
	std::string lowerCase = path;
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), std::tolower);
	ZipContentsMap::const_iterator it = m_DirectoryContentsMap.find(lowerCase);
	if (it == m_DirectoryContentsMap.end())
		return -1;

	return it->second;

}


bool DevelopmentResourceZipFile::VOpen()
{
	if (m_Mode != Editor)
	{
		ResourceZipFile::VOpen();
	}

	if (m_Mode == Editor)
	{
		ReadAssetsDirectory(L"*");
	}

	return true;
}


int DevelopmentResourceZipFile::VGetRawResource(const Resource& r, char* buffer)
{
	if (m_Mode == Editor)
	{
		int num = Find(r.m_Name.c_str());
		if (num = -1)
			return -1;

		std::string fileSpec = WideStringToString(m_AssetsDir, fileSpec) + r.m_Name.c_str();
		FILE* file = fopen(fileSpec.c_str(), "rb");
		size_t bytes = fread(buffer, 1, m_AssetFileInfo[num].nFileSizeLow, file);
		fclose(file);
		return bytes;
	}

	return ResourceZipFile::VGetRawResource(r, buffer);
	
}



int DevelopmentResourceZipFile::VGetRawResourceSize(const Resource& r)
{
	int size = 0;
	if (m_Mode == Editor)
	{
		int num = Find(r.m_Name.c_str());
		if (num = -1)
			return -1;
		return m_AssetFileInfo[num].nFileSizeLow;
	}

	return ResourceZipFile::VGetRawResourceSize(r);
}



int DevelopmentResourceZipFile::VGetNumResources() const
{
	return m_Mode == Editor ? m_AssetFileInfo.size() : ResourceZipFile::VGetNumResources();
}


std::string DevelopmentResourceZipFile::VGetResourceName(int num) const
{
	if (m_Mode == Editor)
	{
		std::wstring wName = m_AssetFileInfo[num].cFileName;
		std::string name;
		return WideStringToString(wName, name);
	}

	return ResourceZipFile::VGetResourceName(num);
}


void DevelopmentResourceZipFile::ReadAssetsDirectory(std::wstring fileSpec)
{
	HANDLE fileHandle;
	WIN32_FIND_DATA fileInfo;

	std::wstring path = m_AssetsDir + fileSpec;
	fileHandle = FindFirstFile(path.c_str(), &fileInfo);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		while (FindNextFile(fileHandle, &fileInfo))
		{
			if (fileInfo.dwFileAttributes& FILE_ATTRIBUTE_HIDDEN)
				continue;

			std::wstring fileName = fileInfo.cFileName;
			if (fileInfo.dwFileAttributes& FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fileName != L".."&&fileName != L".")
				{
					fileName = fileSpec.substr(0, fileSpec.length() - 1) + fileName + L"\\*";
					ReadAssetsDirectory(fileName);
				}
			}
			else
			{
				fileName = fileSpec.substr(0, fileSpec.length() - 1) + fileName;
				std::wstring lower = fileName;
				std::transform(lower.begin(), lower.end(), lower.begin(), std::tolower);
				wcscpy_s(&fileInfo.cFileName[0], MAX_PATH, lower.c_str());
				std::string file;
				WideStringToString(lower, file);
				m_DirectoryContentsMap[file] = m_AssetFileInfo.size();
				m_AssetFileInfo.push_back(fileInfo);
			}
		}
	}

	FindClose(fileHandle);
}


ResHandle::ResHandle(Resource& resource, char* buffer, unsigned int size, ResCache* pResCache)
	:m_Resource(resource)
{
	m_Buffer = buffer;
	m_Size = size;
	m_Extra = NULL;
	m_pResCache = pResCache;
}

ResHandle::~ResHandle()
{
	SAFE_DELETE_ARRAY(m_Buffer);
	m_pResCache->MemoryHasBeenFreed(m_Size);
}


ResCache::ResCache(const unsigned int sizeInMb, IResourceFile* resFile)
{
	m_CacheSize = sizeInMb * 1024 * 1024;
	m_Allocated = 0;
	m_File = resFile;
}


ResCache::~ResCache()
{
	while (!m_LRU.empty())
	{
		FreeOneResource();

	}
	SAFE_DELETE(m_File);
}


bool ResCache::Init()
{
	bool retValue = false;
	if (m_File->VOpen())
	{
		RegisterLoader(shared_ptr<IResourceLoader>(New DefaultResourceLoader()));
		retValue = true;
	}

	return retValue;
}


void ResCache::RegisterLoader(shared_ptr<IResourceLoader> loader)
{
	m_ResourceLoaders.push_front(loader);
}


shared_ptr<ResHandle> ResCache::GetHandle(Resource* r)
{
	shared_ptr<ResHandle> handle(Find(r));
	if (handle == NULL)
	{
		handle = Load(r);
		ASSERT(handle);
	}
	else
	{
		Update(handle);

	}

	return handle;
}



shared_ptr<ResHandle> ResCache::Load(Resource* r)
{
	shared_ptr<IResourceLoader> loader;
	shared_ptr<ResHandle> handle;

	for (ResourceLoaders::iterator it = m_ResourceLoaders.begin(); it != m_ResourceLoaders.end(); ++it)
	{
		shared_ptr<IResourceLoader> testLoader = *it;
		if (WildcardMatch(testLoader->VGetPattern().c_str(), r->m_Name.c_str()))
		{
			loader = testLoader;
			break;
		}

	}

	if (!loader)
	{
		ASSERT(loader&& "Resource loader not found!");
		return handle;
	}

	int rawSize = m_File->VGetRawResourceSize(*r);
	if (rawSize < 0)
	{
		ASSERT(0 && "Resource size return -1");
		return shared_ptr<ResHandle>();
	}

	int allocSize = rawSize + (loader->VAddNullZero() ? 1 : 0);
	char* rawBuffer = loader->VUseRawFile() ? Allocate(allocSize) : New char[allocSize];
	memset(rawBuffer, 0, allocSize);

	if (rawBuffer == NULL || m_File->VGetRawResource(*r, rawBuffer) == 0)
	{
		return shared_ptr<ResHandle>();
	}

	char* buffer = NULL;
	unsigned int size = 0;

	if (loader->VUseRawFile())
	{
		buffer = rawBuffer;
		handle = shared_ptr<ResHandle>(New ResHandle(*r, buffer, rawSize, this));

	}
	else
	{
		size = loader->VGetLoadedResourceSize(rawBuffer, rawSize);
		buffer = Allocate(size);
		if (buffer = NULL)
		{
			return shared_ptr<ResHandle>();
		}
		handle = shared_ptr<ResHandle>(New ResHandle(*r, buffer, size, this));
		bool success = loader->VLoadResource(rawBuffer, rawSize, handle);

		if (loader->VDiscardRawBufferAfterLoad())
			SAFE_DELETE_ARRAY(rawBuffer);

		if (!success)
			return shared_ptr<ResHandle>();
	}

	if (handle)
	{
		m_LRU.push_front(handle);
		m_Resources[r->m_Name] = handle;
	}

	return handle;
	

}


shared_ptr<ResHandle> ResCache::Find(Resource* r)
{
	ResHandleMap::iterator it = m_Resources.find(r->m_Name);
	if (it == m_Resources.end())
		return shared_ptr<ResHandle>();

	return it->second;
}


void ResCache::Update(shared_ptr<ResHandle> handle)
{
	m_LRU.remove(handle);
	m_LRU.push_front(handle);
}


char* ResCache::Allocate(unsigned int size)
{
	if (!MakeRoom(size))
		return NULL;

	char* mem = New char[size];
	if (mem)
	{
		m_Allocated += size;
	}

	return mem;
}


void ResCache::FreeOneResource()
{
	ResHandleList::iterator it = m_LRU.end();
	--it;

	shared_ptr<ResHandle> handle = *it;
	m_LRU.pop_back();
	m_Resources.erase(handle->m_Resource.m_Name);
}


void ResCache::Flush()
{
	while (!m_LRU.empty())
	{
		shared_ptr<ResHandle> handle = *(m_LRU.begin());
		Free(handle);
		m_LRU.pop_front();
	}
}


bool ResCache::MakeRoom(unsigned int size)
{
	if (size > m_CacheSize)
	{
		return false;
	}

	while (size > (m_CacheSize - m_Allocated))
	{
		if (m_LRU.empty())
			return false;

		FreeOneResource();
	}

	return true;
}


void ResCache::Free(shared_ptr<ResHandle> handle)
{
	m_LRU.remove(handle);
	m_Resources.erase(handle->m_Resource.m_Name);

}


void ResCache::MemoryHasBeenFreed(unsigned int size)
{
	m_Allocated -= size;
}


std::vector<std::string> ResCache::Match(const std::string pattern)
{
	std::vector<std::string> matchingNames;
	if (m_File == NULL)
		return matchingNames;

	int numFiles = m_File->VGetNumResources();
	for (int i = 0; i < numFiles; ++i)
	{
		std::string name = m_File->VGetResourceName(i);
		std::transform(name.begin(), name.end(), name.begin(), (int(*)(int)) std::tolower);
		if (WildcardMatch(pattern.c_str(), name.c_str()))
		{
			matchingNames.push_back(name);
		}
	}
	return matchingNames;
}


int ResCache::Preload(const std::string pattern, void(*progressCallback)(int, bool &))
{
	if (m_File == NULL)
		return 0;

	int numFiles = m_File->VGetNumResources();
	int loaded = 0;
	bool cancel = false;
	for (int i = 0; i < numFiles; ++i)
	{
		Resource resource(m_File->VGetResourceName(i));

		if (WildcardMatch(pattern.c_str(), resource.m_Name.c_str()))
		{
			shared_ptr<ResHandle> handle = g_pApp->m_ResCache->GetHandle(&resource);
			++loaded;
		}

		if (progressCallback != NULL)
		{
			progressCallback(i * 100 / numFiles, cancel);
		}
	}
	return loaded;
}


void XmlResourceExtraData::ParseXml(char* pRawBuffer)
{
	m_XmlDocument.Parse(pRawBuffer);
}


bool XmlResourceLoader::VLoadResource(char *rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle)
{
	if (rawSize <= 0)
		return false;

	shared_ptr<XmlResourceExtraData> pExtraData = shared_ptr<XmlResourceExtraData>(New XmlResourceExtraData());
	pExtraData->ParseXml(rawBuffer);

	handle->SetExtra(shared_ptr<XmlResourceExtraData>(pExtraData));

	return true;
}


shared_ptr<IResourceLoader> CreateXmlResourceLoader()
{
	return shared_ptr<IResourceLoader>(New XmlResourceLoader());
}


TiXmlElement* XmlResourceLoader::LoadAndReturnRootXmlElement(const char* r)
{
	Resource resource(r);
	shared_ptr<ResHandle> pResourceHandle = g_pApp->m_ResCache->GetHandle(&resource);  // This actually loads the XML file from the zip file
	shared_ptr<XmlResourceExtraData> pExtraData = static_pointer_cast<XmlResourceExtraData>(pResourceHandle->GetExtra());
	return pExtraData->GetRoot();
}


