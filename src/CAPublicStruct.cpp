/*
 * CAPublicStruct.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: diwenjie
 */

#include "CAPublicStruct.h"
//#include "CALog.h"

g_Data g_data;
struct msghdr g_msghdr;
size_t g_msghdr_size;
size_t g_mdvrinfo_size;
size_t g_zmqdata_size;
struct iovec g_iov[3];
Mdvrinfo *g_pmdvrinfo_head = NULL;
Mdvrinfo *g_pmdvrinfo_pre = NULL;
Mdvrinfo *g_pmdvrinfo_tail = NULL;
pthread_mutex_t g_mdvrinfo_mutex;

#if USE_TEST_FUNCTION

void G_TestFunction()
{
	struct rlimit rlim;
	getrlimit(RLIMIT_NOFILE,&rlim);
	rlim.rlim_max = 65536;
	rlim.rlim_cur = 65535;
	setrlimit(RLIMIT_NOFILE,&rlim);

	int newprio = 20;
	pthread_attr_t attr;
	sched_param param;
	pthread_attr_init(&attr);
	pthread_attr_getschedparam(&attr,&param);
	param.__sched_priority = newprio;
	pthread_attr_setschedparam(&attr,&param);
}

#endif

/*
 * 获取eth0网卡到ip
 */
string G_GetLocalIpAddr() {
	int fd;
	unsigned int addr;
	struct ifreq ifr;
	struct sockaddr_in *sin;
	string localip;
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, "eth0");
	ioctl(fd, SIOCGIFADDR, &ifr);
	sin = (struct sockaddr_in*) &ifr.ifr_addr;
	addr = sin->sin_addr.s_addr;
	char strip[5] = { 0 };
	memset(strip, 0, 5);
	memcpy(strip, (void*) &addr, 4);
	close(fd);
	if (!addr)
		return "";
	for (fd = 0; fd < 3; fd++) {
		localip += boost::lexical_cast<string>(0xff & strip[fd]);
		localip += ".";
	}
	localip += boost::lexical_cast<string>(0xff & strip[fd]);
	return localip;
}

/*
 * Current only a SKD manufacturer
 * pSdkConfNode list no dynamic add,
 * 1.2Ver laster realization
 * ip: 8.8.8.8
 */
int G_InitGlobalDefine(Config* cfg, SdkConfInfo* pSdkConfNode) {
	int sdkcount = cfg->lookup("SDkCount");
	g_data.ThreadNums = cfg->lookup("ThreadNums");
	g_data.EnableDaemon = cfg->lookup("EnableDaemon");
	int scmultiple = cfg->lookup("SocketCacheMultiple");
	string redisip = cfg->lookup("RedisIp");
	g_data.RedisPort = cfg->lookup("RedisPort");
	string localip = G_GetLocalIpAddr();
	if (localip.empty()) {
		string ip = cfg->lookup("LocalIp");
		g_data.LocalIp = ip;
	} else {
		g_data.LocalIp = localip;
	}
	string version = cfg->lookup("Version");
	string zlogconfpath = cfg->lookup("ZlogConfPath");
	string zlogcname = cfg->lookup("ZlogCname");
	string zlogrmcname = cfg->lookup("ZlogRmCname");
	string zlogformat = cfg->lookup("ZlogFormat");
	string avoutputformat = cfg->lookup("AVoutputFormat");

	g_data.SocketCacheMultiple = FFMAX(1, scmultiple);
	g_data.pSci = NULL;
	g_data.RedisIp = redisip;
	g_data.SDkCount = sdkcount;
	g_data.Version = version;
	g_data.ZlogCname = zlogcname;
	g_data.ZlogFormat = zlogformat;
	g_data.ZlogRmCname = zlogrmcname;
	g_data.ZlogConfPath = zlogconfpath;
	g_data.AvOutputFormat = avoutputformat;

	while (sdkcount--) {
		if (pSdkConfNode) {
			string sdkname = cfg->lookup("RM-SDKname");
			pSdkConfNode->sdkname = sdkname;
			if (localip.empty()) {
				string sdkip = cfg->lookup("RM-SDKip");
				pSdkConfNode->sdkip = sdkip;
			} else {
				pSdkConfNode->sdkip = localip;
			}
			string sdkid = cfg->lookup("RM-SDKid");
			pSdkConfNode->sdkid = sdkid;
			pSdkConfNode->EnableSdkLog = cfg->lookup("RM-EnableSdkLog");
			pSdkConfNode->zmqport = cfg->lookup("RM-ZeroMQPort");
			pSdkConfNode->downzmqport = cfg->lookup("RM-DownZMQPort");
			pSdkConfNode->timeout = cfg->lookup("RM-SDKConnTimeout");
			pSdkConfNode->sdkport = cfg->lookup("RM-SDKport");
			pSdkConfNode->next = NULL;
		} else {
			return FUNCTION_ERROR;
		}
	}

	//initializer socket message structure
	g_msghdr_size = sizeof(g_msghdr);
	bzero(&g_msghdr, g_msghdr_size);
	g_msghdr.msg_iov = g_iov;
	g_msghdr.msg_iovlen = 3;
	g_mdvrinfo_size = sizeof(Mdvrinfo);
	g_zmqdata_size = sizeof(ZmqData);
	G_SetSystemSignal();
	G_GetCurrentPath(g_data.CurrentPath);

	if (pthread_mutex_init(&g_mdvrinfo_mutex,NULL))
		return FUNCTION_ERROR;
	return FUNCTION_SUCCESS;
}

