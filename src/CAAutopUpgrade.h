/*
 * CAAutopUpgrade.h
 *
 *  Created on: May 20, 2015
 *      Author: diwenjie
 */

/* Statement:
 * upgrade packet name SumaVideoAccessUpgrade.tar.gz
 * include executable programs, configure file,
 * Related library file, shell file , *.iso file
 */

#ifndef CAAUTOPUPGRADE_H_
#define CAAUTOPUPGRADE_H_

#include "CAPublicStruct.h"
#include "CALog.h"

#define VERSIONARRAYSIZE 3

class CAAutopUpgrade
{
public:
	CAAutopUpgrade();
	virtual ~CAAutopUpgrade();

	int CheckUpgradeFileExist();
	int CheckUpgradeApplication();

protected:
	void SegmentationVersionInfo(int *versionArray, string versionValue);
	int ReadConfigFileVerInfo();
	int ComparVersionInfo(int *newversionArray);

protected:
	int m_OldVersionArray[VERSIONARRAYSIZE];
	CALog *m_pLog;
	Config m_UpgradeConfigFile;
	std::string m_UpgradeFileName;
};

#endif /* CAAUTOPUPGRADE_H_ */
