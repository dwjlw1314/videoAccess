/*
 * CARmSdkManager.cpp
 *
 *  Created on: Feb 3, 2015
 *      Author: diwenjie
 */

#include <iostream>
#include "CARmSdkManager.h"

CARmSdkManager* CARmSdkManager::g_pRmSdkManager = NULL;
CAThreadManager* CARmSdkManager::m_pThreadManager = NULL;
CARedisClient* CARmSdkManager::m_pRedisclient = NULL;
int CARmSdkManager::m_FileOffset = 0;
char* CARmSdkManager::m_pFilename = NULL;

CARmSdkManager* CARmSdkManager::GetInstanceContext() {
	if (!g_pRmSdkManager) {
		g_pRmSdkManager = new CARmSdkManager;
		m_pRedisclient = new CARedisClient(g_data.RedisIp.c_str(),
				g_data.RedisPort);
		if (!CARedisClient::m_InitStatus) {
			return NULL;
		}
	}
	return g_pRmSdkManager;
}

int CARmSdkManager::StartRmSdkInit(SdkConfInfo *sdkinfo) {
	while (sdkinfo && sdkinfo->sdkname.compare("RM"))
		sdkinfo = sdkinfo->next;
	//如果sdk信息没有找到，暂时采用默认port:8080
	if (!sdkinfo) {
		m_pLog->Info("The SDK info don't found!!");
	}

	m_Zeromqport = boost::lexical_cast<string>(sdkinfo->zmqport);
	m_pLog->Info("RmSdk Starting ......");
	if (InvokSdkFunction(sdkinfo)) {
		return FUNCTION_ERROR;
	}
	m_pThreadManager->CreateThreadGroup();

	 cout<<"start"<<endl;

	 CA_RM_SDK_SetMediaRegisterCallBack(CARmSdkManager::SetMediaRegisterCallback);
	 CA_RM_SDK_SetRealDataCallBack (CARmSdkManager::SetRealDataCallback);
	 CA_RM_SDK_SetFileTaskCallBack(CARmSdkManager::SetFileTaskCallback);
	 CA_RM_SDK_SetFileDataCallBack(CARmSdkManager::SetFileDataCallback);
	 CA_RM_SDK_Startup(12002);

	 cout<<"end"<<endl;

#if 1 //采用线程等待阻塞主线程
	m_pThreadManager->ThreadJoinAll();
#else
	while(!isExit)
	{
		sleep(100);
	}
#endif
	return FUNCTION_ERROR;
}

void CARmSdkManager::WriteLogCallback(unsigned int nType, const char *szFmt,
		va_list argptr) {
	char msg[2048] = "0";
	string msgtype = "";

	if (!((SdkConfInfo*) (g_data.pSci))->EnableSdkLog)
		return;

	vsprintf(msg, szFmt, argptr);
	CARmSdkManager *gsdk = CARmSdkManager::GetInstanceContext();
	if (!gsdk) {
		std::cout << "CARmSdkManager::GetInstanceContext();" << __FILE__
				<< __LINE__ << std::endl;
		exit(0);
	}
	switch (nType) {
	case LOG_EMERG:
		msgtype = "EMERG";
		gsdk->m_pLog->Info("RmSDK:%s", msg);
		break;
	case LOG_ALERT:
		msgtype = "ALERT";
		gsdk->m_pLog->Warn("RmSDK:%s", msg);
		break;
	case LOG_CRIT:
		msgtype = "CRIT";
		gsdk->m_pLog->Info("RmSDK:%s", msg);
		break;
	case LOG_ERR:
		msgtype = "ERR";
		gsdk->m_pLog->Error("RmSDK:%s", msg);
		break;
	case LOG_WARNING:
		msgtype = "WARNING";
		gsdk->m_pLog->Warn("RmSDK:%s", msg);
		break;
	case LOG_NOTICE:
		msgtype = "NOTICE";
		gsdk->m_pLog->Notice("RmSDK:%s", msg);
		break;
	case LOG_INFO:
		msgtype = "INFO";
		gsdk->m_pLog->Info("RmSDK:%s", msg);
		break;
	case LOG_DEBUG:
	default:
		msgtype = "DEBUG";
		gsdk->m_pLog->Debug("RmSDK:%s", msg);
	}
}

