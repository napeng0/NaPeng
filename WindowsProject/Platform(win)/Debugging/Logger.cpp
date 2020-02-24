#pragma once
#include "Debugging/Logger.h"
#include "GameCodeStd.h"
#include "MulitiCore/CriticalSection.h"
#include "Utilities/String.h"


using std::string;

#pragma region Constant globals definition
//Log filename
static const char* ERRORLOG_FILENAME = "error.log";

//Default display flags
#ifdef _DEBUG
const unsigned char ERRORFLAG_DEFAULT = LOGFLAG_WRITE_TO_DEBUGGER;
const unsigned char WARNINGFLAG_DEFAULT = LOGFLAG_WRITE_TO_DEBUGGER;
const unsigned char LOGFLAG_DEFAULT = LOGFLAG_WRITE_TO_DEBUGGER;
#elif NDEBUG
const unsigned char ERRORFLAG_DEFAULT = LOGFLAG_WRITE_TO_DEBUGGER | LOGFLAG_WRITE_TO_LOG_FILE;
const unsigned char WARNINGFLAG_DEFAULT= LOGFLAG_WRITE_TO_DEBUGGER | LOGFLAG_WRITE_TO_LOG_FILE;
const unsigned char LOGFLAG_DEFAULT= LOGFLAG_WRITE_TO_DEBUGGER | LOGFLAG_WRITE_TO_LOG_FILE;
#else
const unsigned char ERRORFLAG_DEFAULT = 0;
const unsigned char WARNINGFLAG_DEFAULT = 0;
const unsigned char LOGFLAG_DEFAULT = 0;
#endif

//singleton
class LogManager;
static LogManager* s_pLogMgr = NULL;
#pragma endregion

#pragma region LogMessenger class
class LogManager
{
public:
	enum ErrorDialogResult
	{
		LOGMGR_ERROR_ABORT,
		LOGMGR_ERROR_RETRY,
		LOGMGR_ERROR_IGNORE
	};

	typedef std::map<string, unsigned char> Tags;
	typedef std::list<Logger::ErrorMessenger*> ErrorMessengerList;

	Tags m_tags;
	ErrorMessengerList m_errorMessengers;

	//Thread safety
	CriticalSection m_tagCriticalSection;
	CriticalSection m_messengerCriticalSection;

	//Constructor
	LogManager();
	~LogManager();
	
	//Initializer
	void Init(const char* loggingConfigFilename);

	//Logs
	void Log(const string& tag, const string& message, const char* funcName, const char* sourceFile, unsigned int lineNum);
	void SetDisplayFlags(const string& tag, unsigned char flags);

	//Error messengers
	void AddErrorMessenger(Logger::ErrorMessenger* pmessenger);
	ErrorDialogResult Error(const string& errorMessage, bool isFatal, const char* funcName, const char* sourceFile, unsigned int lineNum);

private:
	//log helpers
	void OutputFinalBufferToLogs(const string& finalBuffer, unsigned char flags);
	void WriteToLogFile(const string& data);
	void GetOutputBuffer(string& outputBuffer, const string& tag, const string& message, const char* funcName, const char* sourceFile, unsigned int lineNum);

};
#pragma endregion

#pragma region LogMessenger methods definition
//Constructor definition
LogManager::LogManager()
{
	SetDisplayFlags("ERROR", ERRORFLAG_DEFAULT);
	SetDisplayFlags("WARNING", WARNINGFLAG_DEFAULT);
	SetDisplayFlags("INFO", LOGFLAG_DEFAULT);
}

//Destructor
LogManager::~LogManager()
{
	m_messengerCriticalSection.Lock();
	for (ErrorMessengerList::iterator it = m_errorMessengers.begin(); it != m_errorMessengers.end(); ++it)
	{
		Logger::ErrorMessenger* pmessenger = *it;
		delete pmessenger;
	}
	m_errorMessengers.clear();
	m_messengerCriticalSection.Unlock;
}

//Initializer
void LogManager::Init(const char* loggingConfigFilename)
{
	if (loggingConfigFilename)
	{
		TiXmlDocument loggingConfigFile(loggingConfigFilename);
		if (loggingConfigFile.LoadFile())
		{
			TiXmlElement* pRoot = loggingConfigFile.RootElement();
			if (!pRoot)
				return;

			//Load every child component
			for (TiXmlElement* pNode = pRoot->FirstChildElement(); pNode; pNode->NextSiblingElement())
			{
				unsigned char flags = 0;
				string tag(pNode->Attribute("tag"));

				int debugger = 0;
				pNode->Attribute("debugger", &debugger);
				if (debugger)
					flags |= LOGFLAG_WRITE_TO_DEBUGGER;

				int logfile = 0;
				pNode->Attribute("file", &logfile);
				if (logfile)
					flags |= LOGFLAG_WRITE_TO_LOG_FILE;

				SetDisplayFlags(tag, flags);

			}
		}
	}
}