void G_GetCurrentPath(string &path) {
	char cpwd[1024] = { 0 };
	getcwd(cpwd, 1023);
	path = cpwd;
	path += "/";
	return;
}

/*
 * set open file limit
 */
void G_SetSecurityLimit(int item, int curvalue, int maxvalue) {
	struct rlimit rlim;

	if (getrlimit(item, &rlim))
		return;

	rlim.rlim_max = maxvalue;
	rlim.rlim_cur = curvalue;
	setrlimit(item, &rlim);
}

/*
 * Run the function exits when the main program
 * Logging and Exception Handing
 */
void G_FinitGlobalData(void) {
}

/*
 * set process about info.
 * sigpipe: write already close socket trigger
 * sigchld: child process to exit the sending to parent process
 *  Because concurrent server often fork a lot of the child process
 *  ,child process after the end of the need to clean up resources
 *  server process wait.if the treatment methods of this signals is
 *  set to ignore,can allow the kernel to zombie child processes transferred
 *  to the init process,saves a lot of zombie process occupies system resources.
 */
void G_SetSystemSignal() {
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGABRT, SIG_IGN);
	signal(SIGSEGV, SIG_IGN);
	setbuf(stdout, NULL);
}

/*
 * 休眠 单位：毫秒
 */
#ifndef WIN32
void G_Sleep(int ms_time) {
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = ms_time * 1000;
	select(1, NULL, NULL, NULL, &tv);
}
#else
void G_Sleep(int ms_time)
{
	Sleep(ms_time);
}
#endif

/*
 * 检查业务程序是否健在
 */
int G_PidExist(int dst_pid) {
	return !kill(dst_pid, 0);
}

/*
 * Monitoring daemon process function
 * par: Program absolute path
 * ret: ENABLE_DAEMON_ERROR is Daemon error, 0 is success
 */
