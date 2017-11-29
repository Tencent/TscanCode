/******************************************************************************
Module:  APIHook.cpp
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/
#include <Windows.h>
#include <tchar.h>
#include <StrSafe.h>
#include <DbgHelp.h>
#include <tlhelp32.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include "CrashHelp.h"
#pragma comment(lib, "DbgHelp.lib")


//def of common header
/******************************************************************************
Module:  CmnHdr.h
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
Purpose: Common header file containing handy macros and definitions
used throughout all the applications in the book.
See Appendix A.
******************************************************************************/


#pragma once   // Include this header file once per compilation unit



//////////////////////////// Unicode Build Option /////////////////////////////


// Always compiler using Unicode.
#ifndef UNICODE
#define UNICODE
#endif

// When using Unicode Windows functions, use Unicode C-Runtime functions too.
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif


///////////////////////// Include Windows Definitions /////////////////////////

#pragma warning(push, 3)
#include <Windows.h>
#pragma warning(pop) 
#pragma warning(push, 4)
#include <CommCtrl.h>
#include <process.h>       // For _beginthreadex


///////////// Verify that the proper header files are being used //////////////


#ifndef FILE_SKIP_COMPLETION_PORT_ON_SUCCESS
#pragma message("You are not using the latest Platform SDK header/library ")
#pragma message("files. This may prevent the project from building correctly.")
#endif


////////////// Allow code to compile cleanly at warning level 4 ///////////////


/* nonstandard extension 'single line comment' was used */
#pragma warning(disable:4001)

// unreferenced formal parameter
#pragma warning(disable:4100)

// Note: Creating precompiled header 
#pragma warning(disable:4699)

// function not inlined
#pragma warning(disable:4710)

// unreferenced inline function has been removed
#pragma warning(disable:4514)

// assignment operator could not be generated
#pragma warning(disable:4512)

// conversion from 'LONGLONG' to 'ULONGLONG', signed/unsigned mismatch
#pragma warning(disable:4245)

// 'type cast' : conversion from 'LONG' to 'HINSTANCE' of greater size
#pragma warning(disable:4312)

// 'argument' : conversion from 'LPARAM' to 'LONG', possible loss of data
#pragma warning(disable:4244)

// 'wsprintf': name was marked as #pragma deprecated
#pragma warning(disable:4995)

// unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable:4146)

// 'argument' : conversion from 'size_t' to 'int', possible loss of data
#pragma warning(disable:4267)

// nonstandard extension used : nameless struct/union
#pragma warning(disable:4201)

///////////////////////// Pragma message helper macro /////////////////////////


/*
When the compiler sees a line like this:
#pragma chMSG(Fix this later)

it outputs a line like this:

c:\CD\CmnHdr.h(82):Fix this later

You can easily jump directly to this line and examine the surrounding code.
*/

#define chSTR2(x) #x
#define chSTR(x)  chSTR2(x)
#define chMSG(desc) message(__FILE__ "(" chSTR(__LINE__) "):" #desc)


////////////////////////////// chINRANGE Macro ////////////////////////////////


// This macro returns TRUE if a number is between two others
#define chINRANGE(low, Num, High) (((low) <= (Num)) && ((Num) <= (High)))



///////////////////////////// chSIZEOFSTRING Macro ////////////////////////////


// This macro evaluates to the number of bytes needed by a string.
#define chSIZEOFSTRING(psz)   ((lstrlen(psz) + 1) * sizeof(TCHAR))



/////////////////// chROUNDDOWN & chROUNDUP inline functions //////////////////


// This inline function rounds a value down to the nearest multiple
template <class TV, class TM>
inline TV chROUNDDOWN(TV Value, TM Multiple) {
	return((Value / Multiple) * Multiple);
}


// This inline function rounds a value down to the nearest multiple
template <class TV, class TM>
inline TV chROUNDUP(TV Value, TM Multiple) {
	return(chROUNDDOWN(Value, Multiple) +
		(((Value % Multiple) > 0) ? Multiple : 0));
}



///////////////////////////// chBEGINTHREADEX Macro ///////////////////////////


// This macro function calls the C runtime's _beginthreadex function. 
// The C runtime library doesn't want to have any reliance on Windows' data 
// types such as HANDLE. This means that a Windows programmer needs to cast
// values when using _beginthreadex. Since this is terribly inconvenient, 
// I created this macro to perform the casting.
typedef unsigned(__stdcall *PTHREAD_START) (void *);

#define chBEGINTHREADEX(psa, cbStackSize, pfnStartAddr, \
   pvParam, dwCreateFlags, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                          \
         (void *)        (psa),                         \
         (unsigned)      (cbStackSize),                 \
         (PTHREAD_START) (pfnStartAddr),                \
         (void *)        (pvParam),                     \
         (unsigned)      (dwCreateFlags),               \
         (unsigned *)    (pdwThreadId)))


