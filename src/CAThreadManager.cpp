/*
 * CAThreadManager.cpp
 *
 *  Created on: Mar 2, 2015
 *      Author: diwenjie
 *  <socketpair>: To establish an anonymous socket is connected
 *  boost member before used protected
 */

#include "CAThreadManager.h"

CAThreadManager* CAThreadManager::g_pThreadManager = NULL;

int CAThreadManager::read_packet(void* opaque, uint8_t* buf, int buf_size) {
	struct buffer_data *bd = (struct buffer_data *) opaque;

	buf_size = FFMIN(buf_size, (int )bd->nCustomDataLen);

	if (!buf_size)
		return buf_size;

	/* copy internal buffer data to buf */
	if (bd->pCustomValue && bd->nCustomDataLen > 0) {
		memcpy(buf, bd->pCustomValue, buf_size);
		bd->pCustomValue += buf_size;
		bd->nCustomDataLen -= buf_size;
	}

	return buf_size;
}

int CAThreadManager::write_packet(void* opaque, uint8_t* buf, int buf_size) {
	ZmqData* data = (ZmqData*) opaque;
	CALog* p_log = CALog::GetInstance(g_data.ZlogRmCname);
//	p_log->Info("write_packet() the size of data is %d",buf_size);
	//CAAvCodeTrans *p = (CAAvCodeTrans*)opaque;
	CAZeroMQ *pZeroMQ;
	try {
		pZeroMQ = CAZeroMQ::GetInstance("tcp", "*", DEFAULT_PORT, ZMQ_PUB);
	} catch (const CAException& mqexp) {
		sleep(10);
		//m_log->Info("m_pZeroMQSender Try Again Constrcutor!!");
		if (!(pZeroMQ = CAZeroMQ::GetInstance("tcp", "*", DEFAULT_PORT, ZMQ_PUB)))
			throw(mqexp);
	}
	if (pZeroMQ->SendData(buf, buf_size, data)) {
		p_log->Error("SendData() false!!the size of data :%d", buf_size);
	}

	FILE* fp = fopen("access_rev.ts", "a+");
	fwrite(buf, buf_size, 1, fp);
	fclose(fp);
#if FRAME_WRITE_FILE
	//test *.TS file whether right
	p->m_VideoFile.fwrite((char*)buf, buf_size, 1);
#endif

//	p_log->Debug("write_packet() success and the size of data is %d", buf_size);
	return buf_size;
}

CAThreadManager* CAThreadManager::GetInstanceContext() {
	if (!g_pThreadManager) {
		g_pThreadManager = new CAThreadManager;
		//保证程序启动后相关编码信息已经注册
		av_register_all();
//		CAVideoTrans videoTrans(NULL,NULL);
	}
	return g_pThreadManager;
}

/*
 * main thread sleep will be interruption
 */
/*
 * 1.2Ver Cancel join_all();
 */
void CAThreadManager::ThreadJoinAll() {
	m_pMaintainThread->join();
	//m_ThreadGroup.join_all();
}

/*
 * Returns 0 on success, -1 for errors
 * <test---result!!!>
 * The default 256k, send SOCKET the maximum is 2M,
 * (256*8*1024byte=2097152=2M)
 * receive SOCKET the maximum is 8M
 */
int CAThreadManager::SetSocketCache(int fd, int flag) {
	int nErrorCode;
	int nBufLen, orginal;
	socklen_t nOptLen = sizeof(nBufLen);
	// 获取当前套接字socketfd的接受数据缓冲区大小
	nErrorCode = getsockopt(fd, SOL_SOCKET, flag, (char*) &nBufLen, &nOptLen);
	if (SOCKET_ERROR == nErrorCode) {
		m_pLog->Info("getsockopt (rev) error");
		return FUNCTION_ERROR;
	}

	orginal = nBufLen;
	// 设置当前套接字s的接受数据缓冲区为原来的N倍
	if (SO_SNDBUF == flag)
		nBufLen = FFMIN(2097152, nBufLen * g_data.SocketCacheMultiple);
	else if (SO_RCVBUF == flag)
		nBufLen = FFMIN(8388608, nBufLen * g_data.SocketCacheMultiple);
	nBufLen /= 2; //The kernel automatically expand twice

	//m_log->Info("socketfd(%d)->SetSocketCache Multiple=%d,Orginal=%d,Lastsize=%d",
	//	fd,g_data.SocketCacheMultiple,orginal,nBufLen*2);

	nErrorCode = setsockopt(fd, SOL_SOCKET, flag, (char*) &nBufLen, nOptLen);
	if (SOCKET_ERROR == nErrorCode) {
		m_pLog->Info("setsockopt (rev) error");
		return FUNCTION_ERROR;
	}
	// 检查套接字s的接受缓冲区是否设置成功
	int uiNewRcvBuf = 0;
	nErrorCode = getsockopt(fd, SOL_SOCKET, flag, (char*) &uiNewRcvBuf,
			&nOptLen);
	if (SOCKET_ERROR == nErrorCode || nBufLen * 2 != uiNewRcvBuf) {
		m_pLog->Info("Socket Cache Set False;NewSocketCache[%d]", uiNewRcvBuf);
		return FUNCTION_ERROR;
	}
	return FUNCTION_SUCCESS;
}