int CARmSdkManager::SetStateCallback(int nObjType, int nObjID, int nState,
		unsigned char *pStateBufInfo, unsigned int nBufLen) {
	CARmSdkManager *gsdk = CARmSdkManager::GetInstanceContext();
	VMS_STATEDATA_SDK_REALDATA *pinfo =
			(VMS_STATEDATA_SDK_REALDATA*) pStateBufInfo;

	if (!pinfo || !gsdk) {
		std::cout << "CARmSdkManager::SetStateCallback() fail." << std::endl;
		return FUNCTION_ERROR;
	}

	//
	//just for debug
	//begin
	//
	string source = "SetStateCallback invoking: \n\t";

	// nObjType 状态对象类型 0 MDVR，1 SDK
	// nObjID   状态对象编号 MDVRID或SDK模块编号
	// nState    状态码
	switch (nObjType) {
	case 1:
		source += "\n\tInfo from RM SDK, Mode No. "
				+ boost::lexical_cast<string>(nObjID);
		break;
	case 0:
	default:
		source += "\n\tInfo from RM MDVR, Mode No. "
				+ boost::lexical_cast<string>(nObjID);
		break;
	}
//	typedef enum {
//			VMS_SDK_THREAD_STATE = 0, //SDK 内部线程函数工作状态
//			VMS_SDK_DAS_STATE,			//DAS数据接入服务器在线状态
//			VMS_SDK_MDVR_STATE,			//MDVR在线状态及工作模式状态
//		VMS_SDK_REALPLAY_STATE,		//实时流工作状态码
//		VMS_SDK_REALDATA_STATE,		//实时流数据工作状态码
//		VMS_SDK_FILETASK_STATE,		//文件下载任务工作状态码
//		VMS_SDK_FILEDATA_STATE		//文件下载传输工作状态码
//		}VMS_STATECODE;
//	int nState    状态码
	source += "\n\tStateCode=" + boost::lexical_cast<string>(nState);
	//source += string("\n\thandle: ") +(char*) pinfo->pStreamHandle;
	source += "\n\t extra data length: " + boost::lexical_cast<string>(nBufLen);
	gsdk->m_pLog->Debug(source.c_str() );
//	switch(pinfo->value){
//	case VMS_STATEDATA_SDK_REALDATA::NORMAL:		//正常传输
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state:NORMAL, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::MAXLIMIT:		//传输到达带宽上限
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state:MAXLIMIT, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::NODATA:		//传输数据不存在，只有心跳无数据
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state: NODATA, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::SENDAGAIN:		//数据重发
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state: SENDAGAIN, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::DROPFRAME:		//数据超时，SDK丢弃数据
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state: DROPFRAME, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::DISCONNECT:	//停止传输
//		gsdk->m_pLog->Info("SetStateCallback invoking: \n\t state: DISCONNECT, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::FRAMERATECHANGE:		//帧率变化
//		source += "\n\tFrame Rate or Code rate: "+ pinfo->u.nFrameRate;
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state: FRAMERATECHANGE, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::STREAMRATECHANGE:		//码率变化
//		source += "\n\tFrame Rate or Code rate: "+ pinfo->u.nStreamRate;
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state: STREAMRATECHANGE, %s",source.c_str() );
//		break;
//	case VMS_STATEDATA_SDK_REALDATA::REALPLAYNORMALSTOP:   //实时流正常结束:
//		gsdk->m_pLog->Debug("SetStateCallback invoking: \n\t state:REALPLAYNORMALSTOP, %s",source.c_str() );
//		break;
//	default:
//		break;
//	}
	//
	//just for debug
	//end
	//
	return FUNCTION_SUCCESS;
}
#if 0
std::map<string, long> recvpktnum_map;
#endif

