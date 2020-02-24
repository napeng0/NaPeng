#pragma once
#include <string>

const unsigned char LOGFLAG_WRITE_TO_LOG_FILE = 1;		//Display flags
const unsigned char LOGFLAG_WRITE_TO_DEBUGGER = 1 << 1;

namespace Logger
{
	class ErrorMessenger
	{
	public:
		bool m_enabled;
	
	public:
		ErrorMessenger(void);
		void Show(const std::string &errorMessage, bool isFatal, const char* funcName, const char* sourceFile, unsigned int lineNum);


	private:

	};
	void Init(const char* loggingConfigFileName);
	void Destroy(void);

	//Logging functions
	void Log(const std::string& tag, const std::string& message, const char* funcName, const char* sourceFile, unsigned int lineNum);
	void SetDisplayFlags(const std::string& tag, unsigned char flags);
}

//Debug macros
#define FATAL(str)\
	do\
	{\
	static Logger::ErrorMessenger* pErrorMessenger=New Logger::ErrorMessenger;\
	std::string s((str));\
	pErrorMessenger->Show(s, true, __FUNCTION__, __FILE__, __LINE__);\
	}\
	while(0)\

#ifndef NDEBUG

//Displaying ERROR messages
#define ERROR(str)\
	do\
	{\
	static Logger::ErrorMessenger* pErrorMessenger= New Logger::ErrorMessenger;\
	std::string s((str));\
	pErrorMessenger->Show(s, false, __FUNCTION__, __FILE__, __LINE__);\
	}while (0)\

//Displaying WARNING messages
#define WARNING(str)\
	do\
	{\
	std::string s((str));\
	Logger::Log("WARNING", s, __FUNCTION__, __FILE__, __LINE__);\
	}while(0)\

//Convenient macro calling for Log()with default tag of INFO and default flag of LOGFLAG_DEFAULT
#define INFO(str)\
	do\
	{\
	std::string s((str));\
	Logger::Log("INFO", s, NULL, NULL, 0);\
	}while (0)\

//Macro uesd for logging
#define LOG(tag,str)\
	do\
	{\
	std::string s((str));\
	Logger::Log(tag, s, NULL, NULL, 0);\
	}while (0)\

//Macro replaces ASSERT()
#define ASSERT(expr)\
	do\
	{\
	if(!(expr))\
		{\
		static Logger::ErrorMessenger* pErrorMessenger= New Logger::ErrorMessenger;\
		pErrorMessenger->Show(#expr, false, __FUNCTION__, __FILE__, __LINE__);\
		}\
	}while(0)\

#else	//NDEBUG is defined

//Release mode definition for the macros above
#define ERROR(str) do{(void)sizeof(str); }while(0)
#define WARNING(str) do{(void)sizeof(str); }while(0)
#define INFO(str) do({(void)sizeof(str); while(0)
#define LOG(tag,str) do{(void)sizeof(str); }while(0);
#define ASSERT(expr) do{(void)sizeof(expr); }while(0);

#endif // !NDEBUG