/*
 * 初始化一定数量的线程,有抛出异常的可能,1.1版本暂时不处理
 * 线程创建个数少于配置文件的程序继续执行,1.1版本暂时不处理
 * 线程退出维护链表,1.1版本暂时不处理
 * Returns 0 on success, >0 for Thread numbers;
 */
int CAThreadManager::CreateThreadGroup() throw () {
	int threadnums = g_data.ThreadNums;

	if (CreateMaintainThread())
		return m_ThreadList.size();

	for (int i = 0; i < threadnums; i++) {
		CreateThread(i);
	}
	m_pLog->Info("Thread group created. Thread numbers=%d;",
			m_ThreadList.size());
	return m_ThreadList.size();
}

/*
 * 创建一个管理业务处理的线程,保证业务线程崩溃后及时清理线程组垃圾数据
 * Returns 0 on success, -1 on false;
 */
int CAThreadManager::CreateMaintainThread() {
	m_pMaintainThread = m_ThreadGroup.create_thread(
			boost::bind(&CAThreadManager::MaintainRun, this));
	if (!m_pMaintainThread) {
		m_pLog->Info("CAThreadManager::CreateMaintainThread false!");
		return FUNCTION_ERROR;
	}
	m_pLog->Info("CAThreadManager::CreateMaintainThread Success!");

	return FUNCTION_SUCCESS;
}

/*
 * 创建业务处理的线程
 * Returns 0 on success, -1 on false;
 */
int CAThreadManager::CreateThread(int flag) {
	ThreadList threadnode = { 0 };
	boost::thread *p_thread = NULL;
	int sockpair[2] = { 0 };
	char uuid[32] = { 0 };
	uuid_t uid;
	if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair)) {
		int errcode = errno; //Can Cancel this line
		m_pLog->Info("CreateThreadGroup error.times=%d;socketpair errstat=%s",
				flag, strerror(errcode));
		return FUNCTION_ERROR;
	}
	p_thread = m_ThreadGroup.create_thread(
			boost::bind(&CAThreadManager::Run, this, flag, sockpair[0],
					sockpair[1]));
	if (!p_thread) {
		shutdown(sockpair[0], SHUT_RDWR);
		shutdown(sockpair[1], SHUT_RDWR);
		m_pLog->Info("CreateThreadGroup error.times=%d;", flag);
		return FUNCTION_ERROR;;
	}

	if (SetSocketCache(sockpair[0], SO_SNDBUF)
			|| SetSocketCache(sockpair[1], SO_RCVBUF)) {
		shutdown(sockpair[0], SHUT_RDWR);
		shutdown(sockpair[1], SHUT_RDWR);
		p_thread->interrupt();
		m_ThreadGroup.remove_thread(p_thread);
		m_pLog->Info("SetSocketCache false!!");
		return FUNCTION_ERROR;;
	}

	uuid_generate(uid);
	uuid_unparse(uid, uuid);
	threadnode.threadflag = flag;
	threadnode.wfd = sockpair[0];
	threadnode.rfd = sockpair[1];
	threadnode.ptr = p_thread;
	threadnode.nodedevnum = 0;
	threadnode.uuid = uuid;
	m_ThreadList.push_front(threadnode);

	m_pLog->Info("CAThreadManager::CreateThread Success--%d!", m_ThreadList.size());

	return FUNCTION_SUCCESS;
}

/*
 * par: 崩溃线程的线程标记
 * 后期可以采用信号的方式触发
 * Ver 1.3.1 after no used
 */