int CARmSdkManager::SetRealDataCallback(void *pHandle, int nFrameType,
		unsigned int nFrameLen, unsigned char *pFrameData, long long nPTS,
		unsigned char* nRTC, unsigned char *pExtraDataBuf, unsigned int nExtraDataLen)
{
//	std::cout << "CARmSdkManager::SetRealDataCallback() sucess!!!." << std::endl;

	int ErrorCode = 0;
	CARmSdkManager *gsdk = CARmSdkManager::GetInstanceContext();
	if (!gsdk) {
		gsdk->m_pLog->Error("CARmSdkManager::SetRealDataCallback() fail.");
		return FUNCTION_ERROR;
	}

#if FRAME_WRITE_FILE
	//h264 or g726 file
	gsdk->m_VideoFile.fwrite((char*)pFrameData,nFrameLen,1);
#endif

	if (FRAME_TYPE_UNKNOWN == nFrameType) {
		gsdk->m_pLog->Info("SetRealDataCallback Unknow FrameType");
		return FUNCTION_SUCCESS;
	}

	//
	//just for debug
	//begin
	//
#if 0
	string info = string("SetRealDataCallback invoking:\n\thandle:")
			+ (char*) pHandle;
	info += "\n\tFrame type:" + boost::lexical_cast<string>(nFrameType);
	info += "\n\tFrame length:" + boost::lexical_cast<string>(nFrameLen);
	char temp[20];
	sprintf(temp, "%lld", nPTS);
	info += "\n\tpts:" + string(temp);
	string time = "";
	G_GetCurrentTimeString(time, nUTC, YMDHMS);
	info += "\n\tnUTC:" + time;
	info += "\n\tExtra data length:"
			+ boost::lexical_cast<string>(nExtraDataLen);
	gsdk->m_pLog->Debug(info.c_str());
#endif
	//
	//just for debug
	//end
	//

#if 0
   //存储每一个通道上传的原始数据
	if (nFrameType != 2) {
		string msg = (char*) pHandle;
		std::map<string, long>::iterator ite = recvpktnum_map.find(msg);
		if (ite == recvpktnum_map.end()) {
			recvpktnum_map.insert(std::pair<std::string, long>(msg, 1));
		} else {
			ite->second++;
			string str = ".h264";
			FILE* fp = fopen(((ite->first) + str).c_str(), "a+");
			fwrite(pFrameData, nFrameLen, 1, fp);
			fclose(fp);
			gsdk->m_pLog->Debug("handle:%s,recv num:%ld", msg.c_str(),
					ite->second);
		}
	}
#endif

//	gsdk->m_pLog->Info("G_SendMdvrRealData %s-----%d", pHandle, nFrameType);
	if ((ErrorCode = G_SendMdvrRealData(pHandle, nFrameType, nFrameLen,
			pFrameData, nPTS, 0, 0, pExtraDataBuf, nExtraDataLen)))
	{
		gsdk->m_pLog->Error("G_SendMdvrRealData SetRealData false;Handle[%s];ErrCode[%d];",(char*) pHandle, ErrorCode);

		if (SEND_MDVR_REAL_DATA_ERROR == ErrorCode) {
			int socketfd = -1;
			socketfd = m_pThreadManager->RetrunSocketPairfd();
			if (-1 == socketfd) {
				gsdk->m_pLog->Error(
						"G_SendMdvrRealData RetrunSocketPairfd false!!");
				return FUNCTION_SUCCESS;
			}
			if (!G_ModifyMdvrInfoNode(pHandle, socketfd, MDVRINFO_SOCKET_ADD)) {
				gsdk->m_pLog->Error("G_SendMdvrRealData G_ModifyMdvrInfoNode:"
						"pHandle[%s],socketfd[%d] false!!", pHandle, socketfd);
				if (-1 == m_pThreadManager->DecreasSocketPairfd(socketfd))
					gsdk->m_pLog->Error(
							"G_SendMdvrRealData DecreasSocketPairfd false!!");
			}
		}
	}
//	gsdk->m_pLog->Info("G_SendMdvrRealData after");

	return FUNCTION_SUCCESS;
}

