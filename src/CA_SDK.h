#ifndef CARMSDK_H_
#define CARMSDK_H_

#include <time.h>

#define MTU 1460
#define HEADSIZE 12
#define MAXLINE 10240

typedef struct {
	char mdvrid[16];
	char handle[36];
	char filename[12];
}CA_CALLBACKINFO;


typedef void (*CA_FUNC_WRITELOGCALLBACK) (unsigned char* buf, int len);

/*-------------------------------------------------------
 * int   nConnectType 0 视频点播 1 一键报警视频 2 文件下载
 * int   nStreamID  0 主码流 1 子码流 2 报警15秒码流
 * char* szAlarmID  警情ID
 * int*  bSuccess   1同意连接 0 拒绝连接（输出参数）
 * int*  nTimeOut
 -----------------------------------------------------------*/
typedef int (*CA_FUNC_SETMEDIAREGISTERCALLBACK) (void* pHandle, unsigned char* nSessionID, char* szMDVRID,\
                int nConnectType, int nChannelID, int nStreamID, char* szAlarmID, int* bSuccess, int* pAsyncCall,\
                int* nReturnInfo, char * szReconnectIP, int* nReconnectPort, int  * nTimeOut);
//
typedef int (*CA_FUNC_SETREALDATACALLBACK) (void* pHandle, int nFrameType, unsigned int nFrameLen, \
                unsigned char* pFrameData, long long nPTS, unsigned char* nRTC, unsigned char * pExtraDataBuf, \
                unsigned int nExtraDataLen);

//nStateCode  1 开始文件下载 2 文件下载完成 3 文件下载中断，任务未完成
//nFlagRecordType 录像类型 0 普通录像 1 报警录像
typedef int (*CA_FUNC_SETFILETASKCALLBACK) (unsigned char* nTaskID, char * szMDVRID, int nMDVRName, char * szFileName, \
                int nStartOffset, int nStopOffset, int nStartCode, int nFlagRecordType, unsigned int * nCurrentOrgFileOffset);

typedef int (*CA_FUNC_SETFILEDATACALLBACK) (unsigned char*  nTaskID, char * szCurFileName, int nFileOffset, int nFrameType, \
                                         unsigned int nFrameLen, unsigned char* pFrameData, long long nPTS, unsigned char* nRTC, \
                                         unsigned char * pExtraDataBuf, unsigned int nExtraDataLen);

typedef int (*CA_FUNC_SETDASCALLBACK) (char * szRequestURL, char * szRequestPostData, char * szRequestHeader, \
                                    unsigned char ** szResponseMsg, unsigned int *   pnResponseLen);

typedef struct 
{
	CA_FUNC_WRITELOGCALLBACK logFunc;
	CA_FUNC_SETMEDIAREGISTERCALLBACK mediaRegisterFunc;
	CA_FUNC_SETREALDATACALLBACK  realDataFunc;
	CA_FUNC_SETFILETASKCALLBACK fileTaskFunc;
	CA_FUNC_SETFILEDATACALLBACK fileDataFunc;
}EventHandle;

/////////////////////////////////////////////////////
////1  功能：SDK初始化，
////   参数：监听端口
////   返回值：int 返回0表示函数执行成功，否则返回错误码。
////   特殊说明：
int CA_RM_SDK_Startup(int listenPort);

/////////////////////////////////////////////////////
////1  功能：设置日志
////   参数：void
////   返回值：int 返回0表示函数执行成功，否则返回错误码。
////   特殊说明：
int CA_RM_SDK_SetLogCallBack(CA_FUNC_WRITELOGCALLBACK pFunc);

///
int CA_RM_SDK_SetMediaRegisterCallBack (CA_FUNC_SETMEDIAREGISTERCALLBACK pFunc);

/////////////////////////////////////////////////////
////6   功能：设置实时流回调
////    参数：FUNC_SETREALDATACALLBACK pFunc
////    返回值：int 返回0表示函数执行成功，否则返回错误码。
////    特殊说明：
int CA_RM_SDK_SetRealDataCallBack (CA_FUNC_SETREALDATACALLBACK pFunc);

/////////////////////////////////////////////////////
//7 功能：设置文件流回调
//  参数：FUNC_SETFILEDATACALLBACK pFunc
//  返回值：int 返回0表示函数执行成功，否则返回错误码。
//  特殊说明：服务器同时上传文件任务过多时，可以通过返回错误码，拒绝下载文件任务。
int CA_RM_SDK_SetFileDataCallBack (CA_FUNC_SETFILEDATACALLBACK pFunc);

/////////////////////////////////////////////////////
//8 功能：设置文件上传任务回调
//  参数：FUNC_SETFILETASKCALLBACK pFunc
//  返回值：int 返回0表示函数执行成功，否则返回错误码。
//  特殊说明：
int CA_RM_SDK_SetFileTaskCallBack (CA_FUNC_SETFILETASKCALLBACK pFunc);

#endif
