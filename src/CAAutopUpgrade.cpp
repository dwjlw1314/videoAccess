/*
 * CAAutopUpgrade.cpp
 *
 *  Created on: May 20, 2015
 *      Author: diwenjie
 */

#include "CAAutopUpgrade.h"

CAAutopUpgrade::CAAutopUpgrade()
{
	// TODO Auto-generated constructor stub
	m_pLog = CALog::GetInstance(g_data.ZlogCname);
	m_UpgradeFileName = g_data.CurrentPath + "upgrade/";
	m_UpgradeFileName += UPGRADEFILENAME;
	SegmentationVersionInfo(m_OldVersionArray , g_data.Version);
	m_pLog->Debug("CAAutopUpgrade::CAAutopUpgrade()");
}

CAAutopUpgrade::~CAAutopUpgrade(){
	m_pLog->Debug("CAAutopUpgrade::~CAAutopUpgrade()");
}

/*
 * par: versionArray is [>3] pointer array
 *      versionValue is string value
 * version value < 999, number <= 3
 * eg: 2.5.56.Debug
 */
void CAAutopUpgrade::SegmentationVersionInfo(
	int *versionArray, string versionValue)
{
	unsigned int pos = 0;
	char buf[4] = {0};

	if (!versionArray)
		return;

	for(int i = 0; true; i++)
	{
		pos = versionValue.find(".");
		if (pos == string::npos)
			break;
		versionValue.copy(buf,pos,0);
		versionArray[i] = atoi(buf);
		versionValue = versionValue.substr(pos+1);
	}
}

/*
 * par: new version info array
 * ret: 0 is need upgrade, -1 no not need upgrade
 */
int CAAutopUpgrade::ComparVersionInfo(int *newversionArray)
{
	for(int i = 0; i < VERSIONARRAYSIZE; i++)
	{
		if (newversionArray[i] > m_OldVersionArray[i])
			return FUNCTION_SUCCESS;
	}

	return FUNCTION_ERROR;
}

int CAAutopUpgrade::CheckUpgradeFileExist()
{
	if (access (m_UpgradeFileName.c_str(), F_OK))
	{
		m_pLog->Info("Need not upgrade since no package!");
		return FUNCTION_ERROR;
	}

	string tarcommand = "tar -zxvf ";
	tarcommand += m_UpgradeFileName;
	tarcommand += " -C " + g_data.CurrentPath + "upgrade/ > /dev/null";
	system(tarcommand.c_str());
	m_pLog->Info("upgrade file[%s] decompression success!", UPGRADEFILENAME);

	return FUNCTION_SUCCESS;
}

int CAAutopUpgrade::ReadConfigFileVerInfo()
{
	int NewVersionArray[3] = {0};
	// Read the file. If there is an error, report it.
	try
	{
		m_UpgradeConfigFile.readFile("upgrade/VideoAccess.conf");
	}
	catch(const FileIOException &fioex)
	{
		m_pLog->Info("upgrade configure file no find!");
		return FUNCTION_ERROR;
	}
	catch(const ParseException &pex)
	{
		m_pLog->Info("upgrade configure file data error!");
		return FUNCTION_ERROR;
	}

	string version = m_UpgradeConfigFile.lookup("Version");
	SegmentationVersionInfo(NewVersionArray , version);
	m_pLog->Info("NewVersionInfo=[%s]",version.c_str());

	if (ComparVersionInfo(NewVersionArray))
		return FUNCTION_ERROR;

	return FUNCTION_SUCCESS;
}

/*
 * check program whether can running
 * ret: 0 ok, -1 err
 */
int CAAutopUpgrade::CheckUpgradeApplication()
{
	int datanum = 0;
	FILE *data = NULL;
	char tmp[1024] = {0};

	system("\\cp -r /etc/ld.so.conf /etc/ld.so.conf_bak");
	m_pLog->Info("upgrade back /etc/ld.so.conf file success!");

	string jmpdir = g_data.CurrentPath + "upgrade/";
	string shellname = jmpdir;
	shellname += SHELLFILENAME;
	string chmod = "chmod 755 " + shellname;
	system(chmod.c_str());
	system(shellname.c_str());
	m_pLog->Info("upgrade shell running success!");

	if(ReadConfigFileVerInfo())
	{
		system("rm -rf upgrade/*");
		m_pLog->Info("upgrade version too low!");
		system("touch upgrade/false.txt");
		system("echo \"version too low\" > upgrade/false.txt");
		return FUNCTION_ERROR;
	}

	string upname = jmpdir + PROGRAMNAME;
	chmod = "chmod 755 " + upname;
	system(chmod.c_str());
	m_pLog->Info("upgrade packet incomplete!");

	string modify = "mv " + upname;
	modify += " " + upname + "_tmp";
	system(modify.c_str());

	system("\\cp -r VideoAccess.conf /opt/");
	system("\\cp -r VideoZlog.conf /opt/");
	string copycmd = "\\cp -r ";
	copycmd += PROGRAMNAME;
	copycmd += " /opt/";
	system(copycmd.c_str());
	m_pLog->Info("original program already move to /opt!");

	string uppackname = "mv -f upgrade/";
	uppackname += UPGRADEFILENAME;
	uppackname += " /tmp/";
	system(uppackname.c_str());
	m_pLog->Info("upgrade packet move to /tmp/");

	system("mv upgrade/VideoAccess.conf pgrade/VideoAccess.conf_bak");
	system("mv upgrade/VideoAccess_upgrade.conf pgrade/VideoAccess.conf");

	string exe = upname + "_tmp";
	system(exe.c_str());

	string opencmd = "ps ax|grep ";
	opencmd += PROGRAMNAME;
	opencmd += "_tmp";
	data = popen(opencmd.c_str(), "r");
	if (!data)
	{
		chdir(g_data.CurrentPath.c_str());
		system("rm -rf upgrade/*");
		system("touch upgrade/false.txt");
		system("echo \"popen false\" > upgrade/false.txt");
		m_pLog->Info("upgrade popen false!");
		return FUNCTION_ERROR;
	}

	while(fgets(tmp,sizeof(tmp),data) != NULL)
		datanum ++;

	if (datanum < 3)
	{
		pclose(data);
		chdir(g_data.CurrentPath.c_str());
		system("rm -rf upgrade/*");
		system("touch upgrade/false.txt");
		system("echo \"program start false\" > upgrade/false.txt");
		m_pLog->Info("upgrade program start false!");
		return FUNCTION_ERROR;
	}
	pclose(data);

	string kill = "killall ";
	kill += PROGRAMNAME;
	kill += "_tmp";
	system(kill.c_str());

	system("mv upgrade/VideoAccess.conf upgrade/VideoAccess_upgrade.conf");
	system("mv upgrade/VideoAccess.conf_bak upgrade/VideoAccess.conf");

	system("rm -rf upgrade/Rm*.log");
	system("rm -rf upgrade/VideoAccess*.log");

	m_pLog->Info("CheckUpgradeApplication ok!!");

	chdir(jmpdir.c_str());

	system("unalias cp");
	system("cp -r * ../");

	return FUNCTION_SUCCESS;
}