int CARmSdkManager::SetMediaRegisterCallback(void *pHandle, UUID_T nSessionID,
		char *szMDVRID, int nConnectType, int nChannelID, int nStreamID,
		char *szAlarmID, int *bSuccess, int *pAsyncCall, int *nReturnInfo,
		char *szReconnectIP, int *nReconnectPort, int *nTimeOut)
{
	int ret = 0;
	int socketfd = -1;
	string key;
	CARmSdkManager *gsdk = CARmSdkManager::GetInstanceContext();
	if (!gsdk) {
		std::cout << "CARmSdkManager::SetMediaRegisterCallback() fail."
				<< std::endl;
		return FUNCTION_ERROR;
	}
	gsdk->m_pLog->Info(
			"SetMediaRegisterCallback:\n\tHandle[%s];\n\tSessionID[%s];\n\t"
					"MDVRID[%s];\n\tConnectType[%d];\n\tChannelID[%d];\n\tStreamID[%d];\n\t"
					"AlarmID[%s];\n\tsuccess[%d];\n\tAsyncCall[%d];\n\tReturnInfo[%d];\n\tReconnectIP[%d];"
					"\n\tReconnectPort[%d];\n\tTimeOut[%d];", (char*) pHandle,
			nSessionID, szMDVRID, nConnectType, nChannelID, nStreamID,
			szAlarmID, *bSuccess, *nTimeOut);

	if (MDVR_VIDEO_ALARM_CONNECT == nConnectType
			&& MDVR_VIDEO_CHILD_STREAM == nStreamID) {
		*bSuccess = 0;
		gsdk->m_pLog->Info(
				"Alarm video request has been cancel,info:\n\tHandle[%s];\n\t"
						"SessionID[%s];\n\tMDVRID[%s];\n\tConnectType[%d];\n\tChannelID[%d];\n\t"
						"StreamID[%d];\n\tAlarmID[%s];", (char*) pHandle,
				nSessionID, szMDVRID, nConnectType, nChannelID, nStreamID,
				szAlarmID);
		return FUNCTION_SUCCESS;
	}

	gsdk->m_pLog->Info("!!!!RetrunSocketPairfd!!!!");
	socketfd = m_pThreadManager->RetrunSocketPairfd();
	if (-1 == socketfd) {
		gsdk->m_pLog->Error(
				"SetMediaRegisterCallback RetrunSocketPairfd error!!");
		*bSuccess = 0;
		return FUNCTION_ERROR;
	}
	gsdk->m_pLog->Info("!!!!G_AddMdvrInfoList!!!!");
	if ((ret = G_AddMdvrInfoList(pHandle, nSessionID, szMDVRID, nConnectType,
			nChannelID, nStreamID, szAlarmID, socketfd))) {
		gsdk->m_pLog->Error("G_AddMdvrInfoList error!!status=%d", ret);
		m_pThreadManager->DecreasSocketPairfd(socketfd);
		*bSuccess = 0;
		return FUNCTION_ERROR;
	}

	gsdk->m_pLog->Info("Registed media Channel total: %d",G_GetMdvrInfoListNums());

	*nTimeOut = 1000;
	*bSuccess = 0;
	return FUNCTION_SUCCESS;
}

int CARmSdkManager::SetMediaDisconnectCallback(void *pHandle, UUID_T nSessionID,
		char *szMDVRID, int nConnectType, int nReasonFlag, int nChannelID,
		int nStreamID) {
	int socket = 0;
	CARmSdkManager *gsdk = CARmSdkManager::GetInstanceContext();
	if (!gsdk) {
		std::cout << "CARmSdkManager::SetMediaDisconnectCallback() fail."
				<< std::endl;
		return FUNCTION_ERROR;
	}

	gsdk->m_pLog->Info(
			"SetMediaDisconnectCallback invoked;\n\tHandle[%s];\n\tSessionID[%s];\n\t"
					"MDVRID[%s];\n\tConnectType[%d];\n\tReasonFlag[%d];\n\tChannelID[%d];\n"
					"\n\tStreamID[%d];\n", (char*) pHandle, nSessionID,
			szMDVRID, nConnectType, nReasonFlag, nChannelID, nStreamID);

	if (nConnectType == 2) {
		string key = KEYPRE;
//		key += (char*) pHandle;
		key += ":";
		key += m_pFilename;
		string value = boost::lexical_cast<string>(m_FileOffset);
		if (m_pRedisclient->HashAdd(key.c_str(), FIELD, value.c_str())) {
			gsdk->m_pLog->Error("HashAdd filename error");
			return FUNCTION_ERROR;
		}
	}
	//删除本地设备列表节点
	if (-1 == (socket = G_DelMdvrInfoList(pHandle, nConnectType))) {
		gsdk->m_pLog->Info("G_DelMdvrInfoList mdvrInfoList empty!!pHandle=%s",
				(char*) pHandle);
		return FUNCTION_ERROR;
	}

	//递减socket引用
	m_pThreadManager->DecreasSocketPairfd(socket);

	gsdk->m_pLog->Info("After SetMediaDisconnectCallback,mdvrListAllNums=%d",G_GetMdvrInfoListNums());

	return FUNCTION_SUCCESS;
}