int G_ShadowDaemon(char *name) {
	char *p;
	pid_t shadow_pid = -1;
	pid_t new_pid = -1;
	pid_t monitor_pid = -1;
	string upgradeFileName;

	if (!name)
		return ENABLE_DAEMON_ERROR;

#if ENABLE_UPGRADE_CODE
	upgradeFileName = name;
	upgradeFileName += "_tmp";
#endif

	monitor_pid = getpid();
	shadow_pid = fork();
	if (shadow_pid < 0)
		return ENABLE_DAEMON_ERROR;
	if (shadow_pid > 0)
		return GLOBAL_FUNCTION_NO_ERROR;

	for (; true; G_Sleep(10 * 1000)) {
		if (G_PidExist(monitor_pid))
			continue;

#if ENABLE_UPGRADE_CODE
		if (!access(upgradeFileName.c_str(), F_OK)) {
			string modify = "mv -f " + upgradeFileName;
			modify += " ";
			modify += name;
			system(modify.c_str());
			system("rm -rf upgrade/*");
			if (!access(upgradeFileName.c_str(), F_OK)) {
				modify = "\\cp -r /opt/";
				modify += PROGRAMNAME;
				modify += " ./";
				system(modify.c_str());
				system("rm -rf /etc/ld.so.conf");
				system("mv /etc/ld.so.conf_bak /etc/ld.so.conf");
				system("\\cp -r /opt/VideoAccess.conf ./");
				system("\\cp -r /opt/VideoZlog.conf ./");
				system("touch upgrade/false.txt");
			} else {
				system("rm -rf /etc/ld.so.conf_bak");
				string shellname = g_data.CurrentPath + SHELLFILENAME;
				system(shellname.c_str());
				system("touch upgrade/success.txt");
			}
		}
#endif

		new_pid = fork();
		if (new_pid < 0)
			continue;
		if (new_pid > 0) {
			monitor_pid = new_pid;
			continue;
		}

		if ((p = strrchr(name, '/')) == NULL)
			p = name;
		else
			p++;

		execl(name, p, "-r", NULL);
	}
	return GLOBAL_FUNCTION_NO_ERROR;
}

/*
 * 设置当前程序为后台进程
 */
int G_EnableDaemon(void) {
	//daemon start
	pid_t pid;
	int fd = 0, max_fd = 0;

	if ((pid = fork()) < 0)
		return ENABLE_DAEMON_ERROR;
	else if (pid != 0)
		exit(1);
	if (setsid() < 0)
		return ENABLE_DAEMON_ERROR;
	if (fork())
		exit(1);

	max_fd = getdtablesize();
	for (fd = 0; fd < max_fd; ++fd)
		close(fd);
	umask(0);

	signal(SIGCHLD, SIG_IGN);
	//daemon ok
	return GLOBAL_FUNCTION_NO_ERROR;
}

/*
 * par: mdvr info about data;;exp: 请求编号,芯片号,连接类型,通道号,码流类型,警情ID
 * ret: int 返回0成功,否则返回错误码
 * add socketpair实现全双工通信 和 线程池资源分配
 * prefix: -D is Downfile; -A Alarm; other is request1
 * mdvr 下发指令nStreamID=1,sdk回调过来到数据nStreamID=0,做一次转换
 */
