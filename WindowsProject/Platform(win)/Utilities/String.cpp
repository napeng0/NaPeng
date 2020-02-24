#include"String.h"
#include<tchar.h>
#include"GameCodeStd.h"

using std::string;

void RemoveFirstLine(std::wstring &src, std::wstring &result)
{
	int breakPosition = (int)src.find('\n');
	result = _T("");
	if (breakPosition != src.npos)	//if found...
	{
		int len = (int)src.length();
		result = src.substr(0, breakPosition);
		src = src.substr(breakPosition + 1, (len - breakPosition) - 1);		// skip the '/n'
	}
	else
	{
		result = src;
		src = _T("");
	}
}

int IsSpace(TCHAR ch)
{
#ifdef UNICODE
	return iswspace(ch);
#else
	return isspace(ch);
#endif // UNICODE

}

void TrimLeft(std::wstring& str)
{
	//find the first non-space char
	int i = 0;
	int len = (int)str.length;
	while (i < len)
	{
		TCHAR ch = str[i];
		int white = IsSpace(ch);
		if (!white)	break;
		++i;
	}
	if (i < len)
		str = str.substr(i);
}

int CountLines(const std::wstring& str)
{
	int lines = 0;
	int breakpos = 0;
	do {
		++lines;
		breakpos = (int)str.find('\n', breakpos + 1);
	} while (breakpos != str.npos);
}

//The following function was found on http ://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html, where it was attributed to 
// the C/C++ Users Journal, written by Mike Cornelison. It is a little ugly, but it is FAST. Use this as an excercise in not reinventing the
// wheel, even if you see gotos.


BOOL WildcardMatch(const char *pat, const char *str) {
	int i, star;

new_segment:

	star = 0;
	if (*pat == '*') {
		star = 1;
		do { pat++; } while (*pat == '*'); /* enddo */
	} /* endif */

test_match:

	for (i = 0; pat[i] && (pat[i] != '*'); i++) {
		//if (mapCaseTable[str[i]] != mapCaseTable[pat[i]]) {
		if (str[i] != pat[i]) {
			if (!str[i]) return 0;
			if ((pat[i] == '?') && (str[i] != '.')) continue;
			if (!star) return 0;
			str++;
			goto test_match;
		}
	}
	if (pat[i] == '*') {
		str += i;
		pat += i;
		goto new_segment;
	}
	if (!str[i]) return 1;
	if (i && pat[i - 1] == '*') return 1;
	if (!star) return 0;
	str++;
	goto test_match;
}

HRESULT AnsiToWideCch(WCHAR* pdest, const CHAR* psrc, int destsize)
{
	if (pdest == NULL || psrc == NULL || destsize < 1)	return E_INVALIDARG;	
	int nResult = MultiByteToWideChar(CP_ACP, 0, psrc, -1, pdest, destsize);
	pdest[destsize - 1] = L'\0';
	if (nResult == 0)	return E_FAIL;
	return S_OK;

}

HRESULT GenericToAnsiCch(CHAR* pdest, const TCHAR* psrc, int destsize)
{
	if (pdest == NULL || psrc == NULL || destsize < 1)	return E_INVALIDARG;

#ifdef UNICODE
	return WideToAnsiCch(pdest, psrc, destsize);
#else
	strncpy(pdest, psrc, destsize);
	pdest[destsize - 1] = '\0';
	return S_OK;
#endif
}

HRESULT GenericToWideCch(WCHAR* pdest, const TCHAR* psrc, int destsize)
{
	if (pdest == NULL || psrc == NULL || destsize < 1)	return E_INVALIDARG;

#ifdef UNICODE
	wcsncpy(pdest, psrc, destsize);
	pdest[destsize - 1] = L'\0';
	return S_OK;
#else
	return AnsiToWideCch(pdest, psrc, destsize);
#endif
}

HRESULT AnsiToGenericCch(TCHAR* pdest, const CHAR* psrc, int destsize)
{
	if (pdest == NULL || psrc == NULL || destsize < 1)	return E_INVALIDARG;

#ifdef UNICODE
	return AnsiToWideCch(pdest, psrc, destsize);
#else
	strncpy(pdest, psrc, destsize);
	pdest[destsize - 1] = '\0';
	return S_OK;
#endif // 
}

HRESULT WideToGenericCch(TCHAR* pdest, const WCHAR* psrc, int destsize)
{
	if (pdest == NULL || psrc == NULL || destsize < 1)	return E_INVALIDARG;

#ifdef UNICODE
	wcsncpy(pdest, psrc, destsize);
	pdest[destsize - 1] = L'\0';
	return S_OK;
#else
	return WideToAnsiCch(pdest, psrc, destsize);
#endif
}

std::string WideStringToString(const std::wstring& str)
{
	int srclen = (int)str.length() + 1;
	int len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), srclen, 0, 0, 0, 0) - 1;
	std::string deststr(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, str.c_str(), srclen, &deststr[0], len, 0, 0);
	return deststr;
}