int CARmSdkManager::SetFileDataCallback(unsigned char * nSessionID, char *szCurFileName,
		int nFileOffset, int nFrameType, unsigned int nFrameLen,
		unsigned char *pFrameData, long long nPTS, unsigned char * nRTC,
		unsigned char *pExtraDataBuf, unsigned int nExtraDataLen)
{
	int ErrorCode = 0;
	char uuid[HANDLESIZE + 4] = { 0 };
	CARmSdkManager *gsdk = CARmSdkManager::GetInstanceContext();
	if (!gsdk) {
		std::cout << "CARmSdkManager::SetFileDataCallback() fail." << std::endl;
		return FUNCTION_ERROR;
	}

	if (FRAME_TYPE_UNKNOWN == nFrameType) {
		gsdk->m_pLog->Info("SetFileDataCallback Unknow FrameType");
		return FUNCTION_SUCCESS;
	}

	//转换nSessionID to uuid
	uuid_unparse(nSessionID, uuid);
	m_FileOffset = nFileOffset;
	m_pFilename = szCurFileName;
//    gsdk->m_pLog->Debug("SetFileDataCallback nFrameType:%d,CurFileName:%s",nFrameType,m_pFilename);

	if ((ErrorCode = G_SendMdvrRealData(uuid, nFrameType, nFrameLen, pFrameData,
			nPTS, NULL, nFileOffset, pExtraDataBuf, nExtraDataLen)))
	{
		gsdk->m_pLog->Info("G_SendMdvrRealData SetDownRealData error;Handle[%s];ErrCode[%d];", uuid, ErrorCode);

		if (SEND_MDVR_REAL_DATA_ERROR == ErrorCode) {
			int socketfd = -1;
			socketfd = m_pThreadManager->RetrunSocketPairfd();
			if (-1 == socketfd)
			{
				gsdk->m_pLog->Info("G_SendMdvrRealData RetrunSocketPairfd false!!");
				return FUNCTION_SUCCESS;
			}
			if (!G_ModifyMdvrInfoNode(uuid, socketfd, MDVRINFO_SOCKET_ADD))
			{
				gsdk->m_pLog->Info("G_SendMdvrRealData G_ModifyMdvrInfoNode:"
						"pHandle[%s],socketfd[%d] false!!", uuid, socketfd);
				if (-1 == m_pThreadManager->DecreasSocketPairfd(socketfd))
					gsdk->m_pLog->Info("G_SendMdvrRealData DecreasSocketPairfd false!!");
			}
		}
	}
//	gsdk->m_pLog->Info("SetFileDataCallback()  G_SendMdvrRealData success!!!!!");

	return FUNCTION_SUCCESS;
}