////////////////// DebugBreak Improvement for x86 platforms ///////////////////


#ifdef _X86_
#define DebugBreak()    _asm { int 3 }
#endif


/////////////////////////// Software Exception Macro //////////////////////////


// Useful macro for creating your own software exception codes
#define MAKESOFTWAREEXCEPTION(Severity, Facility, Exception) \
   ((DWORD) ( \
   /* Severity code    */  (Severity       ) |     \
   /* MS(0) or Cust(1) */  (1         << 29) |     \
   /* Reserved(0)      */  (0         << 28) |     \
   /* Facility code    */  (Facility  << 16) |     \
   /* Exception code   */  (Exception <<  0)))


/////////////////////////// Quick MessageBox Macro ////////////////////////////


inline void chMB(PCSTR szMsg) {
	char szTitle[MAX_PATH];
	GetModuleFileNameA(NULL, szTitle, _countof(szTitle));
	MessageBoxA(GetActiveWindow(), szMsg, szTitle, MB_OK);
}


//////////////////////////// Assert/Verify Macros /////////////////////////////


inline void chFAIL(PSTR szMsg) {
	chMB(szMsg);
	DebugBreak();
}


// Put up an assertion failure message box.
inline void chASSERTFAIL(LPCSTR file, int line, PCSTR expr) {
	char sz[2 * MAX_PATH];
	wsprintfA(sz, "File %s, line %d : %s", file, line, expr);
	chFAIL(sz);
}


// Put up a message box if an assertion fails in a debug build.
#ifdef _DEBUG
#define chASSERT(x) if (!(x)) chASSERTFAIL(__FILE__, __LINE__, #x)
#else
#define chASSERT(x)
#endif


// Assert in debug builds, but don't remove the code in retail builds.
#ifdef _DEBUG
#define chVERIFY(x) chASSERT(x)
#else
#define chVERIFY(x) (x)
#endif


/////////////////////////// chHANDLE_DLGMSG Macro /////////////////////////////


// The normal HANDLE_MSG macro in WindowsX.h does not work properly for dialog
// boxes because DlgProc returns a BOOL instead of an LRESULT (like
// WndProcs). This chHANDLE_DLGMSG macro corrects the problem:
#define chHANDLE_DLGMSG(hWnd, message, fn)                 \
   case (message): return (SetDlgMsgResult(hWnd, uMsg,     \
      HANDLE_##message((hWnd), (wParam), (lParam), (fn))))


//////////////////////// Dialog Box Icon Setting Macro ////////////////////////


// Sets the dialog box icons
inline void chSETDLGICONS(HWND hWnd, int idi) {
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)
		LoadIcon((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
			MAKEINTRESOURCE(idi)));
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)
		LoadIcon((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
			MAKEINTRESOURCE(idi)));
}


/////////////////////////// Common Linker Settings ////////////////////////////


#pragma comment(linker, "/nodefaultlib:oldnames.lib")

// Needed for supporting XP/Vista styles.
#if defined(_M_IA64)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='IA64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#if defined(_M_X64)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.6000.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#if defined(M_IX86)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif



//end of common header

//def of tool help
class CToolhelp {
private:
	HANDLE m_hSnapshot;

public:
	CToolhelp(DWORD dwFlags = 0, DWORD dwProcessID = 0);
	~CToolhelp();

	BOOL CreateSnapshot(DWORD dwFlags, DWORD dwProcessID = 0);

	BOOL ProcessFirst(PPROCESSENTRY32 ppe) const;
	BOOL ProcessNext(PPROCESSENTRY32 ppe) const;
	BOOL ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe) const;

	BOOL ModuleFirst(PMODULEENTRY32 pme) const;
	BOOL ModuleNext(PMODULEENTRY32 pme) const;
	BOOL ModuleFind(PVOID pvBaseAddr, PMODULEENTRY32 pme) const;
	BOOL ModuleFind(PTSTR pszModName, PMODULEENTRY32 pme) const;

	BOOL ThreadFirst(PTHREADENTRY32 pte) const;
	BOOL ThreadNext(PTHREADENTRY32 pte) const;

	BOOL HeapListFirst(PHEAPLIST32 phl) const;
	BOOL HeapListNext(PHEAPLIST32 phl) const;
	int  HowManyHeaps() const;

	// Note: The heap block functions do not reference a snapshot and
	// just walk the process's heap from the beginning each time. Infinite 
	// loops can occur if the target process changes its heap while the
	// functions below are enumerating the blocks in the heap.
	BOOL HeapFirst(PHEAPENTRY32 phe, DWORD dwProcessID,
		UINT_PTR dwHeapID) const;
	BOOL HeapNext(PHEAPENTRY32 phe) const;
	int  HowManyBlocksInHeap(DWORD dwProcessID, DWORD dwHeapId) const;
	BOOL IsAHeap(HANDLE hProcess, PVOID pvBlock, PDWORD pdwFlags) const;