std::wstring StringToWideString(const std::string& str)
{
	int srclen = (int)str.length() + 1;
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), srclen, 0, 0)-1;
	std::wstring deststr(len, '\0');
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), srclen, &deststr[0], len);
	return deststr;
}

std::string ToStr(int num, int base)
{
	char str[MAX_DIGITS_IN_INT];
	memset(str, 0, MAX_DIGITS_IN_INT);
	_itoa_s(num, str, MAX_DIGITS_IN_INT, base);
	return (std::string(str));
}

std::string ToStr(unsigned int num, int base)
{
	char str[MAX_DIGITS_IN_INT];
	memset(str, 0, MAX_DIGITS_IN_INT);
	_ultoa_s((unsigned long)num, str, MAX_DIGITS_IN_INT, base);
	return(std::string(str));
}

std::string ToStr(unsigned int num, int base)
{
		char str[MAX_DIGITS_IN_INT];
		memset(str, 0, MAX_DIGITS_IN_INT);
		_ultoa_s(num, str, MAX_DIGITS_IN_INT, base);
		return(std::string(str));
}

std::string ToStr(unsigned int num, int base)
{
		char str[MAX_DIGITS_IN_INT];
		memset(str, 0, MAX_DIGITS_IN_INT);
		_ultoa_s(num, str, MAX_DIGITS_IN_INT, base);
		return(std::string(str));
}

std::string ToStr(unsigned int num, int base)
{
	char str[MAX_DIGITS_IN_INT];
	memset(str, 0, MAX_DIGITS_IN_INT);
	_ultoa_s(num, str, MAX_DIGITS_IN_INT, base);
	return(std::string(str));
}

std::string ToStr(float num)
{
	char str[64];  // I'm sure this is overkill
	memset(str, 0, 64);
	_sprintf_p(str, 64, "%f", num);
	return (string(str));
}

std::string ToStr(double num)
{
	char str[64];  // I'm sure this is overkill
	memset(str, 0, 64);
	_sprintf_p(str, 64, "%fL", num);
	return (string(str));
}

std::string ToStr(bool val)
{
	return (string(val == true ? "true" : "false"));
}

std::string ToStr(const Vec3& vec)
{
	return std::string("(" + ToStr(vec.x) + "," + ToStr(vec.y) + "," + ToStr(vec.z) + ")");
}

//Split a string by delimiter and store splited strings into stringvector
void Split(const string& str, StringVec& vec, char delimiter)
{
	vec.clear();
	size_t strlen = str.size();
	if (strlen == 0) return;
	size_t startIndex = 0;
	size_t indexOfDel = str.find_first_of(delimiter, startIndex);
	while (indexOfDel != str.npos)
	{
		vec.push_back(str.substr(startIndex, indexOfDel - startIndex));
		startIndex = indexOfDel + 1;
		if (startIndex >= strlen)	break;
		indexOfDel = str.find_first_of(delimiter, startIndex);
	}
	if (startIndex < strlen)
		vec.push_back(str.substr(startIndex));
}


//This function is based upon the adler32 checksum by Mark Adler
//Hash of random string into a 32 bits indentifier output value,
//Input value is treated as lower-case to reduce the effct of human mistypes.
//This function is case-insensitive
void* HashedString::HashName(const char* pidentStr)
{
	//largest prime smaller than 65535
	unsigned long base = 65521L;
	//largest n such that [255n(n+1)/2+(n+1)(base-1)]<=2^32-1
	unsigned long nmax = 5552;

#define DO1(buf, i) {s1+=tolower(buf[i]); s2+=s1;}
#define DO2(buf, i) {DO1(buf, i); DO1(buf, i+1);}
#define DO4(buf, i) {DO2(buf, i); DO2(buf, i+2);}
#define DO8(buf, i) {DO4(buf, i); DO4(buf, i+4);}
#define DO16(buf, i) {DO8(buf, i); DO8(buf, i+8);}

	if (pidentStr==NULL)
	{
		return NULL;
	}

	unsigned long s1 = 0;
	unsigned long s2 = 0;

	for (size_t len = strlen(pidentStr); len > 0;)
	{
		unsigned long k = len < nmax ? len : nmax;
		int count = 0;

		len -= k;

		while (k >= 16)
		{
			DO16(pidentStr, count);
			count += 16;
			k -= 16;
		}

		while(k!=0)
		{
			DO1(pidentStr, count);
			++count;
			--k;
		}

		s1 %= base;
		s2 %= base;

	}

#pragma warning(push)
#pragma warning(disable: 4312)

	return reinterpret_cast<void*>((s2 << 16) | s1);

#pragma warning(pop)
#undef DO1
#undef DO2
#undef DO4
#undef DO8
#undef DO16
}

