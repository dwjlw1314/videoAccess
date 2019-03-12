/*
 * CAThreadMutex.h
 *
 *  Created on: Feb 9, 2015
 *      Author: diwenjie
 */

#ifndef CATHREADMUTEX_H_
#define CATHREADMUTEX_H_

#include "CAPublicStruct.h"
#include "CALog.h"
#include "CAException.h"

using namespace std;
/*
 * 线程锁
 */
class CAThreadMutex: public boost::noncopyable {
public:
	CAThreadMutex(string cname = "video_cat");
	virtual ~CAThreadMutex();

public:

	/*
	 * 加锁
	 */
	void lock() const;

	/*
	 * 尝试锁
	 *
	 * @return bool
	 */
	bool tryLock() const;

	/*
	 * 解锁
	 */
	void unlock() const;

	/*
	 * 加锁后调用unlock是否会解锁, 给Monitor使用的
	 * 永远返回true
	 * @return bool
	 */
	bool willUnlock() const {
		return true;
	}

protected:
	// noncopyable
	CAThreadMutex(const CAThreadMutex&);
	void operator=(const CAThreadMutex&);

protected:
	mutable pthread_mutex_t m_mutex;
	CALog *m_log;
};

#endif /* CATHREADMUTEX_H_ */
