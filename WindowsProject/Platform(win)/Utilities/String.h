#pragma once

//Define some useful string utility functions

#include<vector>
#include<minwindef.h>
#include<winnt.h>
#include"Graphic3D\geometry.h"


#define MAX_DIGITS_IN_INT 12
typedef std::vector<std::string> StringVec;

void RemoveFirstLine(std::wstring& src, std::wstring& result);

void TrimLeft(std::wstring& s);

int CountLines(const std::wstring& s);

BOOL WildcardMatch(const char* pat, const char* str);

std::wstring StringToWideString(const std::string& src, std::wstring& out);
std::string WideStringToString(const std::wstring& src, std::string& out);


HRESULT AnsiToWideCch(WCHAR* out, const CHAR* src, int charCount);
HRESULT WideToAnsiCch(CHAR* out, const WCHAR* src, int charCount);
HRESULT GenericToAnsiCch(CHAR* out, const TCHAR* src, int charCount);
HRESULT GenericToWideCch(WCHAR* out, const TCHAR* src, int charCount);
HRESULT AnsiToGenericCch(TCHAR* out, const CHAR* src, int charCount);
HRESULT WideToGenericCch(TCHAR* out, const WCHAR* src, int charCount);

std::string ToStr(int num, int base = 10);
std::string ToStr(unsigned int num, int base = 10);
std::string ToStr(unsigned long num, int base = 10);
std::string ToStr(float num);
std::string ToStr(double num);
std::string ToStr(bool val);
std::string ToStr(const Vec3& vec);

// Splits a string by the delimeter into a vector of strings.  For example, say you have the following string:
// std::string test("one,two,three");
// You could call Split() like this:
// Split(test, outVec, ',');
// outVec will have the following values:
// "one", "two", "three"

void Split(const std::string& str, StringVec& vec, char delimeter);

#pragma warning(push)
#pragma warning(disable: 4311)

class HashedString
{
private:

	// note: m_ident is stored as a void* not an int, so that in
	// the debugger it will show up as hex-values instead of
	// integer values. This is a bit more representative of what
	// we're doing here and makes it easy to allow external code
	// to assign event types as desired.
	void* m_ident;
	std::string m_identStr;

public:

	explicit HashedString(const char* const pidentString) :m_ident(HashName(pidentString)), m_identStr(pidentString){}
	
	unsigned long GetHashValue() const
	{
		return reinterpret_cast<unsigned long>(m_ident);
	}

	const std::string& GetStr() const
	{
		return m_identStr;
	}

	static void* HashName(const char* pidentStr);

	bool operator<(const HashedString& other) const
	{
		bool r = (GetHashValue() < other.GetHashValue());
		return r;
	}

	bool operator==(const HashedString& other) const
	{
		bool r = (GetHashValue() == other.GetHashValue());
		return r;
	}
};
#pragma warning(pop)
