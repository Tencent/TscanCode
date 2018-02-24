#ifndef configH
#define configH

#ifdef _WIN32
#  ifdef TSCANCODELIB_EXPORT
#    define TSCANCODELIB __declspec(dllexport)
#  elif defined(TSCANCODELIB_IMPORT)
#    define TSCANCODELIB __declspec(dllimport)
#  else
#    define TSCANCODELIB
#  endif
#else
#  define TSCANCODELIB
#endif

#include <assert.h>

#if defined(_WIN32)
#pragma warning(disable:4100)
#pragma warning(disable:4819)
#endif

// MS Visual C++ memory leak debug tracing
#if defined(_MSC_VER) && defined(_DEBUG)
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#endif

#include <string>
#ifdef __linux__
#include <string.h>
#endif

#if (defined(__GNUC__) || defined(__sun)) && !defined(__MINGW32__)
#define TSC_THREADING_MODEL_NOT_WIN
#elif defined(_WIN32)
#define TSC_THREADING_MODEL_WIN
//#include <windows.h>
#endif

#ifdef TSC_THREADING_MODEL_WIN

#define TSC_THREAD				HANDLE
#define TSC_LOCK				CRITICAL_SECTION
#define TSC_LOCK_ENTER(lock)	EnterCriticalSection(lock)
#define TSC_LOCK_LEAVE(lock)	LeaveCriticalSection(lock)
#define TSC_LOCK_INIT(lock)		InitializeCriticalSection(lock)
#define TSC_LOCK_DELETE(lock)	DeleteCriticalSection(lock)

#define TSC_MAX					max
#define TSC_MIN					min

#define PATH_SEP	'\\'

#else

#define TSC_THREAD				pthread_t
#define TSC_LOCK				pthread_mutex_t
#define TSC_LOCK_ENTER(lock)	pthread_mutex_lock(lock)
#define TSC_LOCK_LEAVE(lock)	pthread_mutex_unlock(lock)
#define TSC_LOCK_INIT(lock)		pthread_mutex_init(lock, nullptr)
#define TSC_LOCK_DELETE(lock)	pthread_mutex_destroy(lock)

#define TSC_MAX					std::max
#define TSC_MIN					std::min

#define PATH_SEP	'/'

#endif



static const std::string emptyString;

#define SAFE_DELETE(p) { if(p) { delete p; p = nullptr; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] p; p = nullptr; } }



// if TSC2_DUMP_TYPE_TREE is defined, global type tree would be dumped.
#define TSC2_DUMP_TYPE_TREE


#if defined(_MSC_VER) && (_MSC_VER > 1600 ) && (!defined WINCE)
#define  SNPRINTF snprintf
#elif defined _MSC_VER
#define SNPRINTF	_snprintf
#else
#define SNPRINTF	snprintf
#endif


#endif // configH