public:
	static BOOL EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable = TRUE);
	static BOOL ReadProcessMemory(DWORD dwProcessID, LPCVOID pvBaseAddress,
		PVOID pvBuffer, SIZE_T cbRead, SIZE_T* pNumberOfBytesRead = NULL);
};


///////////////////////////////////////////////////////////////////////////////


inline CToolhelp::CToolhelp(DWORD dwFlags, DWORD dwProcessID) {

	m_hSnapshot = INVALID_HANDLE_VALUE;
	CreateSnapshot(dwFlags, dwProcessID);
}


///////////////////////////////////////////////////////////////////////////////


inline CToolhelp::~CToolhelp() {

	if (m_hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(m_hSnapshot);
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::CreateSnapshot(DWORD dwFlags, DWORD dwProcessID) {

	if (m_hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(m_hSnapshot);

	if (dwFlags == 0) {
		m_hSnapshot = INVALID_HANDLE_VALUE;
	}
	else {
		m_hSnapshot = CreateToolhelp32Snapshot(dwFlags, dwProcessID);
	}
	return(m_hSnapshot != INVALID_HANDLE_VALUE);
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable) {

	// Enabling the debug privilege allows the application to see
	// information about service applications
	BOOL fOk = FALSE;    // Assume function fails
	HANDLE hToken;

	// Try to open this process's access token
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
		&hToken)) {

		// Attempt to modify the given privilege
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, szPrivilege, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);

		// Don't forget to close the token handle
		CloseHandle(hToken);
	}
	return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::ReadProcessMemory(DWORD dwProcessID,
	LPCVOID pvBaseAddress, PVOID pvBuffer, SIZE_T cbRead,
	SIZE_T* pNumberOfBytesRead) {

	return(Toolhelp32ReadProcessMemory(dwProcessID, pvBaseAddress, pvBuffer,
		cbRead, pNumberOfBytesRead));
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::ProcessFirst(PPROCESSENTRY32 ppe) const {

	BOOL fOk = Process32First(m_hSnapshot, ppe);
	if (fOk && (ppe->th32ProcessID == 0))
		fOk = ProcessNext(ppe); // Remove the "[System Process]" (PID = 0)
	return(fOk);
}


inline BOOL CToolhelp::ProcessNext(PPROCESSENTRY32 ppe) const {

	BOOL fOk = Process32Next(m_hSnapshot, ppe);
	if (fOk && (ppe->th32ProcessID == 0))
		fOk = ProcessNext(ppe); // Remove the "[System Process]" (PID = 0)
	return(fOk);
}


inline BOOL CToolhelp::ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe)
const {

	BOOL fFound = FALSE;
	for (BOOL fOk = ProcessFirst(ppe); fOk; fOk = ProcessNext(ppe)) {
		fFound = (ppe->th32ProcessID == dwProcessId);
		if (fFound) break;
	}
	return(fFound);
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::ModuleFirst(PMODULEENTRY32 pme) const {

	return(Module32First(m_hSnapshot, pme));
}


inline BOOL CToolhelp::ModuleNext(PMODULEENTRY32 pme) const {
	return(Module32Next(m_hSnapshot, pme));
}

inline BOOL CToolhelp::ModuleFind(PVOID pvBaseAddr, PMODULEENTRY32 pme) const {

	BOOL fFound = FALSE;
	for (BOOL fOk = ModuleFirst(pme); fOk; fOk = ModuleNext(pme)) {
		fFound = (pme->modBaseAddr == pvBaseAddr);
		if (fFound) break;
	}
	return(fFound);
}

inline BOOL CToolhelp::ModuleFind(PTSTR pszModName, PMODULEENTRY32 pme) const {
	BOOL fFound = FALSE;
	for (BOOL fOk = ModuleFirst(pme); fOk; fOk = ModuleNext(pme)) {
		fFound = (lstrcmpi(pme->szModule, pszModName) == 0) ||
			(lstrcmpi(pme->szExePath, pszModName) == 0);
		if (fFound) break;
	}
	return(fFound);
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::ThreadFirst(PTHREADENTRY32 pte) const {

	return(Thread32First(m_hSnapshot, pte));
}


inline BOOL CToolhelp::ThreadNext(PTHREADENTRY32 pte) const {

	return(Thread32Next(m_hSnapshot, pte));
}


///////////////////////////////////////////////////////////////////////////////


inline int CToolhelp::HowManyHeaps() const {

	int nHowManyHeaps = 0;
	HEAPLIST32 hl = { sizeof(hl) };
	for (BOOL fOk = HeapListFirst(&hl); fOk; fOk = HeapListNext(&hl))
		nHowManyHeaps++;
	return(nHowManyHeaps);
}


inline int CToolhelp::HowManyBlocksInHeap(DWORD dwProcessID,
	DWORD dwHeapID) const {

	int nHowManyBlocksInHeap = 0;
	HEAPENTRY32 he = { sizeof(he) };
	BOOL fOk = HeapFirst(&he, dwProcessID, dwHeapID);
	for (; fOk; fOk = HeapNext(&he))
		nHowManyBlocksInHeap++;
	return(nHowManyBlocksInHeap);
}


inline BOOL CToolhelp::HeapListFirst(PHEAPLIST32 phl) const {

	return(Heap32ListFirst(m_hSnapshot, phl));
}


inline BOOL CToolhelp::HeapListNext(PHEAPLIST32 phl) const {

	return(Heap32ListNext(m_hSnapshot, phl));
}


inline BOOL CToolhelp::HeapFirst(PHEAPENTRY32 phe, DWORD dwProcessID,
	UINT_PTR dwHeapID) const {

	return(Heap32First(phe, dwProcessID, dwHeapID));
}


inline BOOL CToolhelp::HeapNext(PHEAPENTRY32 phe) const {

	return(Heap32Next(phe));
}


inline BOOL CToolhelp::IsAHeap(HANDLE hProcess, PVOID pvBlock,
	PDWORD pdwFlags) const {

	HEAPLIST32 hl = { sizeof(hl) };
	for (BOOL fOkHL = HeapListFirst(&hl); fOkHL; fOkHL = HeapListNext(&hl)) {
		HEAPENTRY32 he = { sizeof(he) };
		BOOL fOkHE = HeapFirst(&he, hl.th32ProcessID, hl.th32HeapID);
		for (; fOkHE; fOkHE = HeapNext(&he)) {
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQueryEx(hProcess, (PVOID)he.dwAddress, &mbi, sizeof(mbi));
			if (chINRANGE(mbi.AllocationBase, pvBlock,
				(PBYTE)mbi.AllocationBase + mbi.RegionSize)) {

				*pdwFlags = hl.dwFlags;
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

//end of tool help

/////////////////////////////////////////////////////////////////////////////////

class CAPIHook {
public:
	// Hook a function in all modules
	CAPIHook(PSTR pszCalleeModName, PSTR pszFuncName, PROC pfnHook);

	// Unhook a function from all modules
	~CAPIHook();

	// Returns the original address of the hooked function
	operator PROC() { return(m_pfnOrig); }

	// Hook module w/CAPIHook implementation?
	// I have to make it static because I need to use it 
	// in ReplaceIATEntryInAllMods
	static BOOL ExcludeAPIHookMod;


public:
	// Calls the real GetProcAddress 
	static FARPROC WINAPI GetProcAddressRaw(HMODULE hmod, PCSTR pszProcName);

private:
	static PVOID sm_pvMaxAppAddr; // Maximum private memory address
	static CAPIHook* sm_pHead;    // Address of first object
	CAPIHook* m_pNext;            // Address of next  object

	PCSTR m_pszCalleeModName;     // Module containing the function (ANSI)
	PCSTR m_pszFuncName;          // Function name in callee (ANSI)
	PROC  m_pfnOrig;              // Original function address in callee
	PROC  m_pfnHook;              // Hook function address

private:
	// Replaces a symbol's address in a module's import section
	static void WINAPI ReplaceIATEntryInAllMods(PCSTR pszCalleeModName,
		PROC pfnOrig, PROC pfnHook);

	// Replaces a symbol's address in all modules' import sections
	static void WINAPI ReplaceIATEntryInOneMod(PCSTR pszCalleeModName,
		PROC pfnOrig, PROC pfnHook, HMODULE hmodCaller);

	// Replaces a symbol's address in a module's export sections
	static void ReplaceEATEntryInOneMod(HMODULE hmod, PCSTR pszFunctionName, PROC pfnNew);

private:
	// Used when a DLL is newly loaded after hooking a function
	static void    WINAPI FixupNewlyLoadedModule(HMODULE hmod, DWORD dwFlags);

	// Used to trap when DLLs are newly loaded
	static HMODULE WINAPI LoadLibraryA(PCSTR pszModulePath);
	static HMODULE WINAPI LoadLibraryW(PCWSTR pszModulePath);
	static HMODULE WINAPI LoadLibraryExA(PCSTR pszModulePath,
		HANDLE hFile, DWORD dwFlags);
	static HMODULE WINAPI LoadLibraryExW(PCWSTR pszModulePath,
		HANDLE hFile, DWORD dwFlags);

	// Returns address of replacement function if hooked function is requested
	static FARPROC WINAPI GetProcAddress(HMODULE hmod, PCSTR pszProcName);

private:
	// Instantiates hooks on these functions
	static CAPIHook sm_LoadLibraryA;
	static CAPIHook sm_LoadLibraryW;
	static CAPIHook sm_LoadLibraryExA;
	static CAPIHook sm_LoadLibraryExW;
	static CAPIHook sm_GetProcAddress;
};



// The head of the linked-list of CAPIHook objects
CAPIHook* CAPIHook::sm_pHead = NULL;

// By default, the module containing the CAPIHook() is not hooked
BOOL CAPIHook::ExcludeAPIHookMod = TRUE; 


///////////////////////////////////////////////////////////////////////////////


CAPIHook::CAPIHook(PSTR pszCalleeModName, PSTR pszFuncName, PROC pfnHook) {

   // Note: the function can be hooked only if the exporting module 
   //       is already loaded. A solution could be to store the function
   //       name as a member; then, in the hooked LoadLibrary* handlers, parse
   //       the list of CAPIHook instances, check if pszCalleeModName
   //       is the name of the loaded module to hook its export table and 
   //       re-hook the import tables of all loaded modules.
  
   m_pNext  = sm_pHead;    // The next node was at the head
   sm_pHead = this;        // This node is now at the head

   // Save information about this hooked function
   m_pszCalleeModName   = pszCalleeModName;
   m_pszFuncName        = pszFuncName;
   m_pfnHook            = pfnHook;

   m_pfnOrig            = GetProcAddressRaw(GetModuleHandleA(pszCalleeModName), m_pszFuncName);

   // If function does not exit,... bye bye
   // This happens when the module is not already loaded
   if (m_pfnOrig == NULL)
   {
      wchar_t szPathname[MAX_PATH];
      GetModuleFileNameW(NULL, szPathname, _countof(szPathname));
      wchar_t sz[1024];
      StringCchPrintfW(sz, _countof(sz), 
         TEXT("[%4u - %s] impossible to find %S\r\n"), 
         GetCurrentProcessId(), szPathname, pszFuncName);
      OutputDebugString(sz);
      return;
   }
   
#ifdef _DEBUG
   // This section was used for debugging sessions when Explorer died as 
   // a folder content was requested
   // 
   //static BOOL s_bFirstTime = TRUE;
   //if (s_bFirstTime)
   //{
   //   s_bFirstTime = FALSE;

   //   wchar_t szPathname[MAX_PATH];
   //   GetModuleFileNameW(NULL, szPathname, _countof(szPathname));
   //   wchar_t* pszExeFile = wcsrchr(szPathname, L'\\') + 1;
   //   OutputDebugStringW(L"Injected in ");
   //   OutputDebugStringW(pszExeFile);
   //   if (_wcsicmp(pszExeFile, L"Explorer.EXE") == 0)
   //   {
   //      DebugBreak();
   //   }
   //   OutputDebugStringW(L"\n   --> ");
   //   StringCchPrintfW(szPathname, _countof(szPathname), L"%S", pszFuncName);
   //   OutputDebugStringW(szPathname);
   //   OutputDebugStringW(L"\n");
   //}
#endif

   // Hook this function in all currently loaded modules
   ReplaceIATEntryInAllMods(m_pszCalleeModName, m_pfnOrig, m_pfnHook);
}


///////////////////////////////////////////////////////////////////////////////


CAPIHook::~CAPIHook() {

   // Unhook this function from all modules
   ReplaceIATEntryInAllMods(m_pszCalleeModName, m_pfnHook, m_pfnOrig);

   // Remove this object from the linked list
   CAPIHook* p = sm_pHead; 
   if (p == this) {     // Removing the head node
      sm_pHead = p->m_pNext; 
   } else {

      BOOL bFound = FALSE;

      // Walk list from head and fix pointers
      for (; !bFound && (p->m_pNext != NULL); p = p->m_pNext) {
         if (p->m_pNext == this) { 
            // Make the node that points to us point to our next node
            p->m_pNext = p->m_pNext->m_pNext; 
            bFound = TRUE;
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////


// NOTE: This function must NOT be inlined
FARPROC CAPIHook::GetProcAddressRaw(HMODULE hmod, PCSTR pszProcName) {

   return(::GetProcAddress(hmod, pszProcName));
}


///////////////////////////////////////////////////////////////////////////////


// Returns the HMODULE that contains the specified memory address
static HMODULE ModuleFromAddress(PVOID pv) {

   MEMORY_BASIC_INFORMATION mbi;
   return((VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) 
      ? (HMODULE) mbi.AllocationBase : NULL);
}


///////////////////////////////////////////////////////////////////////////////


void CAPIHook::ReplaceIATEntryInAllMods(PCSTR pszCalleeModName, 
   PROC pfnCurrent, PROC pfnNew) {

   HMODULE hmodThisMod = ExcludeAPIHookMod 
      ? ModuleFromAddress(ReplaceIATEntryInAllMods) : NULL;

   // Get the list of modules in this process
   CToolhelp th(TH32CS_SNAPMODULE, GetCurrentProcessId());

   MODULEENTRY32 me = { sizeof(me) };
   for (BOOL bOk = th.ModuleFirst(&me); bOk; bOk = th.ModuleNext(&me)) {

      // NOTE: We don't hook functions in our own module
      if (me.hModule != hmodThisMod) {

         // Hook this function in this module
         ReplaceIATEntryInOneMod(
            pszCalleeModName, pfnCurrent, pfnNew, me.hModule);
      }
   }
}


///////////////////////////////////////////////////////////////////////////////


// Handle unexpected exceptions if the module is unloaded
LONG WINAPI InvalidReadExceptionFilter(PEXCEPTION_POINTERS pep) {

   // handle all unexpected exceptions because we simply don't patch
   // any module in that case
   LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;

   // Note: pep->ExceptionRecord->ExceptionCode has 0xc0000005 as a value

   return(lDisposition);
}


void CAPIHook::ReplaceIATEntryInOneMod(PCSTR pszCalleeModName, 
   PROC pfnCurrent, PROC pfnNew, HMODULE hmodCaller) {

   // Get the address of the module's import section
   ULONG ulSize;

   // An exception was triggered by Explorer (when browsing the content of 
   // a folder) into imagehlp.dll. It looks like one module was unloaded...
   // Maybe some threading problem: the list of modules from Toolhelp might 
   // not be accurate if FreeLibrary is called during the enumeration.
   PIMAGE_IMPORT_DESCRIPTOR pImportDesc = NULL;
   __try {
      pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ImageDirectoryEntryToData(
         hmodCaller, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);
   } 
   __except (InvalidReadExceptionFilter(GetExceptionInformation())) {
      // Nothing to do in here, thread continues to run normally
      // with NULL for pImportDesc 
   }
   
   if (pImportDesc == NULL)
      return;  // This module has no import section or is no longer loaded


   // Find the import descriptor containing references to callee's functions
   for (; pImportDesc->Name; pImportDesc++) {
      PSTR pszModName = (PSTR) ((PBYTE) hmodCaller + pImportDesc->Name);
      if (lstrcmpiA(pszModName, pszCalleeModName) == 0) {

         // Get caller's import address table (IAT) for the callee's functions
         PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA) 
            ((PBYTE) hmodCaller + pImportDesc->FirstThunk);

         // Replace current function address with new function address
         for (; pThunk->u1.Function; pThunk++) {

            // Get the address of the function address
            PROC* ppfn = (PROC*) &pThunk->u1.Function;

            // Is this the function we're looking for?
            BOOL bFound = (*ppfn == pfnCurrent);
            if (bFound) {
               if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, 
                    sizeof(pfnNew), NULL) && (ERROR_NOACCESS == GetLastError())) {
                  DWORD dwOldProtect;
                  if (VirtualProtect(ppfn, sizeof(pfnNew), PAGE_WRITECOPY, 
                     &dwOldProtect)) {

                     WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, 
                        sizeof(pfnNew), NULL);
                     VirtualProtect(ppfn, sizeof(pfnNew), dwOldProtect, 
                        &dwOldProtect);
                  }
               }
               return;  // We did it, get out
            }
         }
      }  // Each import section is parsed until the right entry is found and patched
   }
}


///////////////////////////////////////////////////////////////////////////////


void CAPIHook::ReplaceEATEntryInOneMod(HMODULE hmod, PCSTR pszFunctionName, 
   PROC pfnNew) {

   // Get the address of the module's export section
   ULONG ulSize;

   PIMAGE_EXPORT_DIRECTORY pExportDir = NULL;
   __try {
      pExportDir = (PIMAGE_EXPORT_DIRECTORY) ImageDirectoryEntryToData(
         hmod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ulSize);
   } 
   __except (InvalidReadExceptionFilter(GetExceptionInformation())) {
      // Nothing to do in here, thread continues to run normally
      // with NULL for pExportDir 
   }
   
   if (pExportDir == NULL)
      return;  // This module has no export section or is unloaded

   PDWORD pdwNamesRvas = (PDWORD) ((PBYTE) hmod + pExportDir->AddressOfNames);
   PWORD pdwNameOrdinals = (PWORD) 
      ((PBYTE) hmod + pExportDir->AddressOfNameOrdinals);
   PDWORD pdwFunctionAddresses = (PDWORD) 
      ((PBYTE) hmod + pExportDir->AddressOfFunctions);

   // Walk the array of this module's function names 
   for (DWORD n = 0; n < pExportDir->NumberOfNames; n++) {
      // Get the function name
      PSTR pszFuncName = (PSTR) ((PBYTE) hmod + pdwNamesRvas[n]);

      // If not the specified function, try the next function
      if (lstrcmpiA(pszFuncName, pszFunctionName) != 0) continue;

      // We found the specified function
      // --> Get this function's ordinal value
      WORD ordinal = pdwNameOrdinals[n];

      // Get the address of this function's address
      PROC* ppfn = (PROC*) &pdwFunctionAddresses[ordinal];
      
      // Turn the new address into an RVA
      pfnNew = (PROC) ((PBYTE) pfnNew - (PBYTE) hmod);

      // Replace current function address with new function address
      if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, 
         sizeof(pfnNew), NULL) && (ERROR_NOACCESS == GetLastError())) {
         DWORD dwOldProtect;
         if (VirtualProtect(ppfn, sizeof(pfnNew), PAGE_WRITECOPY, 
            &dwOldProtect)) {
            
            WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, 
               sizeof(pfnNew), NULL);
            VirtualProtect(ppfn, sizeof(pfnNew), dwOldProtect, &dwOldProtect);
         }
      }
      break;  // We did it, get out
   }
}


///////////////////////////////////////////////////////////////////////////////
// Hook LoadLibrary functions and GetProcAddress so that hooked functions
// are handled correctly if these functions are called.

CAPIHook CAPIHook::sm_LoadLibraryA  ("Kernel32.dll", "LoadLibraryA",   
   (PROC) CAPIHook::LoadLibraryA);

CAPIHook CAPIHook::sm_LoadLibraryW  ("Kernel32.dll", "LoadLibraryW",   
   (PROC) CAPIHook::LoadLibraryW);

CAPIHook CAPIHook::sm_LoadLibraryExA("Kernel32.dll", "LoadLibraryExA", 
   (PROC) CAPIHook::LoadLibraryExA);

CAPIHook CAPIHook::sm_LoadLibraryExW("Kernel32.dll", "LoadLibraryExW", 
   (PROC) CAPIHook::LoadLibraryExW);

CAPIHook CAPIHook::sm_GetProcAddress("Kernel32.dll", "GetProcAddress", 
   (PROC) CAPIHook::GetProcAddress);


///////////////////////////////////////////////////////////////////////////////


void CAPIHook::FixupNewlyLoadedModule(HMODULE hmod, DWORD dwFlags) {

   // If a new module is loaded, hook the hooked functions
   if ((hmod != NULL) &&   // Do not hook our own module
       (hmod != ModuleFromAddress(FixupNewlyLoadedModule)) && 
       ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) == 0) &&
       ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) == 0) &&
       ((dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) == 0)
       ) {

      for (CAPIHook* p = sm_pHead; p != NULL; p = p->m_pNext) {
         if (p->m_pfnOrig != NULL) {
            ReplaceIATEntryInAllMods(p->m_pszCalleeModName, 
               p->m_pfnOrig, p->m_pfnHook);  
         } else {
#ifdef _DEBUG
            // We should never end up here 
            wchar_t szPathname[MAX_PATH];
            GetModuleFileNameW(NULL, szPathname, _countof(szPathname));
            wchar_t sz[1024];
            StringCchPrintfW(sz, _countof(sz), 
               TEXT("[%4u - %s] impossible to find %S\r\n"), 
               GetCurrentProcessId(), szPathname, p->m_pszCalleeModName);
            OutputDebugString(sz);
#endif
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////


HMODULE WINAPI CAPIHook::LoadLibraryA(PCSTR pszModulePath) {

   HMODULE hmod = ::LoadLibraryA(pszModulePath);
   FixupNewlyLoadedModule(hmod, 0);
   return(hmod);
}


///////////////////////////////////////////////////////////////////////////////


HMODULE WINAPI CAPIHook::LoadLibraryW(PCWSTR pszModulePath) {

   HMODULE hmod = ::LoadLibraryW(pszModulePath);
   FixupNewlyLoadedModule(hmod, 0);
   return(hmod);
}


///////////////////////////////////////////////////////////////////////////////


HMODULE WINAPI CAPIHook::LoadLibraryExA(PCSTR pszModulePath, 
   HANDLE hFile, DWORD dwFlags) {

   HMODULE hmod = ::LoadLibraryExA(pszModulePath, hFile, dwFlags);
   FixupNewlyLoadedModule(hmod, dwFlags);
   return(hmod);
}


///////////////////////////////////////////////////////////////////////////////


HMODULE WINAPI CAPIHook::LoadLibraryExW(PCWSTR pszModulePath, 
   HANDLE hFile, DWORD dwFlags) {

   HMODULE hmod = ::LoadLibraryExW(pszModulePath, hFile, dwFlags);
   FixupNewlyLoadedModule(hmod, dwFlags);
   return(hmod);
}


///////////////////////////////////////////////////////////////////////////////


FARPROC WINAPI CAPIHook::GetProcAddress(HMODULE hmod, PCSTR pszProcName) {

   // Get the true address of the function
   FARPROC pfn = GetProcAddressRaw(hmod, pszProcName);

   // Is it one of the functions that we want hooked?
   CAPIHook* p = sm_pHead;
   for (; (pfn != NULL) && (p != NULL); p = p->m_pNext) {

      if (pfn == p->m_pfnOrig) {

         // The address to return matches an address we want to hook
         // Return the hook function address instead
         pfn = p->m_pfnHook;
         break;
      }
   }

   return(pfn);
}

void DisableSetUnhandledExceptionFilter()
{
	void *addr = (void*)GetProcAddress(LoadLibrary(TEXT("kernel32.dll")), "SetUnhandledExceptionFilter");

	if (addr)
	{
		unsigned char code[16];
		int size = 0;
		code[size++] = 0x33;
		code[size++] = 0xC0;
		code[size++] = 0xC2;
		code[size++] = 0x04;
		code[size++] = 0x00;

		DWORD dwOldFlag, dwTempFlag;
		VirtualProtect(addr, size, PAGE_READWRITE, &dwOldFlag);
		WriteProcessMemory(GetCurrentProcess(), addr, code, size, NULL);
		VirtualProtect(addr, size, dwOldFlag, &dwTempFlag);
	}
}

//程序未捕获的异常处理函数 
LONG WINAPI ExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	std::wstringstream wss;
	TCHAR szBuf[MAX_PATH];
	GetModuleFileName(NULL, szBuf, MAX_PATH);
	std::string::size_type pos =std::wstring(szBuf).find_last_of('\\');
	wss << std::wstring(szBuf, 0, pos) << "\\csharp.crash." <<  sysTime.wDay << "_" << sysTime.wHour << "_" << sysTime.wMinute << ".dmp";
	HANDLE hFile = ::CreateFile(wss.str().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION einfo;
		einfo.ThreadId = ::GetCurrentThreadId();
		einfo.ExceptionPointers = ExceptionInfo;
		einfo.ClientPointers = FALSE;

		::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpNormal, &einfo, NULL, NULL);
		::CloseHandle(hFile);
	}
	else
	{
		std::wcout << L"error open file " << wss.str() << "error code" << GetLastError();
	}

	return 0;
}

//把当前时刻的线程栈记录到DUMP文件中 
int RecordCurStack()
{
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	std::wstringstream wss;
	TCHAR szBuf[MAX_PATH];
	GetModuleFileName(NULL, szBuf, MAX_PATH);
	std::string::size_type pos = std::wstring(szBuf).find_last_of('\\');
	wss << std::wstring(szBuf, 0, pos) << "\\csharp.crash." << sysTime.wDay << "_" << sysTime.wHour << "_" << sysTime.wMinute << ".dmp";
	HANDLE hFile = ::CreateFile(wss.str().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, NULL, NULL, NULL);

		::CloseHandle(hFile);
		return 1;
	}

	return 0;
}


bool bCreateDumpThrd = true;
//循环检测线程 
//查看到有ADTV2_TEMP.TXT文件，则记录下当前时刻的堆栈 
void CreateDumpThrd(void* pv)
{
	HANDLE hFile;
	std::wstring strPath = TEXT("./ADTV2_TEMP.TXT");
	while (bCreateDumpThrd)
	{
		//每5秒检测一次 
		Sleep(5000);
		hFile = CreateFileW(strPath.c_str(),    // file to open 
			GENERIC_READ,          // open for reading 
			FILE_SHARE_READ,       // share for reading 
			NULL,                  // default security 
			OPEN_EXISTING,         // existing file only 
			FILE_ATTRIBUTE_NORMAL, // normal file 
			NULL);                 // no attr. template 

		if (hFile != INVALID_HANDLE_VALUE)
		{
			//防止多次记录当前堆栈信息，删除文件 
			::CloseHandle(hFile);
			::DeleteFile(strPath.c_str());
			RecordCurStack();
		}
	}
}




LONG WINAPI RedirectedSetUnhandledExceptionFilter(EXCEPTION_POINTERS * ExceptionInfo)
{
	// When the CRT calls SetUnhandledExceptionFilter with NULL parameter
	// our handler will not get removed.
	ExceptionFilter(ExceptionInfo);
	return 0;
}

LONG WINAPI OurSetUnhandledExceptionFilter(EXCEPTION_POINTERS * ExceptionInfo)
{
	std::cout << "\n\n"
				 " _______________________________________________\n"
				 "|                                               \n"
				 "|        ( ︶︿︶)                              \n"
				 "|                                               \n"
		         "|    Oops, I am running into crash.             \n"
		         "|    Contact Tencent TscanCode Team please!     \n"
				 "|_______________________________________________\n" << std::endl;
	ExceptionFilter(ExceptionInfo);
	return EXCEPTION_EXECUTE_HANDLER;
}



void StartWinCrashRecored()
{
	::SetUnhandledExceptionFilter(OurSetUnhandledExceptionFilter);

	CAPIHook apiHook("kernel32.dll", "SetUnhandledExceptionFilter", (PROC)RedirectedSetUnhandledExceptionFilter);
}

//////////////////////////////// End of File //////////////////////////////////
