/*

	VMS_MDVR_SDK.h


	This File is the header file of SDK.


	Author:liuxin
	History:2012/09/10 Create.

*/
#ifndef __ANT_VMS_MDVR_SDK__
#define __ANT_VMS_MDVR_SDK__


#ifdef __cplusplus
extern "C"
{
#endif

//typedef __time_t time_t;
#include <time.h>
//#include <uuid/uuid.h>
#include <stdarg.h>
//typedef struct tm time_t;
typedef unsigned char UUID_T[16];

typedef enum {
	MDVR_MANUFACTURE_NONE = 0,
	MDVR_MANUFACTURE_RM, 				//streamax
	MDVR_MANUFACTURE_WKP, 				//wekomp
    MDVR_MANUFACTURE_SC                 //sichuang
} MDVR_MANUFACTRUE;

typedef enum {
	MDVR_DEVICE_NONE = 0,
	MDVR_RM_SD4C = 100, 				//streamax  SD4C
	MDVR_WKP_CW1003_W = 200, 			//wekomp	WP-CW1003-W
	MDVR_SC_X1 = 300, 					//sichuang , maybe modify
} MDVR_DEVICE_TYPE;

typedef enum {
	VMS_TCP = 0,
	VMS_UDP,
	VMS_RTP,
	VMS_MULITCAST
}VMS_PROCTYPE;

typedef struct tagVMS_VERSION_INFO
{
	unsigned int dwMainVer;
	unsigned int dwSubVer;
	unsigned int dwBuildDate;
	unsigned int nTest;
} VMS_VERSION_INFO;



////////////////////////////////////////////////////////////////////////////////////
//STATE CODE AND STATE DATA

typedef enum {
	VMS_SDK_THREAD_STATE = 0,   //response VMS_STATEDATA_SDK_THREAD
	VMS_SDK_DAS_STATE,			//response VMS_STATEDATA_SDK_DAS
	VMS_SDK_MDVR_STATE,			//response VMS_STATEDATA_SDK_MDVR
	VMS_SDK_REALPLAY_STATE,		//response VMS_STATEDATA_SDK_REALPLAY
	VMS_SDK_REALDATA_STATE,		//response VMS_STATEDATA_SDK_REALDATA
	VMS_SDK_FILETASK_STATE,		//response VMS_STATEDATA_SDK_FILETASK
	VMS_SDK_FILEDATA_STATE		//response VMS_STATEDATA_SDK_FILEDATA
}VMS_STATECODE;

typedef struct tagStateDataSDKThread
{
	enum  state{
		STARTING = 0,			//初始化过程中
		ACCEPT,					//开始服务
		CONNECTED,				//有MDVR进行通讯
		ACCEPTBLOCK,			//不再额外提供服务
		SHUTDOWN				//关闭服务
	} value;
}VMS_STATEDATA_SDK_THREAD;

typedef struct tagStateDataSDKDAS
{
	enum  state{
		STARTING = 0,			//初始化过程中
		BIND,					//绑定结束
		CONNECTED,				//与数据接入服务器建立连接
		BUSY,					//服务器繁忙没有响应
		SHUTDOWN				//关闭服务
	} value;
}VMS_STATEDATA_SDK_DAS;

typedef struct tagStateDataSDKMDVR
{
	enum  state{
		STARTING = 0,			//初始化过程中
		CONNECTED,				//与数据接入服务器建立连接
		BUSY,					//MDVR繁忙没有响应
		DISCONNECT,				//MDVR下线
		REALPLAY,				//正在实时流播放
		ALERT,					//一键报警过程中
		FILETRANSFER			//文件下载过程中
	} value;
}VMS_STATEDATA_SDK_MDVR;

typedef struct tagStateDataSDKRealPlay
{
	enum  state{
		STARTING = 0,			//开始实时流播放过程
		WAITTING,				//服务准备完毕，等待MDVR处理完毕
		REQDAS,					//向数据接入服务器请求实时流播放
		RSPDASSUCCESS,			//数据接入服务器返回成功
		RSPDASFAIL,				//数据接入服务器返回失败
		OPENCHANNEL,			//打开输入信道成功
		STREAMREADY,			//编码正常，输出流
		MDVRREADY,				//MDVR准备完毕
		MDVRFAIL				//MDVR准备失败
	} value;
}VMS_STATEDATA_SDK_REALPLAY;

typedef struct tagStateDataSDKRealData
{
	enum  state{
		NORMAL = 0,				//正常传输
		MAXLIMIT,				//传输到达带宽上限
		NODATA,					//传输数据不存在，只有心跳无数据
		SENDAGAIN,				//数据重发
		DROPFRAME,				//数据超时，SDK丢弃数据
		DISCONNECT,				//停止传输
		FRAMERATECHANGE,		//帧率变化
		STREAMRATECHANGE,		//码率变化
		REALPLAYNORMALSTOP   //实时流正常结束
	} value;
	union data{
		int   nFrameRate;		//帧率
		int   nStreamRate;		//码率
	}u;
	void * pStreamHandle;
}VMS_STATEDATA_SDK_REALDATA;

typedef struct tagStateDataSDKFileTask
{
	enum  state{
		STARTING = 0,			//开始文件下载过程
		WAITTING,				//服务准备完毕，等待MDVR处理完毕
		REQDAS,					//向数据接入服务器请求实时流播放
		RSPDASSUCCESS,			//数据接入服务器返回成功
		RSPDASFAIL,				//数据接入服务器返回失败
		OPENFILE				//打开文件成功
	} value;
}VMS_STATEDATA_SDK_FILETASK;

typedef struct tagStateDataSDKFileData
{
	enum  state{
		NORMAL = 0,				//正常传输
		MAXLIMIT,				//传输到达带宽上限
		NODATA,					//传输数据不存在，只有心跳无数据
		NOFILE,					//MDVR文件丢失
		DISCONNECT,				//停止传输
		FILECHANGE				//同个任务内部，切换传输文件
	} value;
	union data{
		char szFileName[256];//新切换的文件名称
	}u;
}VMS_STATEDATA_SDK_FILEDATA;


////////////////////////////////////////////////////////////////////////////////////
//FUNCTION CALLBACK DEFINE


typedef void (*FUNC_WRITELOGCALLBACK) (unsigned int nType,const char *szFmt,va_list argptr);

typedef int (*FUNC_SETSTATECALLBACK) (int nObjType, \
									  int nObjID, \
									  int nState, \
									  unsigned char * pStateBufInfo, \
									  unsigned int nBufLen);

typedef int (*FUNC_SETREALDATACALLBACK) (void * pHandle, \
										 int nFrameType, \
										 unsigned int nFrameLen, \
										 unsigned char* pFrameData, \
										 long long nPTS, \
										 time_t nUTC, \
										 unsigned char * pExtraDataBuf, \
										 unsigned int nExtraDataLen);

//nFrameType   0 I Frame 1 Other Video Frame 2 Audio Frame 3 Extra Data

//typedef unsigned char uuid_t[16];
typedef int (*FUNC_SETFILEDATACALLBACK) (UUID_T nTaskID, \
										 char * szCurFileName, \
										 int nFileOffset, \
										 int nFrameType, \
										 unsigned int nFrameLen, \
										 unsigned char* pFrameData, \
										 long long nPTS, \
										 time_t nUTC, \
										 unsigned char * pExtraDataBuf, \
										 unsigned int nExtraDataLen);

//int 		 nStateCode  1 开始文件下载 2 文件下载完成 3 文件下载中断，任务未完成
//int			 nFlagRecordType 录像类型 0 普通录像 1 报警录像
typedef int (*FUNC_SETFILETASKCALLBACK) (UUID_T nTaskID, \
										 char * szMDVRID, \
										 int nMDVRName, \
										 char * szFileName, \
										 time_t nFileSliceStartUTC, \
										 time_t nFileSliceStopUTC, \
										 int 	nStartOffset, \
										 int 	nStopOffset, \
										 int 	nStartCode, \
										 int	nFlagRecordType, \
										 unsigned int * nCurrentOrgFileOffset);

typedef int (*FUNC_SETDASCALLBACK) (char * szRequestURL, \
									char * szRequestPostData, \
									char * szRequestHeader, \
									unsigned char ** szResponseMsg, \
									unsigned int *   pnResponseLen);

//int   nConnectType 0 视频点播 1 一键报警视频 2 文件下载
//int   nStreamID  0 主码流 1 子码流 2 报警15秒码流  
//char * szAlarmID  警情ID 
//int *  bSuccess   1同意连接 0 拒绝连接（输出参数）
//int *  pAsyncCall  0 同步调用 1 异步调用
//int *  nReturnInfo  0  负载均衡需要重连动作 1 黑名单，拒绝连接
//char * szReconnectIP  重连IP       
//int  * nReconnectPort 重连Port
 
typedef int (*FUNC_SETMEDIAREGISTERCALLBACK) (void * pHandle, \
								  UUID_T nSessionID, \
								  char * szMDVRID, \
								  int nConnectType, \
								  int nChannelID, \
								  int nStreamID, \
								  char * szAlarmID, \
								  int * bSuccess, \
								  int * pAsyncCall, \
								  int * nReturnInfo, \
								  char * szReconnectIP, \
								  int * nReconnectPort, \
								  int  * nTimeOut);
								  
//int   nConnectType 0 视频点播 1 一键报警视频 2 文件下载
//int   nReasonFlag  0 业务异常结束  1 业务正常结束 2 网络异常
//int   nStreamID  0 主码流 1 子码流 2 报警15秒码流  

typedef int (*FUNC_SETMEDIADISCONNECTCALLBACK) (void * pHandle, \
									UUID_T nSessionID, \
									char * szMDVRID,  \
									int nConnectType,  \
									int nReasonFlag, \
									int nChannelID, \
									int nStreamID);
////////////////////////////////////////////////////////////////////////////////////
//ERROR CODE
#define _EC(x)					       (0x80000000|x)
#define VMS_NOERROR 				   0	  //没有错误
#define VMS_ERROR					  -1      //未知错误
//--------------------初始化错误-----------------------//
#define VMS_NEW_SOCKET_ERROR	  	  _EC(1)//建立SOCKET失败
#define VMS_CONNECT_ERROR	      	  _EC(2)//建立连接失败
#define VMS_BIND_ERROR	              _EC(3)//绑定端口失败
#define VMS_LISTEN_ERROR	      	  _EC(4)//监听失败
#define VMS_CONNECT_TIMEOUT     	  _EC(5)//网络连接超时
#define VMS_NO_MEMORY			   	  _EC(6)//没有内存
#define VMS_INVALID_HANDLE		   	  _EC(7)//文件不存在
#define VMS_INVALID_SDK 		   	  _EC(8)//SDK版本不匹配
#define VMS_PTHREAD_FAIL 		      _EC(9)//建立线程失败
#define VMS_STARTUP_FAIL 	   		  _EC(10)//其他原因失败
#define VMS_SDK_INTERFACE_NOT_FOUND   _EC(11)//函数指针不存在
#define VMS_NO_SERVICE_LISTEN		  _EC(12)//没有起服务

//--------------------实时流错误----------------------//
#define VMS_INVALID_STREAM_HANDLE	  _EC(101)//流句柄无效
#define VMS_INVALID_MDVRID            _EC(102)//设备ID无效
#define VMS_INPUT_CHANNEL_ERROR		  _EC(103)//设备输入信道号错误
#define VMS_OUTPUT_STREAM_ERROR		  _EC(104)//设备输出流编号错误
#define VMS_TRANSFER_TYPE_ERROR		  _EC(105)//设备传输类型不支持
#define VMS_OPEN_CHANNEL_ERROR		  _EC(106)//打开通道错误
#define VMS_VIDEO_ENCODER_ERROR 	  _EC(107)//视频编码错误
#define VMS_AUDIO_ENCODER_ERROR 	  _EC(108)//音频编码错误
#define VMS_DROP_DATA_ERROR      	  _EC(109)//主动丢包
#define VMS_FRAME_RATE_CHANGE	   	  _EC(110)//主动变动码率
#define VMS_TRANSFER_BREAKDOWN		  _EC(111)//报警打断应用观看
#define VMS_NO_HEARTBEAT			  _EC(112)//设备掉线
#define VMS_MDVR_BUSY				  _EC(113)//设备繁忙性能下降
#define VMS_ILLEGAL_PARAM			  _EC(114)//用户参数不合法
#define VMS_SERVER_DOWN				  _EC(115)//服务器掉线
#define VMS_SERVER_TIMEOUT			  _EC(116)//服务器僵死
#define VMS_SERVER_DATA_ERROR		  _EC(117)//服务器缓存状态和数据库不一致
#define VMS_REALPLAY_WORKING		  _EC(118)//已经存在实时流


//-------------------文件下载相关错误-------------------//

#define VMS_SD_NOT_FOUND			  _EC(201)//SD卡丢失
#define VMS_PTS_ERROR				  _EC(202)//同个下载任务PTS不连续，或一直不变
#define VMS_FILE_MISSING			  _EC(203)//传输中文件不完整
#define VMS_FORCE_BREAKDOWN			  _EC(204)//传输被实时流打断
#define VMS_FILE_TRANSE_MAX_EXCEED    _EC(205)//下载任务过多

////////////////////////////////////////////////////////////////////////////////////
//LOG TYPE
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
////////////////////////////////////////////////////////////////////////////////////
//SDK INTERFACE

/////////////////////////////////////////////////////
//1  功能：SDK初始化，
//	 参数：void
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//   特殊说明：
int VMS_MDVR_SDK_Startup(FUNC_WRITELOGCALLBACK pFunc);


/////////////////////////////////////////////////////
//2	功能：设置连接等待时间,如TCP的超时时间
//	参数：int nTime 单位：毫秒
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：
int VMS_MDVR_SDK_SetConnectTime (int nTime);

/////////////////////////////////////////////////////
//3	功能：获取SDK版本
//	参数：VMS_VERSION_INFO * pInfo
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：
int VMS_MDVR_SDK_GetSDKVersion (VMS_VERSION_INFO * pInfo);

/////////////////////////////////////////////////////
//4	功能：设置DAS服务访问回调
//	参数：FUNC_SETDASCALLBACK pFunc
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：
int VMS_MDVR_SDK_SetDASCallBack(FUNC_SETDASCALLBACK pFunc);

/////////////////////////////////////////////////////
//5 功能：设置状态回调
//	参数：FUNC_SETSTATECALLBACK pFunc
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：返回SDK和MDVR的状态，如果异常会被回调。包括连接断开、设备状态等。
int VMS_MDVR_SDK_SetStateCallBack(FUNC_SETSTATECALLBACK pFunc);


/////////////////////////////////////////////////////
//6	功能：设置实时流回调
//	参数：FUNC_SETREALDATACALLBACK pFunc
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：
int VMS_MDVR_SDK_SetRealDataCallBack (FUNC_SETREALDATACALLBACK pFunc);

/////////////////////////////////////////////////////
//7 功能：设置文件流回调
//	参数：FUNC_SETFILEDATACALLBACK pFunc
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：服务器同时上传文件任务过多时，可以通过返回错误码，拒绝下载文件任务。
int VMS_MDVR_SDK_SetFileDataCallBack (FUNC_SETFILEDATACALLBACK pFunc);

/////////////////////////////////////////////////////
//8	功能：设置文件上传任务回调
//	参数：FUNC_SETFILETASKCALLBACK pFunc
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：
int VMS_MDVR_SDK_SetFileTaskCallBack (FUNC_SETFILETASKCALLBACK pFunc);

/////////////////////////////////////////////////////
//9 功能：设置媒体通道注册回调
//  参数：FUNC_SETMEDIAREGISTERCALLBACK pFunc 
//  返回值：int 返回0表示函数执行成功，否则返回错误码。
//  特殊说明：
int VMS_MDVR_SDK_SetMediaRegisterCallBack (FUNC_SETMEDIAREGISTERCALLBACK pFunc);

/////////////////////////////////////////////////////
//10 功能：设置媒体通道断开回调
//	 参数：FUNC_SETMEDIADISCONNECTCALLBACK pFunc 
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_SetMediaDisconnectCallBack (FUNC_SETMEDIADISCONNECTCALLBACK pFunc);

/////////////////////////////////////////////////////
//11 功能：获取接入服务器参数
//   参数：void * pHandle句柄信息 （实时流句柄或文件下载uuid） 
// 		     int  nSuccess   1同意连接 0 拒绝连接（输出参数）
// 				 int  nReturnInfo  0  负载均衡需要重连动作 1 黑名单，拒绝连接
// 				 char * szReconnectIP  重连IP       
// 				 int   nReconnectPort 重连Port
// 				 int   nTimeOut
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_MediaRegisted(void * pHandle,int  nSuccess, int nReturnInfo,char * szReconnectIP,int nReconnectPort,int nTimeOut);

/////////////////////////////////////////////////////
//12功能：设置数据接入服务器参数
//	参数：char * szIP  数据接入服务器IP， F5的IP
//		int 	nPort 端口信息
//		VMS_PROCTYPE nProcType 网络类型 （见数据结构定义）
//	返回值：int 返回0表示函数执行成功，否则返回错误码。
//	特殊说明：
int VMS_MDVR_SDK_SetDAS(char * szIP, int nPort, VMS_PROCTYPE nProcType);

/////////////////////////////////////////////////////
//13 功能：获取接入服务器参数
//	 参数：char * szIP 获取数据接入服务器IP，int * pnPort 获取端口号
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明
int VMS_MDVR_SDK_GetDAS(char * szIP, int * pnPort);

/////////////////////////////////////////////////////
//14 功能：获取SDK工作状态
//	 参数：VMS_STATECODE nState 			SDK状态码
//		  unsigned char ** szBuffer 	状态具体描述信息
//		  unsigned int *   pnBufferLen
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_GetState(VMS_STATECODE  nStateCode, unsigned char ** szBuffer, unsigned int *pnBufferLen);

/////////////////////////////////////////////////////
//15 功能：打开监视通道
//	 参数：char *  szMDVRID MDVR设备ID
//		  int     nChannelID 视频信道ID
//		  int 	  nStreamID  视频流ID
//		  int 	  nDelayTime 缓冲时间大小
//		  int	  bEnableAudio 是否发送声音
//		  VMS_PROCTYPE  nTransferType 传输类型 TCP /UDP/RTP
//		  void ** pStreamHandle /*out*/返回流句柄，
//		  short * psPort /*out*/
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明

int VMS_MDVR_SDK_RealPlay(char * szMDVRID,			//10位数字  前两位00锐明  01威康普
						  int 	 nChannelID,	    //0 通道一   1 通道二
						  int 	 nStreamID,         //0 主码流d1  1 子码流 cif  2 前10秒及后5秒码流
						  int 	 nAlarmType,		//0 普通实时流点播 1 报警实时流点播
						  int 	 nDelayTime,		//缓冲时间大小
						  int  	 bEnableAudio,		//是否发送声音
						  VMS_PROCTYPE nTransferType,
						  void ** pStreamHandle,
						  short * psPort);

/////////////////////////////////////////////////////
//13 功能：使能前端实时流是否有声音
//	 参数：void * pStreamHandle流句柄,int bEnable 0不需要声音 1需要声音
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_EnableRealPlayAudio(void * pStreamHandle, int  bEnable);

/////////////////////////////////////////////////////
//14 功能：停止监视通道
//	 参数：void * pStreamHandle 流句柄
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明
int VMS_MDVR_SDK_StopRealPlay(void * pStreamHandle);

/////////////////////////////////////////////////////
//15 功能：强制I帧
//	 参数：void * pStreamHandle流句柄
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_MakeKeyFrame(void * pStreamHandle);

/////////////////////////////////////////////////////
//16 功能：获取实时播放状态
//	 参数：void *  		   pStreamHandle  文件流
//		  VMS_STATECODE  * pnState 		  状态码
//		  unsigned char ** szBuffer		  状态描述指针
//		  unsigned int  *  pnBufferLen	  状态描述缓冲区长度
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_GetRealPlayState (void * 		    pStreamHandle,
								   VMS_STATECODE *  pnState,
								   unsigned char ** szBuffer,
								   unsigned int  *  pnBufferLen);

/////////////////////////////////////////////////////
//17 功能：获取流媒体描述信息
//	 参数：char *			 szMDVRID     MDVR设备ID
//		  int   		 nChannelID   视频信道ID
//		  int 	 		 nStreamID    视频流ID
//		  unsigned char**szBuffer     描述信息缓冲
//		  unsigned int * pnBufferLen  描述信息缓冲大小
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_GetStreamDesc (char *  		szMDVRID,
								int 			nChannelID,
								int 			nStreamID,
								unsigned char** szBuffer,
								unsigned int * 	pnBufferLen);

/////////////////////////////////////////////////////
//18 功能：SDK释放
//	 参数：void
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_Cleanup();

/////////////////////////////////////////////////////
//19 功能：启动监听
//	 参数：int nPort 图像服务器端口
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_Listen(char * szIP,int nPort);

/////////////////////////////////////////////////////
//20 功能：停止监听，释放端口
//	 参数：void
//	 返回值：int 返回0表示函数执行成功，否则返回错误码。
//	 特殊说明：
int VMS_MDVR_SDK_StopListen();



#ifdef __cplusplus
}
#endif

#endif //__ANT_VMS_MDVR_SDK__