void CAThreadManager::ModifyThreadList(int tflag) {
	m_ThreadMutex.lock();

	std::list<ThreadList>::iterator itor;
	if (m_ThreadList.empty()) {
		m_ThreadMutex.unlock();
		return;
	}
	itor = m_ThreadList.begin();
	while (itor != m_ThreadList.end()) {
		if ((*itor).threadflag == tflag) {
			m_DeadThreadList.push_back((*itor));
			m_ThreadList.erase(itor);
			break;
		}
	}

	m_ThreadMutex.unlock();
}

/*
 * don't test
 * Returns >0 socketfd, -1 for errors
 */
int CAThreadManager::RetrunSocketPairfd()
{
	m_ThreadMutex.lock();

	std::list<ThreadList>::iterator itor;
	if (m_ThreadList.empty()) {
		m_ThreadMutex.unlock();
		return FUNCTION_ERROR;
	}
	itor = m_ThreadList.begin();
	while (itor != m_ThreadList.end()) {
		if ((*itor).nodedevnum < m_MaxNodeNums) {
			(*itor).nodedevnum++;
			m_pLog->Debug(
					"Add a task to thread SocketPairfd: %d, Tasks on thread node: %d",
					(*itor).wfd, (*itor).nodedevnum);
			m_ThreadMutex.unlock();
			return (*itor).wfd;
		}
		itor++;
	}
	m_MaxNodeNums++;
	(*m_ThreadList.begin()).nodedevnum++;
	m_pLog->Info(
			"Add a task to thread SocketPairfd: %d, Tasks on thread node: %d",
			(*m_ThreadList.begin()).wfd, (*m_ThreadList.begin()).nodedevnum);

	m_ThreadMutex.unlock();
	return (*m_ThreadList.begin()).wfd;
}

/*
 * par: write socket fd;
 * ret: m_ThreadList.nodedevnums Success,
 * 		unfind is -1;
 */
int CAThreadManager::DecreasSocketPairfd(int socketwfd) {
	m_ThreadMutex.lock();

	std::list<ThreadList>::iterator itor;
	std::list<ThreadList>::iterator ritor;
	if (m_ThreadList.empty()) {
		m_ThreadMutex.unlock();
		return FUNCTION_ERROR;
	}
	ritor = m_ThreadList.end();
	itor = m_ThreadList.begin();
	m_MaxNodeNums = (*itor).nodedevnum;
	while (itor != m_ThreadList.end()) {
		//全比较,保证了m_MaxNodeNums可以保持变化
		m_MaxNodeNums =
				m_MaxNodeNums < (*itor).nodedevnum ?
						(*itor).nodedevnum : m_MaxNodeNums;
		if ((*itor).wfd == socketwfd) {
			(*itor).nodedevnum ? (*itor).nodedevnum-- : 0;
			ritor = itor;
			m_pLog->Info(
					"Delete a task on thread SocketPairfd: %d, Tasks on thread node: %d",
					socketwfd, (*itor).nodedevnum);
		}
		itor++;
	}

	if (ritor != m_ThreadList.end()) {
		m_ThreadMutex.unlock();
		return (*ritor).nodedevnum;
	}

	m_ThreadMutex.unlock();
	return FUNCTION_ERROR;
}

/*
 * par: recv data buffer
 * memory Each triple, signal thread use
 * Returns 0 for success, -1 for errors
 * function return pointer value is empty
 */
int CAThreadManager::ReallocMemory(unsigned char** first, size_t fsize,
		unsigned char** second, size_t ssize) {
	static size_t framesize, extrasize;
	if (!first || !second) {
		return FUNCTION_ERROR;
	}
	m_ReallocMutex.lock();

	if (framesize < fsize) {
		framesize = fsize * 2;
		*first = (unsigned char*) realloc(*first, framesize);
		memset(*first, 0, framesize);
	} else {
		memset(*first, 0, framesize);
	}
	if (extrasize < ssize) {
		extrasize = ssize * 2;
		*second = (unsigned char*) realloc(*second, extrasize);
		memset(*second, 0, extrasize);
	} else {
		memset(*second, 0, extrasize);
	}
	m_pLog->Info("Two parameters ReallocMemory new size success;\n"
			"framesize=%d,extrasize=%d", framesize, extrasize);

	m_ReallocMutex.unlock();
	return FUNCTION_SUCCESS;
}

/*
 * Ver 1.3.1 after retain
 * boost 设计问题,如果采用joinall无法后期添加线程
 * 功能: 循环扫描业务线程数量,同时保证线程数量,
 * 检查频率不需要太高,初设置为5分钟,同时打印当前数
 * 所以在主线程中join本线程
 * 1.4.2 add ENABLE_UPGRADE_CODE
 */
