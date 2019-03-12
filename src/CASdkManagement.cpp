/*
 * CASdkManagement.cpp
 *
 *  Created on: Jan 30, 2015
 *      Author: diwenjie
 */

#include "CASdkManagement.h"

CASdkManagement::CASdkManagement()
	:m_pSdk_conf_info(NULL),m_pRmsdk_manager(NULL)
{
	m_pLog = CALog::GetInstance(g_data.ZlogCname);
	m_pLog->Debug("CASdkManagement::CASdkManagement()");

}

CASdkManagement::~CASdkManagement()
{
	m_pLog->Debug("CASdkManagement::~CASdkManagement()");
}

/*
 * par: sdk about info struct
 * add sdk pointer to member method
 */
int CASdkManagement::SetSdkConfInfo(SdkConfInfo* psdkconf)
	throw(CAException)
{
#if 0
	pSdkConfInfo pSdkTmpConfNode = m_pSdk_conf_info;
	if (!m_pSdk_conf_info)
	{
		m_pSdk_conf_info = psdkconf;
	}
	else
	{
		while(pSdkTmpConfNode->next)
			pSdkTmpConfNode = pSdkTmpConfNode->next;
		pSdkTmpConfNode = psdkconf;
	}
#endif

	if (!m_pSdk_conf_info)
	{
		m_pSdk_conf_info = psdkconf;
	}
	else
	{
		return FUNCTION_ERROR;
	}

	return FUNCTION_SUCCESS;
}

/*
 * sdk loading begin to place
 */
void CASdkManagement::StartLoadSdk() throw(CAException)
{
	m_pLog->Info("Loading SDK! SDK-Count=%d, Name=%s ",g_data.SDkCount, g_data.ZlogRmCname.c_str());

	if (g_data.Version.compare(PROCESSVERSION))
	{
		m_pLog->Info("Process Version[%s]\nConfig Version[%s]",
			PROCESSVERSION, g_data.Version.c_str());
		throw(CAException("Version Compare Error,Check Configure File!"));
	}

	m_pRmsdk_manager = CARmSdkManager::GetInstanceContext();
	if (m_pRmsdk_manager)
	{
		m_pRmsdk_manager->StartRmSdkInit(m_pSdk_conf_info);
	}
	else
	{
		m_pLog->Error("CARmSdkManager::GetInstanceContext() fail.");
		throw(CAException("Get RmSDK Instances fail!"));
	}
#if USE_FORK
	CreateSdkProcess();
#endif
}

/*
 * Create every sdk process,save pid to SdkConfInfo
 * The late maintenance can be performed by PID
 */
int CASdkManagement::CreateSdkProcess() throw(CAException)
{
	pid_t pid;
	int status = 0;
	pSdkConfInfo sdkinfo = m_pSdk_conf_info;

	while(sdkinfo)
	{
		/*
		 * the switch syntax Rely on sdkid Identification
		 * Not currently in use "switch(sdkinfo->sdkid){}"
		 */
		if (!sdkinfo->sdkname.compare("RM"))
		{
			pid = fork();
			if (0 == pid)
			{
				//Different manufacturers of SDK treatment code Area.
			}
			else if (0 < pid)
			{
				sdkinfo->pid = pid;
			}
			else
			{
				//fork error Area.
			}
		}
		else if (!sdkinfo->sdkname.compare("other_sdk_name"))
		{
			//Other manufacturers ditto treatment method
		}
		sdkinfo = sdkinfo->next;
	}
	pid = waitpid(-1,&status,0);
	m_pLog->Info("sdk process No. %d is exit",pid);
	return FUNCTION_SUCCESS;
}