int G_AddMdvrInfoList(void *pHandle, UUID_T nSessionID, char *szMDVRID,
		int nConnectType, int nChannelID, int nStreamID, char *szAlarmID,
		int socketfd)
{
	CALog* plog = CALog::GetInstance(g_data.ZlogCname);

	if (!pHandle || !szMDVRID)
		return ADD_MDVR_LIST_NEWMEMORY_ERROR;

	pthread_mutex_lock(&g_mdvrinfo_mutex);
	if (!g_pmdvrinfo_head) {
		g_pmdvrinfo_head = new Mdvrinfo;
		if (!g_pmdvrinfo_head) {
			pthread_mutex_unlock(&g_mdvrinfo_mutex);
			return ADD_MDVR_LIST_NEWMEMORY_ERROR;
		}
		else
		{
			plog->Debug("G_AddMdvrInfoList() ------g_pmdvrinfo_head!!!!!!!!!!!!!!!");
		}
		memset(g_pmdvrinfo_head, 0, g_mdvrinfo_size);
		g_pmdvrinfo_pre = g_pmdvrinfo_head;
	}

	g_pmdvrinfo_tail = new Mdvrinfo;
	if (!g_pmdvrinfo_tail) {
		pthread_mutex_unlock(&g_mdvrinfo_mutex);
		return ADD_MDVR_LIST_NEWMEMORY_ERROR;
	}

	memset(g_pmdvrinfo_tail, 0, g_mdvrinfo_size);
	g_pmdvrinfo_tail->next = NULL;
	g_pmdvrinfo_pre->next = g_pmdvrinfo_tail;
	g_pmdvrinfo_pre = g_pmdvrinfo_tail;

//	string prefix = szMDVRID;
//	prefix += ":";
//	prefix += boost::lexical_cast<string>(nChannelID);

//	if (MDVR_FILE_DOWNLOAD_CONNECT == nConnectType) {
//		prefix += ":D";
//		memcpy(g_pmdvrinfo_tail->zmqData.prefix, prefix.c_str(),
//		PREFIXSIZE + 2);
//	} else {
//		if (MDVR_VIDEO_MAIN_STREAM == nStreamID)
//			nStreamID = MDVR_VIDEO_CHILD_STREAM;
//		memcpy(g_pmdvrinfo_tail->zmqData.prefix, prefix.c_str(),
//		PREFIXSIZE + 2);
//	}
	if (MDVR_FILE_DOWNLOAD_CONNECT != nConnectType) {
		if (MDVR_VIDEO_MAIN_STREAM == nStreamID)
			nStreamID = MDVR_VIDEO_CHILD_STREAM;
	}
	string prefix = boost::lexical_cast<string>(nChannelID);
	prefix += ":";
	prefix += boost::lexical_cast<string>(nStreamID);
	prefix += ":";
	prefix += szMDVRID;

	memcpy(g_pmdvrinfo_tail->zmqData.prefix, prefix.c_str(),PREFIXSIZE + 2);
//	std::cout << g_pmdvrinfo_tail->zmqData.prefix << std::endl;
	memcpy(g_pmdvrinfo_tail->zmqData.handle, (char*) pHandle, HANDLESIZE);
	memcpy(g_pmdvrinfo_tail->zmqData.uuid, nSessionID, HANDLESIZE);
	memcpy(g_pmdvrinfo_tail->zmqData.mdvrid, szMDVRID, MDVRIDSIZE);
	memcpy(g_pmdvrinfo_tail->zmqData.alarmid, szAlarmID, ALARMIDSIZE);
	g_pmdvrinfo_tail->zmqData.sendFlag = FRAME_DATA_WAIT;
	g_pmdvrinfo_tail->zmqData.midtype = nConnectType;
	g_pmdvrinfo_tail->zmqData.channelid = nChannelID;
	g_pmdvrinfo_tail->zmqData.streamid = nStreamID;
	g_pmdvrinfo_tail->socketWfd = socketfd;

	pthread_mutex_unlock(&g_mdvrinfo_mutex);
	return GLOBAL_FUNCTION_NO_ERROR;
}

/*
 * par: mdvr info about data;;exp: 请求唯一编号,任务类型
 * ret: int 返回-1失败,成功返回对应到写socketpair写描述符
 */
int G_DelMdvrInfoList(void *pHandle, int connectType) {
//	CALog* plog = CALog::GetInstance(g_data.ZlogCname);
//	plog->Debug("G_DelMdvrInfoList() ------begin!");
	int socketwfd = -1;
	pthread_mutex_lock(&g_mdvrinfo_mutex);
//	plog->Debug("G_DelMdvrInfoList() ----add lock");
	if (!g_pmdvrinfo_head) {
		pthread_mutex_unlock(&g_mdvrinfo_mutex);
		return socketwfd;
	}
//	plog->Debug("G_DelMdvrInfoList() ----g_pmdvrinfo_head");
	pMdvrinfo node = g_pmdvrinfo_head->next;
//	plog->Debug("G_DelMdvrInfoList() ----node");
	pMdvrinfo pre = g_pmdvrinfo_head;
//	plog->Debug("G_DelMdvrInfoList() ----pre");
	while (node) {
		if (!strncasecmp(node->zmqData.handle, (char*) pHandle, HANDLESIZE)) {
			G_SendLatestData(node);
			socketwfd = node->socketWfd;
			pre->next = node->next;
			if (!pre->next)
				g_pmdvrinfo_pre = pre;
			delete node;
			node = NULL;
			break;
		}
		pre = node;
		node = node->next;
	}
//	plog->Debug("G_DelMdvrInfoList() ----while (node)");
	pthread_mutex_unlock(&g_mdvrinfo_mutex);
//	plog->Debug("G_DelMdvrInfoList() ------end!");
	return socketwfd;
}

