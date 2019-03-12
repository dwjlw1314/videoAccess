/*
 * CAThreadManager.h
 *
 *  Created on: Mar 2, 2015
 *      Author: diwenjie
 *  Compile time is added in the property <boost_thread>
 */

/*
 * Statement:
 *	 boost库使用到是boost-thread-1.41.0-11.el6_1.2
 *	 boost_thread and boost_thread-mt是一样到,链接到都是同一个库
 *	 if prompt "/usr/bin/ld: cannot find -lboost_thread" add
 *	 a soft link (libboost_thread.so) under the path environment variable
 *	 1.1 version unused policy find socketfd
 */

#ifndef CATHREADMANAGER_H_
#define CATHREADMANAGER_H_

#include "CAPublicStruct.h"
#include "CAThreadMutex.h"
#include "CAException.h"
#include "CAZeroMQ.h"
#include "CAVideoTrans.h"
#include "CAAutopUpgrade.h"

using std::map;

typedef map<string,CAVideoTrans*> Mdvr_Avcodec_Map;

typedef struct threadlist
{
	/*
	 * extended data
	 */
	int threadflag;
	/*
	 * read and write socketpair
	 */
	int wfd;
	int rfd;
	/*
	 * Current thread have mdvrId
	 */
	int nodedevnum;
	/*
	 * thread pointer
	 */
	boost::thread *ptr;

	/*
	 * uuid
	 */
	string uuid;
}ThreadList,*pThreadList;

class CAThreadManager : public thread_group
{
public:
	static CAThreadManager* GetInstanceContext();
	static CAThreadManager *g_pThreadManager;

public:
	/*
	 * set socket fd Cache size
	 * flag: SO_RCVBUF and SO_SNDBUF two types(Current)
	 * Later can extend
	 */
	int SetSocketCache(int socketfd, int flag);

	/*
	 * Create a thread class no static method
	 */
	int CreateThreadGroup() throw();

	/*
	 * Create a thread
	 */
	int CreateThread(int flag);
	int CreateMaintainThread();
	/*
	 * modify m_ThreadList
	 */
	void ModifyThreadList(int tflag);
	/*
	 * Thread business processing function
	 */
	void MaintainRun();
	void Run(int flag,int socketwfd,int socketrfd);
	/*
	 * wait all thread
	 */
	void ThreadJoinAll();

	/*
	 * ralloc memory
	 * In order to improve the efficiency,
	 * not frequent allocate space
	 */
	int ReallocMemory(unsigned char** first,size_t fsize,
		unsigned char** second, size_t ssize);
	/*
	 * Return the selected descriptors
	 * Adopting the policy of version 1.1 don't used
	 */
	int RetrunSocketPairfd();

	/*
	 * Decreasing socketpair fd
	 */
	int DecreasSocketPairfd(int socketwfd);

	/*
	 * Operation mdvr-avCodecTrans map
	 */
	CAVideoTrans* OperMdvrAvCodecMap(Mdvrinfo*, Mdvr_Avcodec_Map&);
	static int read_packet(void *opaque, uint8_t *buf, int buf_size);
	static int write_packet(void *opaque, uint8_t *buf, int buf_size);
private:
	CAThreadManager();
	virtual ~CAThreadManager();

private:
	/*
	 * 每个线程上挂载的设备最大数量
	 */
	int m_MaxNodeNums;

	CAThreadMutex m_ThreadMutex;
	CAThreadMutex m_ReallocMutex;
	/*
	 * Before Thread Exception Maintenance
	 */
	std::list<ThreadList> m_ThreadList;
	std::list<ThreadList> m_DeadThreadList;
	//c format structure
	//ThreadList *m_pThreadListHead;
	boost::thread *m_pMaintainThread;
	boost::thread_group m_ThreadGroup;

	CALog *m_pLog;
	CAAutopUpgrade m_Upgrade;
};

#endif /* CATHREADMANAGER_H_ */
