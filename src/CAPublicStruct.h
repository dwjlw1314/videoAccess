/*
 * CAPublicStruct.h
 *
 *  Created on: Jan 30, 2015
 *      Author: diwenjie
 */

#ifndef CAPUBLICSTRUCT_H_
#define CAPUBLICSTRUCT_H_

/*
 * c head include
 */
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/*
 * c++ head include
 */
#include <list>
#include <map>
#include <libconfig.h++>
#include "CA_SDK.h"
#include "VMS_MDVR_SDK.h"
#include "CAPerAnalysis.h"
#include "CAException.h"
#include "CALog.h"

//int cover string use
#include <boost/lexical_cast.hpp>
//CAThreadManager use,Comments not be used
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace boost;
using namespace libconfig;
using std::string;

#ifndef _WIN32
#define SOCKET_ERROR -1
#endif

#define FUNCTION_SUCCESS 0
#define FUNCTION_ERROR   -1

/*
 * Cancel the compiler hint unused parameters
 */
#define UNUSED(VAR) {VAR++;VAR--;}
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))

/* Automatically generated by configure - do not modify! */
#ifndef VIDEOACCESS_CONFIG_H
//define Switch
#define PROCESSVERSION "1.4.7.Release"
#define VIDEOACCESS_LICENSE "GPL version 2 or later"
#define CONFIG_THIS_YEAR 2014
#define CC_IDENT "gcc 4.4.7 (GCC) 20120313 (Red Hat 4.4.7-3)"
#define PROGRAMNAME "VAS_Access"
#define UPGRADEFILENAME "VAS_AccessUpgrade.tar.gz"
#define SHELLFILENAME "vainstall.sh"
#define SDK_CONF_BUFFER_SIZE 32
#define THROW_EXCEPTION 0
#define ENABLE_UPGRADE_CODE 0
#define ZLOG_LIST 0
#define ZLOG_SINGLETON 1
#define ZLOG_USE_FORMAT 0
#define ZLOG_ENABLE 1
#define FRAME_WRITE_FILE 0
#define USE_BOOST_TIMER 0
#define USE_G726_AAC_ECODEC 1

//define AVCode
#define USE_AVIO_CTX 0

//define real data size
#define PREFIXSIZE 12
#define HANDLESIZE 36
#define ALARMIDSIZE 40
#define TIMEFORMATSIZE 14
#define MDVRIDSIZE 10
#define SENDSIZE (188*7)
#define LASTFRAMESIZE 8
//define file download data size
#define FILENAMELENGTH 512

//test used
#define USE_FORK 0
#define USE_TEST_FUNCTION 0

#endif /* VIDEOACCESS_CONFIG_H */

#if USE_TEST_FUNCTION
	void G_TestFunction();
#endif

#if USE_BOOST_TIMER
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

typedef struct GlobalData {
	/*
	 * struct SdkConfInfo pointer
	 */
	void *pSci;

	int SDkCount;

	/*
	 * Whether to start the daemon false
	 */
	int EnableDaemon;

	/*
	 * socket Cache size operator
	 */
	int SocketCacheMultiple;

	/*
	 * The business process can create the number of threads
	 * Value through the configuration file
	 */
	int ThreadNums;

	/*
	 * redis server port
	 * redis and zermq used ip
	 */
	int RedisPort;
	string RedisIp;
	string LocalIp;

	/*
	 * AvCodeTrans output file format,eg:mp4,rmvb...
	 */
	string AvOutputFormat;

	/*
	 * Current zlog used,include each sdk info
	 */
	string ZlogConfPath;
	string ZlogCname;
	string ZlogRmCname;
	string ZlogFormat;

	/*
	 * process version
	 */
	string Version;

	//process run absolutely path
	string CurrentPath;
}g_Data,*g_pData;
extern g_Data g_data;

extern size_t g_msghdr_size;
extern size_t g_mdvrinfo_size;
extern size_t g_zmqdata_size;
extern std::map<const char*,int> g_downFileOffset;
typedef struct SdkConfInfo {
	/*
	 * sdk company name,read configure file,
	 * but buffer size < SDK_CONF_BUFFER_SIZE
	 */
	//uint8_t sdkname[SDK_CONF_BUFFER_SIZE];
	string sdkname;

	//sdkid current is uuid form;
	string sdkid;

	//uint8_t sdkip[SDK_CONF_BUFFER_SIZE];
	string sdkip;

	//Factory zlogname
	string ZlogCname;

	//whether open log
	int EnableSdkLog;

	//sdk port
	int sdkport;

	//zeromq port
	int zmqport;

	//filedown zmq port,now unused
	int downzmqport;

	//timeout
	int timeout;

	//sdk process pid
	pid_t pid;

	/*
	 * maybe hava more sdk
	 */
	struct SdkConfInfo *next;

}SdkConfInfo,*pSdkConfInfo;