int CARmSdkManager::SetFileTaskCallback(unsigned char *nSessionID, char *szMDVRID,
		int nMDVRName, char *szFileName, int nStartOffset, int nStopOffset,
		int nStateCode, int nFlagRecordType, unsigned int *nCurrentOrgFileOffset)
{
	char uuid[HANDLESIZE + 4] = { 0 };
	CARmSdkManager *gsdk = CARmSdkManager::GetInstanceContext();
	if (!gsdk)
		return FUNCTION_ERROR;

	gsdk->m_pLog->Info("SetFileTaskCallback invoked;\n\tSessionID[%s];\n\tMDVRID[%s];\n\t"
					"MDVRName[%d];\n\tFileName[%s];\n\tStateCode[%d];\n\tFlagRecordType[%d];"
					"\n\tCurrentOrgFileOffset[%d];",uuid, szMDVRID, nMDVRName, szFileName, nStateCode, nFlagRecordType, *nCurrentOrgFileOffset);
	uuid_unparse(nSessionID, uuid);

	string key = KEYPRE;
//	key += uuid;
	key += ":";
	key += szFileName;
	string value;

	if (m_pRedisclient->ReturnRedisContext()->err) {
		CARedisClient* t_rc;
		t_rc = new CARedisClient(g_data.RedisIp.c_str(), g_data.RedisPort);
		if (CARedisClient::m_InitStatus) {
			delete m_pRedisclient;
			m_pRedisclient = t_rc;
		}
	}
	if (nStateCode != 2) {
		if (m_pRedisclient->HashExists(key.c_str(), FIELD)) {
			string temp = m_pRedisclient->HashGetValue(key.c_str(), FIELD);
			*nCurrentOrgFileOffset = atoi(temp.c_str());
			gsdk->m_pLog->Debug("Last point:%d", *nCurrentOrgFileOffset);
		} else {
			value = boost::lexical_cast<string>(*nCurrentOrgFileOffset);
			if (m_pRedisclient->HashAdd(key.c_str(), FIELD, value.c_str())) {
				gsdk->m_pLog->Error("HashAdd filename error");
				return FUNCTION_ERROR;
			}
		}
	}

	if (MDVR_MANUFACTURE_RM != nMDVRName) {
		gsdk->m_pLog->Info("MDVR MANUFACTURE NO RM,REGISTER NO REPLY");
		return FUNCTION_ERROR;
	}

	//success return 0;
	if (G_ModifyFileDownInfoList(uuid, nMDVRName, szFileName, nStateCode,
			nStartOffset, nStopOffset, nFlagRecordType, 0,0))
	{
		return FUNCTION_ERROR;
	}

	gsdk->m_pLog->Info("After SetFileTaskCallback() MdvrListAllNums=%d",G_GetMdvrInfoListNums());

	return FUNCTION_SUCCESS;
}
CARmSdkManager::CARmSdkManager() :
		isExit(0), m_Zeromqport("") {
	m_MdvrBlackList.clear();
	m_pLog = CALog::GetInstance(g_data.ZlogRmCname);
	m_pLog->Info("CARmSdkManager::CARmSdkManager()");

#if FRAME_WRITE_FILE
	//string filepath = "/mnt/hgfs/share/006A00E5B7_test.ts";
	string filepath = g_data.CurrentPath + "test.h264";
	if (!m_VideoFile.fopen(filepath.c_str(),"w+b"))
	{
		m_pLocal_log->WriteZlog(m_pZlog_category, Z_LEVEL_INFO,
				"m_VideoFile[test.ts]fopen false;");
	}
#endif
}

CARmSdkManager::~CARmSdkManager() {
//	if (VMS_MDVR_SDK_StopListen())
//		m_pLog->Info("VMS_MDVR_SDK_StopListen error");
//	if (VMS_MDVR_SDK_Cleanup())
//		m_pLog->Info("VMS_MDVR_SDK_Cleanup error");
#if FRAME_WRITE_FILE
	m_VideoFile.fclose();
#endif
	m_pLog->Debug("CARmSdkManager::~CARmSdkManager()");
}

/*
 * explain: 把厂商相关的skd信息存储到全局指针上,方便其他类操作
 * （每个sdk是单独的进程,全局指针会指向自己到sdk信息节点）
 * par: sdk厂家接口相关参数信息
 * ret: success 0; erro -1;
 */