//This function builds up the log string and outputs it to various
//places based on display flags
void LogManager::Log(const string& tag, const string& message, const char* funcName, const char* sourceFile, unsigned int lineNum)
{
	m_tagCriticalSection.Lock();
	Tags::iterator it = m_tags.find(tag);
	if (it != m_tags.end())
	{
		m_tagCriticalSection.Unlock();

		string buffer;
		GetOutputBuffer(buffer, tag, message, funcName, sourceFile, lineNum);
		OutputFinalBufferToLogs(buffer, it->second);
	}
	else
	{
		m_tagCriticalSection.Unlock();
	}
}


void LogManager::SetDisplayFlags(const string& tag, unsigned char flags)
{
	m_tagCriticalSection.Lock();
	if (flags != 0)
	{
		Tags::iterator it = m_tags.find(tag);
		if (it == m_tags.end())
			m_tags.insert(std::make_pair(tag, flags));
		else
			it->second = flags;
	}
	else
	{
		m_tags.erase(tag);
	}
	m_tagCriticalSection.Unlock();
}


void LogManager::AddErrorMessenger(Logger::ErrorMessenger* pmessenger)
{
	m_messengerCriticalSection.Lock();
	m_errorMessengers.push_back(pmessenger);
	m_messengerCriticalSection.Unlock();
}


LogManager::ErrorDialogResult LogManager::Error(const string& errorMessage, bool isFatal, const char* funcName, const char* sourceFile, unsigned int lineNum)
{
	string tag = isFatal ? "FATAL" : "ERROR";

	string buffer;
	GetOutputBuffer(buffer, tag, errorMessage, funcName, sourceFile, lineNum);
	m_tagCriticalSection.Lock();
	Tags::iterator it = m_tags.find(tag);
	if (it != m_tags.end())
		OutputFinalBufferToLogs(buffer, it->second);
	m_tagCriticalSection.Unlock();

	int result = MessageBoxA(NULL, buffer.c_str(), tag.c_str(), MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_DEFBUTTON3);

	switch (result)
	{
		case IDIGNORE: return LOGMGR_ERROR_IGNORE;
		case IDABORT: __debugbreak(); return LogManager::LOGMGR_ERROR_ABORT;
		case IDRETRY: return LogManager::LOGMGR_ERROR_RETRY;
		default:	  return LogManager::LOGMGR_ERROR_RETRY;
	}
}


//This function outputs the passed in buffer to places based on display flags
void LogManager::OutputFinalBufferToLogs(const string& finalBuffer, unsigned char flags)
{
	if (flags& LOGFLAG_WRITE_TO_LOG_FILE)
		WriteToLogFile(finalBuffer);
	if (flags& LOGFLAG_WRITE_TO_DEBUGGER)
		OutputDebugStringA(finalBuffer.c_str());
}

void LogManager::WriteToLogFile(const string& data)
{
	FILE* pLogFile = NULL;
	fopen_s(&pLogFile, ERRORLOG_FILENAME, "a+");
	if (!pLogFile)
		return;
	fprintf_s(pLogFile, data.c_str());

	fclose(pLogFile);
}

void LogManager::GetOutputBuffer(string& buffer, const string& tag, const string& message, const char* funcName, const char* sourceFile, unsigned int lineNum)
{
	if (!tag.empty())
		buffer = "[" + tag + "]" + message;
	else
		buffer = message;

	if (funcName != NULL)
	{
		buffer += "\nFunction: ";
		buffer += funcName;
	}

	if (sourceFile != NULL)
	{
		buffer += "\n";
		buffer += sourceFile;
	}

	if (lineNum != 0)
	{
		buffer += "\nLine: ";
		char lineNumBuffer[16];
		memset(lineNumBuffer, 0, 16*sizeof(char));
		buffer += _itoa(lineNum, lineNumBuffer, 10);
	}

	buffer += "\n";

}
#pragma endregion

#pragma region ErrorMessenger definition
Logger::ErrorMessenger::ErrorMessenger()
{
	s_pLogMgr->AddErrorMessenger(this);
	m_enabled = true;
}

void Logger::ErrorMessenger::Show(const string& errorMessage, bool isFatal, const char* funcName, const char* sourceFile, unsigned int lineNum)
{
	if (m_enabled)
	{
		if (s_pLogMgr->Error(errorMessage, isFatal, funcName, sourceFile, lineNum) == LogManager::LOGMGR_ERROR_ABORT)
			m_enabled = false;
	}
}
#pragma endregion

#pragma region C interface
namespace Logger
{
	void init(const char* loggingConfigFilename)
	{
		if (!s_pLogMgr)
		{
			s_pLogMgr = New LogManager;
			s_pLogMgr->Init(loggingConfigFilename);
		}
	}


	void Destroy()
	{
		delete s_pLogMgr;
		s_pLogMgr = NULL;
	}

	void Log(const string& tag, const string& message, const char* funcName, const char* sourceFile, unsigned int lineNum)
	{
		ASSERT(s_pLogMgr);
		s_pLogMgr->Log(tag, message, funcName, sourceFile, lineNum);
	}

	void SetDiplayFlags(const std::string& tag, unsigned char flags)
	{
		ASSERT(s_pLogMgr);
		s_pLogMgr->SetDisplayFlags(tag, flags);
	}
}
#pragma endregion