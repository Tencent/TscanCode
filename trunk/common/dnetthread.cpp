#include "../common/DNetThread.h"
#include <vector>
using namespace std;

DNetMutex::DNetMutex()
{
#ifdef WIN32
	LPSECURITY_ATTRIBUTES lpMutexAttributes = 0;
	BOOL bInitialOwner = FALSE;
	LPCTSTR lpName = 0;

	m_CriticalID = CreateMutex(lpMutexAttributes, bInitialOwner, lpName);
#else
	pthread_mutex_init(&m_CriticalID, NULL);
#endif
}

DNetMutex::~DNetMutex()
{
#ifdef WIN32
	if(m_CriticalID)
	{
		CloseHandle(m_CriticalID);
	}
#else
	pthread_mutex_destroy(&m_CriticalID);
#endif
}

bool DNetMutex::WaitOne()
{
#ifdef WIN32
	if( m_CriticalID == 0)
	{
		return false;
	}
	DWORD result;

	result = WaitForSingleObject(
		m_CriticalID,
		INFINITE
		);
	if( result == WAIT_OBJECT_0)
	{
		return true;
	}
	else
	{
		return false;
	}
#else
	pthread_mutex_lock(&m_CriticalID);
	return true;
#endif
}

// Operations
void DNetMutex::ReleaseMutex()
{
#ifdef WIN32
	::ReleaseMutex(m_CriticalID);
#else
	pthread_mutex_unlock(&m_CriticalID);
#endif
}
