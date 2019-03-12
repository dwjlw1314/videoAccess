/*
 * CAThreadMutex.cpp
 *
 *  Created on: Feb 9, 2015
 *      Author: diwenjie
 */

#include "CAThreadMutex.h"

/*
 * 互斥锁类型为 PTHREAD_MUTEX_ERRORCHECK,会提供错误检查,
 * 如果某个线程尝试重新锁定的互斥锁已经由该线程锁定,则返回错误
 * 如果某个线程尝试解除锁定的互斥锁不是由该线程锁定或者未锁定
 * 则返回EDEADLK错误,表示当前线程已经拥有互斥锁
 */
CAThreadMutex::CAThreadMutex(string cname)
{
	// TODO Auto-generated constructor stub
	int rc;
	pthread_mutexattr_t attr;

	m_log = CALog::GetInstance(cname);

	rc = pthread_mutexattr_init(&attr);
	if (0 != rc)
		goto end;

	rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	if (0 != rc)
		goto end;

	rc = pthread_mutex_init(&m_mutex, &attr);
	if (0 != rc)
		goto end;

	rc = pthread_mutexattr_destroy(&attr);
	if (0 != rc)
		goto end;
	return;
end:
	m_log->Info("%s;code=%d","CAThreadMutex pthread_mutexattr_init error",rc);
	throw CAException(1002);
}

CAThreadMutex::~CAThreadMutex()
{
	// TODO Auto-generated destructor stub
    int rc = 0;
    rc = pthread_mutex_destroy(&m_mutex);
    if (rc)
    {
    	m_log->Info("CAThreadMutex pthread_mutex_destroy error;code=%d",rc);
    	return;
    }
	m_log->Info("CAThreadMutex pthread_mutex_destroy ok;");
}

/*
 * 异常抛出不做处理会导致程序错误崩溃
 */
void CAThreadMutex::lock() const
{
    int rc = pthread_mutex_lock(&m_mutex);
    if(rc != 0)
    {
    	m_log->Info("%s;code=%d","CAThreadMutex lock error",rc);
        if(rc == EDEADLK)
    	{
        	throw CAException(1003);
    	}
    	else
    	{
    		throw CAException(1004);
    	}
    }
}

/*
 * 异常抛出不做处理会导致程序错误崩溃
 */
bool CAThreadMutex::tryLock() const
{
    int rc = pthread_mutex_trylock(&m_mutex);
    if(rc != 0 && rc != EBUSY)
    {
    	m_log->Info("%s;code=%d","CAThreadMutex tryLock error",rc);
        if(rc == EDEADLK)
    	{
        	throw CAException(1003);
    	}
    	else
    	{
    		throw CAException(1005);
    	}
    }
    return (rc == 0);
}

/*
 * 异常抛出不做处理会导致程序错误崩溃
 */
void CAThreadMutex::unlock() const
{
    int rc = pthread_mutex_unlock(&m_mutex);
    if(rc != 0)
    {
    	m_log->Info("%s;code=%d","CAThreadMutex unlock error",rc);
    	throw CAException(1006);
    }
}