/*
 * modify....
 * par: write Socketfd description
 * ret: modify nums, others is 0
 */
int G_ModifyMdvrInfoNode(void *pHandle, int socketwfd, int type) {
	int modifyNodeNums = 0;
//	CALog* plog = CALog::GetInstance(g_data.ZlogCname);
//	plog->Debug("G_ModifyMdvrInfoNode() ------begin!");
	if (!g_pmdvrinfo_head) {
		return modifyNodeNums;
	}
//	plog->Debug("G_ModifyMdvrInfoNode() ------g_pmdvrinfo_head!");
//	pthread_mutex_lock(&g_mdvrinfo_mutex);
//	plog->Debug("G_ModifyMdvrInfoNode() ------g_mdvrinfo_mutex!");
	pMdvrinfo node = g_pmdvrinfo_head->next;
//	plog->Debug("G_ModifyMdvrInfoNode() ------node!");
	if (!node) {
//		pthread_mutex_unlock(&g_mdvrinfo_mutex);
		return GLOBAL_FUNCTION_NO_ERROR;
	}

	while (node) {
		if (MDVRINFO_SOCKET_DEL == type) {
			if (!strncasecmp(node->zmqData.handle, (char*) pHandle, HANDLESIZE)
					&& node->socketWfd == socketwfd) {
				pthread_mutex_lock(&g_mdvrinfo_mutex);
				node->zmqData.sendFlag = FRAME_DATA_WAIT;
//				node->socketWfd = -1;
				modifyNodeNums++;
				pthread_mutex_unlock(&g_mdvrinfo_mutex);
				break;
			}
		} else if (MDVRINFO_SOCKET_ADD == type) {
			if (!strncasecmp(node->zmqData.handle, (char*) pHandle, HANDLESIZE)
					&& -1 == node->socketWfd) {
				pthread_mutex_lock(&g_mdvrinfo_mutex);
				modifyNodeNums++;
				node->socketWfd = socketwfd;
				pthread_mutex_unlock(&g_mdvrinfo_mutex);
				break;
			}
		}
		node = node->next;
	}
//	plog->Debug("G_ModifyMdvrInfoNode() ------while (node)!");
//	pthread_mutex_unlock(&g_mdvrinfo_mutex);
//	plog->Debug("G_ModifyMdvrInfoNode() ------end!");
	return modifyNodeNums;
}

int G_GetMdvrInfoListNums() {
	int nodeNums = 0;

	if (!g_pmdvrinfo_head) {
		return nodeNums;
	}

//	pthread_mutex_lock(&g_mdvrinfo_mutex);
	pMdvrinfo node = g_pmdvrinfo_head->next;
	if (!node) {
//		pthread_mutex_unlock(&g_mdvrinfo_mutex);
		return 0;
	}

	while (node) {
		nodeNums++;
		node = node->next;
	}
//	pthread_mutex_unlock(&g_mdvrinfo_mutex);
	return nodeNums;
}

/*
 * send'd data don't copy
 * Specific implementation can be reference for advanced data format
 * iovec and msghdr is BSD socket function
 */