//FRAME TYPE Customer
typedef enum FRAME_TYPE{
	FRAME_TYPE_I_FRAME = 0,
	FRAME_TYPE_P_FRAME,
	FRAME_TYPE_A_FRAME,
	FRAME_TYPE_EXTRA_FRAME,
	FRAME_TYPE_UNKNOWN = 101
}frame_type;

typedef enum TRANS_STATUS{
	FRAME_DATA_WAIT = 102,
	FRAME_DATA_DELETE,
	FRAME_DATA_START = 1001,
	FRAME_DATA_TRANS,
	FRAME_DATA_STOP,
} trans_status;

//FRAME TYPE Customer
typedef enum mdvr_avcodec_map{
	NO_MODIFY_MAP = 2001,
	CREATE_MDVR_AVCODEC_MAP,
	DELETE_MDVR_AVCODEC_MAP,
}CREATE_MDVR_AVCODEC_MAP_TYPE;

//Time exact
typedef enum exact{
	YMDHMS = 0, //%Y%m%d%H%M%S
	YMDHM,		//%Y%m%d%H%M
	YMDH,		//%Y%m%d%H
	YMD,		//%Y%m%d
	YM,			//%Y%m
	Y,			//%Y
}EXACT_TYPE;

/*
 * connect_type ->
 * 0:视频点播
 * 1:一键报警视频
 * 2:文件下载
 */
typedef enum connect_type{
	MDVR_VIDEO_REQUEST_CONNECT,
	MDVR_VIDEO_ALARM_CONNECT,
	MDVR_FILE_DOWNLOAD_CONNECT
}MDVR_CONNECT_TYPE;

/*
 * stream_id ->
 * 0:主码流
 * 1:子码流
 * 2:报警15秒码流
 */
typedef enum stream_id{
	MDVR_VIDEO_MAIN_STREAM,
	MDVR_VIDEO_CHILD_STREAM,
	MDVR_VIDEO_ALARM_STREAM
}MDVR_STREAM_ID;

/*
 * connect_type ->
 * 1:开始文件下载
 * 2:文件下载完成
 * 3:文件下载中断,任务未完成
 */
typedef enum file_download_state{
	FILE_DOWNLOAD_START = 1,
	FILE_DOWNLOAD_FINISHED,
	FILE_DOWNLOAD_INTERRUPT,
}FILE_DOWNLOAD_STATE;

/*
 * G_ModifyMdvrInfoNode Function par;
 * 主要是处理线程退出异常处理到参数标志
 */
typedef enum modify_mdvr_info{
	MDVRINFO_SOCKET_ADD = 3001,
	MDVRINFO_SOCKET_DEL,
}MODIFY_MDVR_INFO_TYPE;

/*
 * ZMQ use message structure
 * two variable ZmqData,pZmqData
 * 控制模块和存储模块共同使用的数据结构
 */
typedef struct ZMQData {
	//过滤消息头（转发模块,存储模块）
	char prefix[PREFIXSIZE+4];
	//设备ID
	char mdvrid[MDVRIDSIZE+4];
	//ocx和文件下载唯一编号
	char uuid[HANDLESIZE+4];
	//mdvr视频唯一编号,文件下载与uuid相同
	char handle[HANDLESIZE+4];
	//警情ID
	char alarmid[ALARMIDSIZE+4];
	//录像开始时间(年月日时分秒),文件下载开始时间
	char starttime[TIMEFORMATSIZE+4];
	//录像结束时间(年月日时分秒)，文件下载结束时间
	char endtime[TIMEFORMATSIZE+4];
	//下载文件名
	char filename[FILENAMELENGTH];
	//视频流数据
	char fData[SENDSIZE];
	//原始文件偏移信息(-1:无效)
	int fileStartOffset;
	//原始文件结束偏移信息(-1:无效)
	int fileStopOffset;
	//当前传输文件大小偏移,默认是0
	int fileOffset;
	//通道ID
	int channelid;
	//码流:0主码流/1子码流/2 报警15秒码流
	int streamid;
	//视频类型: 0视频点播 1一键报警视频 2文件下载
    int midtype;
	/*
	 * 文件下载状态
	 * 1 开始文件下载
	 * 2 文件下载完成
	 * 3 文件下载中断，任务未完成
	 */
	int statecode;
	/*
	 * 录像类型
	 * 0 普通录像 1 报警录像
	 */
	int recordtype;
    //视频流数据实际大小
	int fActualSize;
	//视频传输开始标记,视频传输结束标记,
	//用于单设备对应单个编解码类以及存储使用
	//1.2Ver after used
    int sendFlag;

    //0 完全下载 1 按大小偏移下载 2 按时间偏移下载
    int downloadType;

    //按大小偏移 时间偏移下载对应的起始偏移量和结束偏移量
    char startLoadOffset[TIMEFORMATSIZE+4];
    char endLoadOffset[TIMEFORMATSIZE+4];

}ZmqData,*pZmqData;

