/*
 * CARmSdkManager.h
 *
 *  Created on: Feb 3, 2015
 *      Author: diwenjie
 */

#ifndef CARMSDKMANAGER_H_
#define CARMSDKMANAGER_H_

#include "CAPublicStruct.h"
#include "CALog.h"
#include "CAThreadManager.h"
#include "CAException.h"
#include "CARedisClient.h"

enum sdkStat {
	FILE_RECORD_DOWNLOAD = 0,
	VIDEO_REALTIME_PREVIEW
};

/////////////////////////////////////////////////////
class CARmSdkManager : public noncopyable
{
public:
	static CARmSdkManager* GetInstanceContext();
	static CARmSdkManager *g_pRmSdkManager;
	static CAThreadManager *m_pThreadManager;

public:
	/*
	 * callback used;
	 */
	static void WriteLogCallback(unsigned int nType,const char *szFmt,
			va_list argptr);
	static int SetStateCallback(int nObjType,int nObjID,int nState,
			unsigned char *pStateBufInfo,unsigned int nBufLen);
	static int SetRealDataCallback(void *pHandle, int nFrameType,
			unsigned  int nFrameLen, unsigned char*pFrameData,
			long long nPTS, unsigned char* nRTC, unsigned char *pExtraDataBuf,
			unsigned int nExtraDataLen);
	static int SetFileDataCallback (unsigned char* nSessionID,	char *szCurFileName,
			int nFileOffset,int nFrameType, unsigned int nFrameLen,
			unsigned char *pFrameData,long long nPTS, unsigned char* nRTC,
			unsigned char *pExtraDataBuf,unsigned int nExtraDataLen);
	static int SetMediaRegisterCallback(void *pHandle,unsigned char* nSessionID,
			char *szMDVRID,int nConnectType,int nChannelID,int nStreamID,
			char *szAlarmID,int *bSuccess,int *pAsyncCall,int *nReturnInfo,
			char *szReconnectIP,int *nReconnectPort,int *nTimeOut);
	static int SetMediaDisconnectCallback(void * pHandle,UUID_T nSessionID,
			char *szMDVRID,int nConnectType,int nReasonFlag,int nChannelID,
			int nStreamID);
	static int SetFileTaskCallback (unsigned char* nSessionID,char *szMDVRID,
			int nMDVRName,char *szFileName,int nStartOffset,int nStopOffset,int nStateCode,int nFlagRecordType,
			unsigned int *nCurrentOrgFileOffset);

public:
	int StartRmSdkInit(SdkConfInfo *sdkinfo);

private:
	CARmSdkManager();
	virtual ~CARmSdkManager();
	int InvokSdkFunction(SdkConfInfo *sdkinfo);

private:
	int isExit;
	string m_zlogRmCname;
	//mdvr黑名单数据结构
	std::list<string> m_MdvrBlackList;
	CALog *m_pLog;
	static CARedisClient *m_pRedisclient;
	//设备一次只允许下载一个文件
	static int m_FileOffset;
	//上一次下载的文件名
	static char* m_pFilename;

public:
	/*
	 * port转换成string类型方便频繁调用
	 */
	string m_Zeromqport;

#if FRAME_WRITE_FILE
    CAFileManager m_VideoFile;
#endif
};

#endif /* CARMSDKMANAGER_H_ */