void CAThreadManager::MaintainRun() {
	std::list<ThreadList>::iterator itor;
	std::list<ThreadList>::iterator tmp;
	while (1) {
		m_ThreadMutex.lock();
		itor = m_DeadThreadList.begin();
		while (itor != m_DeadThreadList.end()) {
			if (CreateThread((*itor).threadflag)) {
				m_pLog->Info("DynamicAddThread False;threadflag=%d",
						(*itor).threadflag);
				itor++;
			} else {
				m_ThreadGroup.remove_thread((*itor).ptr);
				m_DeadThreadList.erase((tmp = itor++));
			}
		}

#if ENABLE_UPGRADE_CODE
		/*
		 * get mdvr info node nums
		 * if 0 can upgrade
		 */
		if (!G_GetMdvrInfoListNums())
		{
			if (-1 == m_Upgrade.CheckUpgradeFileExist()) {
				;
			}
			else if (-1 == m_Upgrade.CheckUpgradeApplication()) {
				;
			}
			else
			{
				m_ThreadMutex.unlock();
				exit(1);
			}
		}
#endif
		m_ThreadMutex.unlock();
		sleep(300);
	}
}

/*
 * 业务处理部分
 * par: 接收数据的套接子
 * 数据有可能丢失,recvmsg函数没有进行接收数据异常处理
 * MSG_WAITALL 保证大数据的完整接收
 * 一种方式是采用socket回写到方式告知上层线程退出处理,
 * 由于时间问题,本次采用全局函数处理
 * 由于编解码类需要第一个I帧才能获取相关视频信息,所以需要修改线程外的数据资源
 */
void CAThreadManager::Run(int flag, int socketwfd, int socketrfd) {

	m_pLog->Debug("Thread is Running;writefd=%d;readfd=%d;ThreadId=%lu",\
		socketwfd, socketrfd, pthread_self());

	int mdvrs_in_avcodec;
	int wfd = socketwfd;
	//视频请求唯一prefix和编解码的映射关系
	Mdvr_Avcodec_Map map_mdvr_avcodec;
	CAVideoTrans *avcodec = NULL;
	int retcount = 0;
	size_t dyframeLen = 0;
	size_t dyextraLen = 0;
	size_t frameLen = 1024 * 40;
	size_t extraLen = 1024 * 3;

	unsigned char* frameData = (unsigned char*) malloc(
			frameLen * sizeof(unsigned char));
	memset(frameData, 0, frameLen);
	unsigned char* extraData = (unsigned char*) malloc(
			extraLen * sizeof(unsigned char));
	;
	memset(extraData, 0, extraLen);

	pMdvrinfo pmdvrinfo = new Mdvrinfo;
	struct msghdr msgr;
	struct iovec iovr[3];
	bzero(&msgr, g_msghdr_size);
	msgr.msg_iov = iovr;
	msgr.msg_iovlen = 3;

	while (1) {
		dyframeLen = 0;
		dyextraLen = 0;
		memset(pmdvrinfo, 0, g_mdvrinfo_size);

		iovr[0].iov_base = NULL;
		iovr[0].iov_len = 0;
		iovr[1].iov_base = (size_t*) &dyframeLen;
		iovr[1].iov_len = sizeof(size_t);
		iovr[2].iov_base = (size_t*) &dyextraLen;
		iovr[2].iov_len = sizeof(size_t);
		retcount = recvmsg(socketrfd, &msgr, MSG_WAITALL);

		if (frameLen < dyframeLen && dyframeLen < 100000) {
			frameLen = dyframeLen * 2;
			if (frameData && frameLen) {
				frameData = (unsigned char*) realloc(frameData, frameLen);
				memset(frameData, 0, frameLen);
				m_pLog->Info("frameData realloc ok");
			}
		}
		if (extraLen < dyextraLen && dyextraLen < 100000) {
			extraLen = dyextraLen * 2;
			if (extraData && extraLen) {
				extraData = (unsigned char*) realloc(extraData, extraLen);
				memset(extraData, 0, extraLen);
				m_pLog->Info("extraData realloc ok");
			}
		}

		iovr[0].iov_base = pmdvrinfo;
		iovr[0].iov_len = g_mdvrinfo_size;
		iovr[1].iov_base = frameData;
		iovr[1].iov_len = dyframeLen;
		iovr[2].iov_base = extraData;
		iovr[2].iov_len = dyextraLen;
		retcount = recvmsg(socketrfd, &msgr, MSG_WAITALL);

#if 0
		if (-1 == retcount || 0 == dyframeLen || 0 == dyextraLen \
				|| ((size_t) retcount)!= g_mdvrinfo_size + dyframeLen + dyextraLen)
		{
			m_pLog->Error("recvmsg error! recieve data: %d;frame size: %d; extra size: %d;",\
					retcount, dyframeLen, dyextraLen);
			continue;
		}
#endif
		avcodec = OperMdvrAvCodecMap(pmdvrinfo, map_mdvr_avcodec);
		if (!avcodec) {
			m_pLog->Error(
					"OperMdvrAvCodecMap();Prefix[%s]Handle[%s],avcodec is null",
					pmdvrinfo->zmqData.prefix, pmdvrinfo->zmqData.handle);
			continue;
		}

//		fill data to AVcodecTrans
		m_pLog->Debug("dyframeLen= %d\n", dyframeLen);
		if (avcodec->TransH264ToTs(pmdvrinfo, g_mdvrinfo_size, frameData,
				dyframeLen, extraData, dyextraLen))
		{
			pmdvrinfo->zmqData.sendFlag = FRAME_DATA_STOP;
			//递减socket引用计数
			mdvrs_in_avcodec = DecreasSocketPairfd(pmdvrinfo->socketWfd);
			int satatus = G_ModifyMdvrInfoNode(pmdvrinfo->zmqData.handle, wfd,
					MDVRINFO_SOCKET_DEL);
			m_pLog->Error("TransH264ToTs false! modifyNodeNums:%d",satatus);
		}

		if (FRAME_DATA_STOP == pmdvrinfo->zmqData.sendFlag) {
			m_pLog->Info(
					"Trans Over AVcodecTrans Map Delete;SessionId[%s],midtype[%d];",
					pmdvrinfo->zmqData.handle, pmdvrinfo->zmqData.midtype);
			OperMdvrAvCodecMap(pmdvrinfo, map_mdvr_avcodec);
			delete avcodec;
			avcodec = NULL;
		}
	}
	free(frameData);
	free(extraData);
	delete pmdvrinfo;
}

