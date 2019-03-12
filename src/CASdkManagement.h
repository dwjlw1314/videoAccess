/*
 * CASdkManagement.h
 *
 *  Created on: Jan 30, 2015
 *      Author: diwenjie
 */

#ifndef CASDKMANAGEMENT_H_
#define CASDKMANAGEMENT_H_

#include "CAPublicStruct.h"
#include "CARmSdkManager.h"

class CASdkManagement
{
public:
	CASdkManagement();
	virtual ~CASdkManagement();

	/*
	 * load sdk Entry
	 */
	void StartLoadSdk() throw(CAException);
	int CreateSdkProcess() throw(CAException);
	int SetSdkConfInfo(SdkConfInfo* psdkconf) throw(CAException);

	/*
	 * 获取当前程序路径，并保存在path中
	 */
	void GetCurrentPath(string &path);
private:
	/*
	 * sdk厂家的相关信息结构
	 * Each manufacturer to a class
	 */
	pSdkConfInfo m_pSdk_conf_info;
	CALog *m_pLog;
	CARmSdkManager *m_pRmsdk_manager;
};

#endif /* CASDKMANAGEMENT_H_ */
