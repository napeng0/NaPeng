#pragma once
#include"ZipFile.h"
#include"GameCode\interface.h"


class ResHandle;
class ResCache;

using namespace std;

class IResourceExtraData
{
public:
	virtual std::string VToString() = 0;
};


class Resource
{
public:
	std::string m_Name;
	Resource(const std::string& name);
};


Resource::Resource(const std::string& name)
{
	m_Name = name;
	std::transform(m_Name.begin(), m_Name.end(), m_Name.begin(), std::tolower);
}


class ResourceZipFile : public IResourceFile
{
protected:

	ZipFile* m_pZipFile;
	std::wstring m_ResFileName;

public:
	ResourceZipFile(const std::wstring resFileName)
	{
		m_pZipFile = NULL;
		m_ResFileName = resFileName;
	}
	virtual~ResourceZipFile();

	virtual bool VOpen();
	virtual int VGetRawResourceSize(const Resource& r);
	virtual int VGetRawResource(const Resource& r, char* buffer);
	virtual int VGetNumResources() const;
	virtual std::string VGetResourceName(int num) const;
	virtual bool VIsUsingDevelopmentDirectories() const
	{
		return false;
	}

};

class DevelopmentResourceZipFile : public ResourceZipFile
{

public:
	enum Mode
	{
		Development,
		Editor
	};

	Mode m_Mode;
	std::wstring m_AssetsDir;
	std::vector<WIN32_FIND_DATA> m_AssetFileInfo;
	ZipContentsMap m_DirectoryContentsMap;

public:
	DevelopmentResourceZipFile(const std::wstring resFileName, const Mode mode);

	virtual bool VOpen();
	virtual int VGetRawResourceSize(const Resource& r);
	virtual int VGetRawResource(const Resource& r, char* buffer);
	virtual int VGetNumResources() const;
	virtual std::string VGetResourceName(int num) const;
	virtual bool VIsUsingDevelopmentDirectories() const
	{
		return true;
	}
	int Find(const std::string& path);

protected:
	void ReadAssetsDirectory(std::wstring fileSpec);

};


class ResHandle
{
	friend class ResCache;

protected:
	Resource m_Resource;
	char* m_Buffer;
	unsigned int m_Size;
	shared_ptr<IResourceExtraData> m_Extra;
	ResCache* m_pResCache;

public:
	ResHandle(Resource& resource, char* buffer, unsigned int size, ResCache* pResCache);
	virtual ~ResHandle();

	const std::string GetName() const { return m_Resource.m_Name; }
	unsigned int Size() const { return m_Size; }
	char* Buffer() const { return m_Buffer; }
	char* WritableBuffer() const { return m_Buffer; }
	shared_ptr<IResourceExtraData> GetExtra() const { return m_Extra; }
	void SetExtra(shared_ptr<IResourceExtraData> extra) { m_Extra = extra; }

};

class DefaultResourceLoader : public IResourceLoader
{
public:
	virtual bool VUseRawFile() { return true;}
	virtual bool VDiscardRawBufferAfterLoad() { return true; }
	virtual unsigned int VGetLoadedResourceSize(char* rawBuffer, unsigned int rawSize) { return rawSize; }
	virtual bool VLoadResource(char* rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle) { return true; }
	virtual std::string VGetPattern() { return "*"; }

};



typedef std::list<shared_ptr<ResHandle>> ResHandleList;
typedef std::map<std::string, shared_ptr<ResHandle>> ResHandleMap;
typedef std::list<shared_ptr<IResourceLoader>> ResourceLoaders;
class ResCache
{
	friend class ResHandle;

	ResHandleList m_LRU;
	ResHandleMap m_Resources;
	ResourceLoaders m_ResourceLoaders;
	IResourceFile* m_File;
	unsigned int m_CacheSize;
	unsigned int m_Allocated;

public:
	ResCache(const unsigned int sizeInMb, IResourceFile* file);
	virtual ~ResCache();

	bool Init();
	void RegisterLoader(shared_ptr<IResourceLoader> loader);
	shared_ptr<ResHandle> GetHandle(Resource* r);
	int Preload(const std::string pattern, void (*progressCallBcack)(int, bool&));
	std::vector<std::string> Match(const std::string pattern);
	void Flush();
	bool IsUsingDevelopmentDirectories() const
	{
		ASSERT(m_File);
		return m_File->VIsUsingDevelopmentDirectories();
	}

protected:
	bool MakeRoom(unsigned int size);
	char* Allocate(unsigned int size);
	void Free(shared_ptr<ResHandle> resHandle);
	shared_ptr<ResHandle> Load(Resource* r);
	shared_ptr<ResHandle> Find(Resource* r);
	void Update(shared_ptr<ResHandle> handle);
	void FreeOneResource();
	void MemoryHasBeenFreed(unsigned int size);
};


class XmlResourceExtraData : public IResourceExtraData
{
	TiXmlDocument m_XmlDocument;

public:
	virtual std::string VToString() { return "XmlResourceExtraData"; }
	void ParseXml(char* pRawBuffer);
	TiXmlElement* GetRoot(void) { return m_XmlDocument.RootElement(); }
};


class XmlResourceLoader : public IResourceLoader
{
public:
	virtual bool VUseRawFile() { return false; }
	virtual bool VDiscardRawBufferAfterLoad() { return true; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) { return rawSize; }
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle);
	virtual std::string VGetPattern() { return "*.xml"; }

	
	static TiXmlElement* LoadAndReturnRootXmlElement(const char* resourceString);
};