int CARmSdkManager::InvokSdkFunction(SdkConfInfo *sdkinfo)
{
	sdkinfo->ZlogCname = g_data.ZlogRmCname;
	g_data.pSci = sdkinfo;

	try {
			m_pThreadManager = CAThreadManager::GetInstanceContext();
			m_pLog->Info("Loading SDK Success!!!,listenPort=%d",
						sdkinfo->sdkport ? sdkinfo->sdkport : 8080);

		} catch (const CAException& exp) {
			m_pLog->Info("Loading SDK Error!!!,listenPort=%d",
						sdkinfo->sdkport ? sdkinfo->sdkport : 8080);

			return FUNCTION_ERROR;
		}

#if 0
		int errstat = 0;
			VMS_VERSION_INFO versionInfo = { 0 };
	if ((errstat = VMS_MDVR_SDK_Startup(CARmSdkManager::WriteLogCallback))) {
		m_pLog->Info("stat=%d;%s", errstat, "VMS_MDVR_SDK_Startup error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_SetConnectTime(sdkinfo->timeout))) {
		m_pLog->Info("stat=%d;%s", errstat,
				"VMS_MDVR_SDK_SetConnectTime error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_SetStateCallBack(
			CARmSdkManager::SetStateCallback))) {
		m_pLog->Info("stat=%d;%s", errstat,
				"VMS_MDVR_SDK_SetStateCallBack error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_SetMediaRegisterCallBack(
			CARmSdkManager::SetMediaRegisterCallback))) {
		m_pLog->Info("stat=%d;%s", errstat,
				"VMS_MDVR_SDK_SetMediaRegisterCallBack error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_SetMediaDisconnectCallBack(
			CARmSdkManager::SetMediaDisconnectCallback))) {
		m_pLog->Info("stat=%d;%s", errstat,
				"VMS_MDVR_SDK_SetMediaDisconnectCallBack error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_SetRealDataCallBack(
			CARmSdkManager::SetRealDataCallback))) {
		m_pLog->Info("stat=%d;%s", errstat,
				"VMS_MDVR_SDK_SetRealDataCallBack error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_SetFileDataCallBack(
			CARmSdkManager::SetFileDataCallback))) {
		m_pLog->Info("stat=%d;%s", errstat,
				"VMS_MDVR_SDK_SetFileDataCallBack error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_SetFileTaskCallBack(
			CARmSdkManager::SetFileTaskCallback))) {
		m_pLog->Info("stat=%d;%s", errstat,
				"VMS_MDVR_SDK_SetFileTaskCallBack error");
		return FUNCTION_ERROR;
	}
	if ((errstat = VMS_MDVR_SDK_GetSDKVersion(&versionInfo))) {
		m_pLog->Info("stat=%d;%s", errstat, "VMS_MDVR_SDK_GetSDKVersion error");
		return FUNCTION_ERROR;
	} else {
		m_pLog->Info("SdKVersion=%d.%d.%d.%d", versionInfo.dwMainVer,
				versionInfo.dwSubVer, versionInfo.dwBuildDate,
				versionInfo.nTest);
	}

	if ((errstat = VMS_MDVR_SDK_Listen((char*) sdkinfo->sdkip.c_str(),
			sdkinfo->sdkport ? sdkinfo->sdkport : 8080))) {
		m_pLog->Info("stat=%d;%s", errstat, "VMS_MDVR_SDK_Listen error");
		return FUNCTION_ERROR;
	}

//#else
	 cout<<"start"<<endl;
	 CA_RM_SDK_SetMediaRegisterCallBack(CARmSdkManager::SetMediaRegisterCallback);
	 CA_RM_SDK_SetRealDataCallBack (CARmSdkManager::SetRealDataCallback);
	 CA_RM_SDK_Startup(9888);
	 cout<<"end"<<endl;
#endif


	return FUNCTION_SUCCESS;
}

/*
 * rm sdk Entery
 */
/*
 * Static function used in the callback
 * use static member method and data member
 */
/*
 * 状态回调函数
 * 参数：下层回传数据
 * 返回值：int 返回0成功,否则返回错误码
 */
/*
 * add modify Mdvrinfo list
 * 2.0 version by thread pool,reduce process time
 * nFileOffset fill 0
 */
/*
 * mdvr与视频接入服务器的媒体通道注册回调处理函数
 * 参数：下层回传数据
 * 返回值：int 返回0成功,否则返回错误码
 */
/*
 * Socketfd is no reference counting,don't operate descriptor
 * par: pHandle是实时流句柄或文件下载SessionID
 */
/*
 * V1.3.1 par szCurFileName unused;
 */
