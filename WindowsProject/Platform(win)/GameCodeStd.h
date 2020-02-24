#pragma once
#define WIN32_LEAN_AND_MEAN

#define NOMINMAX				//Windows Header Files
#include <windows.h>
#include <windowsx.h>
#include <crtdbg.h>

#if _MSC_VER < 1400
#define _VS2005_
#endif

#include <stdlib.h>		//C RunTime Header Files
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <mmsystem.h> //Get Color ID

#include <algorithm>
#include <string>
#include <list>
#include <vector>
#include <queue>
#include <map>

#ifdef _VS2005_
	#include <memory>
	using std::shared_ptr;
	using std::weak_ptr;
	using std::static_pointer_cast;
	using std::dynamic_pointer_cast;
#endif // _VS2005_

#ifndef _VS2005_
	#include <memory>
	using std::tr1::shared_ptr;
	using std::tr1::weak_ptr;
	using std::tr1::static_pointer_cast;
	using std::tr1::dynamic_pointer_cast;
#endif

	class Noncopyable
	{
	public:
		Noncopyable() {};

	private:
		Noncopyable(const Noncopyable &x);
		Noncopyable& operator=(const Noncopyable &x) {};
	};



#if defined(_DEBUG)
#	define New new(_NORMAL_BLOCK,__FILE__,__LINE__)		//Tracking Memory Allocation
#else 
#	define New new
#endif // defined(_DEBUG)

#define DXUT_AUTOLIB

#include <dxut.h>			//DirectX Header Files
#include <d3dx9tex.h>
#include <SDKmisc.h>

#include <tinyxml.h>

#include "3rdParty/Source/GCC4/3rdParty/FastDelegate/FastDelegate.h"	//Convinient Delegate Functions
	using fastdelegate::MakeDelegate;

#pragma warning(disable:4996)		//Disable Deprecated Function Warning


#include"Debugging/Logger.h"
#include"Utilities/types.h"
#include"Utilities/templates.h"
#include"Graphic3D/geometry.h"
#include"GameCode\interface.h"
#include"Actors\Actor.h"
#include"Utilities\Math.h"
#include"Graphic3D\Shaders.h"
#include"GameCode\BaseGameLogic.h"
#include"MainLoop\Initialization.h"
#include"MainLoop\ProcessManager.h"

typedef D3DXCOLOR Color;

//Alpha values used in color
const float fOPAQUE = 1.0f;
const int iOPAQUE = 1;
const float fTRANSPARENT = 0.0f;
const int iTRANSPARENT = 0;

//Global direction vectors
Vec3 g_Right(1.0f, 0.0f, 0.0f);
Vec3 g_Up(0.0f, 1.0f, 0.0f);
Vec3 g_Forward(0.0f, 0.0f, 1.0f);

Vec4 g_Right4(1.0f, 0.0f, 0.0f, 0.0f);
Vec4 g_Up4(0.0f, 1.0f, 0.0f, 0.0f);
Vec4 g_Forward4(0.0f, 0.0f, 1.0f, 0.0f);

//Global color variables
Color g_White(1.0f, 1.0f, 1.0f, fOPAQUE);
Color g_Black(0.0f, 0.0f, 0.0f, fOPAQUE);
Color g_Cyan(0.0f, 1.0f, 1.0f, fOPAQUE);
Color g_Red(1.0f, 0.0f, 0.0f, fOPAQUE);
Color g_Green(0.0f, 1.0f, 0.0f, fOPAQUE);
Color g_Blue(0.0f, 0.0f, 1.0f, fOPAQUE);
Color g_Yellow(1.0f, 1.0f, 0.0f, fOPAQUE);
Color g_Gray40(0.4f, 0.4f, 0.4f, fOPAQUE);
Color g_Gray25(0.25f, 0.25f, 0.25f, fOPAQUE);
Color g_Gray65(0.65f, 0.65f, 0.65f, fOPAQUE);
Color g_Transparent(1.0f, 0.0f, 1.0f, fTRANSPARENT);

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

const int MEGABYTE = 1024 * 1024;
const float SIXTY_HERTZ = 16.66f;

struct AppMsg
{
	HWND m_hWnd;
	UINT m_uMsg;
	WPARAM m_wParam;
	LPARAM m_lParam;
};


#ifndef SAFE_DELETE
	#define SAFE_DELETE(x) if(x) deletex; x=NULL;
#endif // !SAFE_DELETE

#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(x) if(x) delete[] x; x=NULL;
#endif // !SAFE_DELETE_ARRAY

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(x) if(x) x->Release(); x=NULL;
#endif // !SAFE_RELEASE

#ifdef  UNICODE
	#define _tcssprintf wsprintf
	#define tcsplitpath _wsplitpath
#else
	#define _tcssprintf sprintf
	#define tcsplitpath _splitpath 
#endif //  UNICODE

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)"): Warning Msg:"


#include "GameCode\GameCode.h"

extern INT WINAPI GameCode4(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nCmdShow);




//Define MEM_LOG to log "new" to debug window
//#define MEM_LOG
#if defined(_DEBUG) && defined(MEM_LOG)

void* operator new(size_t size, int memType, const char* filename, int lineNum)
{
	// We have to do this old-school since we're not allowed to dynamically allocate memory here.
	char buffer[2048];
	int index = 0;
	index += strlen(ultoa(size, buffer, 10));
	strcpy(buffer + index, " -> ");
	index += 4;
	strcpy(buffer + index, filename);
	index += strlen(filename);
	buffer[index] = ':';
	++index;
	index += strlen(itoa(lineNum, buffer + index, 10));
	buffer[index] = '\n';
	++index;
	buffer[index] = '\0';
	++index;
	OutputDebugStringA(buffer);

	return _malloc_dbg(size, memType, filename, lineNum);
}

void operator delete(void* pMemory)
{
	_free_dbg(pMemory, 1);
}

void* operator new[](size_t size, int memType, const char* filename, int lineNum)
{
	// We have to do this old-school since we're not allowed to dynamically allocate memory here.
	char buffer[2048];
	int index = 0;
	index += strlen(ultoa(size, buffer, 10));
	strcpy(buffer + index, " -> ");
	index += 4;
	strcpy(buffer + index, filename);
	index += strlen(filename);
	buffer[index] = ':';
	++index;
	index += strlen(itoa(lineNum, buffer + index, 10));
	buffer[index] = '\n';
	++index;
	buffer[index] = '\0';
	++index;
	OutputDebugStringA(buffer);

	return _malloc_dbg(size, 1, filename, lineNum);
}

void operator delete[](void* pMemory)
{
	_free_dbg(pMemory, 1);
}

#endif