/*
 * par: mdvr相关节点信息
 * 		mdvr-avcodec映射关系表的引用
 * ret: 成功返回相关编解码类指针,返回NULL表示失败
 */
CAVideoTrans* CAThreadManager::OperMdvrAvCodecMap(pMdvrinfo pmdvrinfo,
		Mdvr_Avcodec_Map &mapma) {
	CAVideoTrans *pavcodec = NULL;
	if (FRAME_DATA_START == pmdvrinfo->zmqData.sendFlag) {
		try {
			pavcodec = new CAVideoTrans(read_packet, write_packet);
			mapma.insert(
					Mdvr_Avcodec_Map::value_type(pmdvrinfo->zmqData.handle,
							pavcodec));
		} catch (const CAException &excp) {
			m_pLog->Info("new CAAvCodeTrans false;Error=%s", excp.what());
			pavcodec = NULL;
		}
	} else if (FRAME_DATA_TRANS == pmdvrinfo->zmqData.sendFlag) {
		Mdvr_Avcodec_Map::iterator itor;
		for (itor = mapma.begin(); itor != mapma.end(); itor++) {
			if (!(*itor).first.compare(pmdvrinfo->zmqData.handle)) {
				pavcodec = (*itor).second;
				break;
			}
		}
	} else if (FRAME_DATA_STOP == pmdvrinfo->zmqData.sendFlag) {
		Mdvr_Avcodec_Map::iterator itor;
		for (itor = mapma.begin(); itor != mapma.end(); itor++) {
			if (!(*itor).first.compare(pmdvrinfo->zmqData.handle)) {
				//can not delete pavcodec in here it must send a null packet to present this task is complete
				pavcodec = (*itor).second;
//				delete pavcodec;
//				pavcodec = NULL;
				mapma.erase(itor);
				break;
			}
		}
	}
	return pavcodec;
}


CAThreadManager::~CAThreadManager() {
	m_pLog->Debug("CAThreadManager::~CAThreadManager()");
}

CAThreadManager::CAThreadManager() :
		m_ThreadMutex(g_data.ZlogRmCname), m_ReallocMutex(g_data.ZlogRmCname) {
	m_MaxNodeNums = 1;
	m_ThreadList.clear();
	m_DeadThreadList.clear();
	m_pMaintainThread = NULL;
	m_pLog = CALog::GetInstance(g_data.ZlogCname);
	m_pLog->Debug("CAThreadManager::CAThreadManager()!!!!!");
}