int G_SendMdvrRealData(void *pHandle, int nFrameType, unsigned int nFrameLen,
		unsigned char* pFrameData, long long nPTS, unsigned char* nRTC, int nFileOffset,
		unsigned char * pExtraDataBuf, unsigned int nExtraDataLen) {
	if (!pHandle || !g_pmdvrinfo_head) {
		return MDVR_HANDLE_INVALID;
	}

	CALog* plog = CALog::GetInstance(g_data.ZlogCname);
//	pthread_mutex_lock(&g_mdvrinfo_mutex);
	pMdvrinfo node = g_pmdvrinfo_head->next;
	if (!node) {
//		pthread_mutex_unlock(&g_mdvrinfo_mutex);
		return MDVR_LIST_IS_EMPTY;
	}

	while (node) {
		if (!strncasecmp(node->zmqData.handle, (char*) pHandle, HANDLESIZE)) {
			if (-1 == node->socketWfd) {
//				pthread_mutex_unlock(&g_mdvrinfo_mutex);
			    plog->Debug("G_SendMdvrRealData() SEND_MDVR_REAL_DATA_ERROR!!!");
				return SEND_MDVR_REAL_DATA_ERROR;
			}

			pthread_mutex_lock(&g_mdvrinfo_mutex);
		    plog->Debug("come in G_SendMdvrRealData() node->zmqData.sendFlag=%d\n!!!", node->zmqData.sendFlag);

			node->pts = nPTS;
//			node->frameUTC = nUTC;
			node->frameType = nFrameType;
			node->zmqData.fileOffset = nFileOffset;

			if (FRAME_DATA_WAIT == node->zmqData.sendFlag)
			{
				string time = "";
				//重连机制保证传输数据是I帧类型
				if (FRAME_TYPE_I_FRAME != nFrameType) {
					pthread_mutex_unlock(&g_mdvrinfo_mutex);
					plog->Debug("G_SendMdvrRealData() GLOBAL_FUNCTION_NO_ERROR!");
					return GLOBAL_FUNCTION_NO_ERROR;
				}
				if (MDVR_FILE_DOWNLOAD_CONNECT != node->zmqData.midtype) {
//					G_GetCurrentTimeString(time, nUTC, YMDHMS);
//					memcpy(node->zmqData.starttime, time.c_str(),
//					TIMEFORMATSIZE);
//					memcpy(node->zmqData.endtime, time.c_str(), TIMEFORMATSIZE);
				}
				node->zmqData.sendFlag = FRAME_DATA_START;
			}
			else if (FRAME_DATA_START == node->zmqData.sendFlag)
			{
				node->zmqData.sendFlag = FRAME_DATA_TRANS;
			}

			//可以保证存储拿到最后一帧数据是I帧的结束时间
			if (FRAME_TYPE_I_FRAME == nFrameType) {
				if (MDVR_FILE_DOWNLOAD_CONNECT != node->zmqData.midtype) {
//					string time = "";
//					G_GetCurrentTimeString(time, nUTC, YMDHMS);
//					memcpy(node->zmqData.endtime, time.c_str(), TIMEFORMATSIZE);
				}
			}

			plog->Debug("G_SendMdvrRealData() nFrameLen==%d  nPTS==%lld-----start!!", nFrameLen, nPTS);

			g_iov[0].iov_base = NULL;
			g_iov[0].iov_len = 0;
			g_iov[1].iov_base = (size_t*) &nFrameLen;
			g_iov[1].iov_len = sizeof(size_t);
			g_iov[2].iov_base = (size_t*) &nExtraDataLen;
			g_iov[2].iov_len = sizeof(size_t);
			sendmsg(node->socketWfd, &g_msghdr, 0);

			g_iov[0].iov_base = node;
			g_iov[0].iov_len = g_mdvrinfo_size;
			g_iov[1].iov_base = pFrameData;
			g_iov[1].iov_len = nFrameLen;
			g_iov[2].iov_base = pExtraDataBuf;
			g_iov[2].iov_len = nExtraDataLen;
			//g_msg.msg_flags = 3;
			sendmsg(node->socketWfd, &g_msghdr, 0);
			pthread_mutex_unlock(&g_mdvrinfo_mutex);
			break;
		}
		node = node->next;
	}
//	pthread_mutex_unlock(&g_mdvrinfo_mutex);
	return GLOBAL_FUNCTION_NO_ERROR;
}