/*
 * Mdvr use message structure
 * two variable Mdvrinfo,pMdvrinfo
 * Mdvr File Download Information
 * avCreateFlag equal RECONNECTTIMES
 */
#define RECONNECTTIMES 5
typedef struct MdvrInfo {
	//ZMQ struct
	ZmqData zmqData;
	//对应线程组的写socketpair描述符
	int socketWfd;
	//frame type
	int frameType;
	//pts
	long long pts;
	//utc
	time_t frameUTC;
	//next node
	struct MdvrInfo *next;
}Mdvrinfo,*pMdvrinfo;

/*
 * AVCodeTrans use function pointer
 * Redis already cancel
 */
typedef struct getinfoFP{
	int (*avReadCb)(void*, uint8_t*, int);
	int (*avWriteCb)(void*, uint8_t*, int);
}CallbackFun,*pCallbackFun;

/*
 * get local ip
 */
string G_GetLocalIpAddr();

/*
 * get Current path
 */
void G_GetCurrentPath(string &path);

/*
 * init global data
 * par: conf file obj
 * ret: 0 is ok ,otherwish is err
 */
int G_InitGlobalDefine(Config* cfg, SdkConfInfo* pSdkConfNode);

int G_GetCurrentTimeString(string &retbuf, time_t &time, int exact);
int G_ShadowDaemon(char *name);
int G_EnableDaemon(void);

void G_SetSecurityLimit(int item, int curvalue, int maxvalue);
void G_FinitGlobalData(void);
void G_SetSystemSignal();


//GLOBAL TYPE Customer
typedef enum gerr{
	GLOBAL_FUNCTION_NO_ERROR = 0,
	ENABLE_DAEMON_ERROR,
	MDVR_LIST_IS_EMPTY = 100,
	MDVR_HANDLE_INVALID,
	SEND_MDVR_REAL_DATA_ERROR,
	SEND_MDVR_REAL_DATA_TIMESOUT,
	ADD_MDVR_LIST_NEWMEMORY_ERROR,
	DEL_MDVR_LIST_NEWMEMORY_ERROR,
	ADD_MDVR_LIST_FILEDOWN_ERROR,
	DEL_MDVR_LIST_FILEDOWN_ERROR,
}GloableError;

/*
 * add/del/modify mdvrinfo list collection
 */
int G_AddMdvrInfoList(void *pHandle, UUID_T nSessionID, char *szMDVRID,
		int nConnectType, int nChannelID, int nStreamID, char *szAlarmID,
		int socketfd);
int G_DelMdvrInfoList(void *pHandle, int connectType);
int G_ModifyMdvrInfoNode(void *pHandle,int socketwfd,int type);

/*
 * send VideoAudio RealData function;
 * By socketpair reducing copying of data
 */
int G_SendMdvrRealData(void* pHandle, int nFrameType, unsigned int nFrameLen,
		unsigned char* pFrameData, long long nPTS, unsigned char* nRTC, int nFileOffset,
		unsigned char* pExtraDataBuf, unsigned int nExtraDataLen);
/*
 * Send video at the end
 */
int G_SendLatestData(pMdvrinfo node);
/*
 * get MdvrInfoList nums
 */
int G_GetMdvrInfoListNums();

/**************************************
 * file download function part
 **************************************/

/*
 * modify FileDownInfo list collection
 */
int G_ModifyFileDownInfoList(char* SessionID,int nMDVRName,char* szFileName,
		int nStateCode,int nStartOffset,int nStopOffset,int nFlagRecordType,
		time_t nFileSliceStartUTC,time_t nFileSliceStopUTC);

#endif /* CAPUBLICSTRUCT_H_ */
