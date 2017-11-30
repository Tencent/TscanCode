#pragma once

#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

class DNetMutex
{
public:
	// Constructor
	DNetMutex();

	~DNetMutex();

	// Overrides
	bool WaitOne();

	// Operations
	void ReleaseMutex();

private:
#ifdef WIN32
	HANDLE m_CriticalID;
#else
	pthread_mutex_t m_CriticalID;
#endif
};