int G_SendLatestData(pMdvrinfo node) {
	char pFrameData[LASTFRAMESIZE] = "Invalid";
	char pExtraData[LASTFRAMESIZE] = "Invalid";
	size_t FrameLen = LASTFRAMESIZE;
	size_t ExtraDataLen = LASTFRAMESIZE;

	if (-1 == node->socketWfd) {
		return SEND_MDVR_REAL_DATA_ERROR;
	}

	node->zmqData.fActualSize = 0;
	node->frameType = FRAME_TYPE_UNKNOWN;
	node->zmqData.sendFlag = FRAME_DATA_STOP;

	g_iov[0].iov_base = NULL;
	g_iov[0].iov_len = 0;
	g_iov[1].iov_base = (size_t*) &FrameLen;
	g_iov[1].iov_len = sizeof(size_t);
	g_iov[2].iov_base = (size_t*) &ExtraDataLen;
	g_iov[2].iov_len = sizeof(size_t);
	sendmsg(node->socketWfd, &g_msghdr, 0);

	g_iov[0].iov_base = node;
	g_iov[0].iov_len = g_mdvrinfo_size;
	g_iov[1].iov_base = pFrameData;
	g_iov[1].iov_len = FrameLen;
	g_iov[2].iov_base = pExtraData;
	g_iov[2].iov_len = ExtraDataLen;
	//g_msg.msg_flags = 3;
	sendmsg(node->socketWfd, &g_msghdr, 0);

	return GLOBAL_FUNCTION_NO_ERROR;
}

int G_ModifyFileDownInfoList(char* SessionID, int nMDVRName, char* szFileName,
		int nStateCode, int nStartOffset, int nStopOffset, int nFlagRecordType,
		time_t nFileSliceStartUTC, time_t nFileSliceStopUTC) {
	string time = "";

	if (!szFileName || !g_pmdvrinfo_head) {
		return ADD_MDVR_LIST_FILEDOWN_ERROR;
	}

	pthread_mutex_lock(&g_mdvrinfo_mutex);
	pMdvrinfo node = g_pmdvrinfo_head->next;
	if (!node) {
		pthread_mutex_unlock(&g_mdvrinfo_mutex);
		return ADD_MDVR_LIST_FILEDOWN_ERROR;
	}

	while (node) {
		if (!strncasecmp(node->zmqData.handle, (char*) SessionID, HANDLESIZE)) {
			G_GetCurrentTimeString(time, nFileSliceStartUTC, YMDHMS);
//			pthread_mutex_lock(&g_mdvrinfo_mutex);
			memcpy(node->zmqData.starttime, time.c_str(), TIMEFORMATSIZE);
			G_GetCurrentTimeString(time, nFileSliceStopUTC, YMDHMS);
			memcpy(node->zmqData.endtime, time.c_str(), TIMEFORMATSIZE);
			memcpy(node->zmqData.filename, szFileName,
					strnlen(szFileName, FILENAMELENGTH));

			node->zmqData.fileStartOffset = nStartOffset;
			node->zmqData.fileStopOffset = nStopOffset;
			node->zmqData.recordtype = nFlagRecordType;
			node->zmqData.statecode = nStateCode;
			//node->zmqData.fileOffset = 0;

//			pthread_mutex_unlock(&g_mdvrinfo_mutex);
			break;
		}
		node = node->next;
	}

	pthread_mutex_unlock(&g_mdvrinfo_mutex);
	return GLOBAL_FUNCTION_NO_ERROR;
}

/*
 * 返回值：int 返回0成功,否则返回错误码
 * par: 输出缓存区;1970-今的秒数;输出格式(enum);
 * 格式可以参考公共enum类型
 * 获取相应时间字符串
 * 1.2.1Ver later Add
 */
int G_GetCurrentTimeString(string &retbuf, time_t &time, int exact) {
	struct tm *stm;
	char timebuf[32] = { 0 };

	if (!(stm = localtime(&time))) {
		retbuf = "";
		return FUNCTION_ERROR;
	}

	switch (exact) {
	case YMDHMS:
		strftime(timebuf, 32, "%Y%m%d%H%M%S", stm);
		break;
	case YMDHM:
		strftime(timebuf, 32, "%Y%m%d%H%M", stm);
		break;
	case YMDH:
		strftime(timebuf, 32, "%Y%m%d%H", stm);
		break;
	case YMD:
		strftime(timebuf, 32, "%Y%m%d", stm);
		break;
	case YM:
		strftime(timebuf, 32, "%Y%m", stm);
		break;
	case Y:
		strftime(timebuf, 32, "%Y", stm);
		break;
	}
	/*
	 * string Automatically assign a value before the delete data
	 */
	retbuf = timebuf;
	return FUNCTION_SUCCESS;
}
