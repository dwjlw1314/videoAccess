/**
 * Name        : VAS_Access.cpp
 * Author      : diwenjie
 * Version     : 1.4 Version (32 library invoked)
 * Modify	   : 2015-08-30
 * Copyright   : Your copyright notice
 * Description : VAS_Access in C++, Ansi-style
 *
 * Ver format:
 * 	MajorVerson.MinorVersion.RevisionVersion.BuildVersion
 * 	eg: 1.2.1.build-1311;
 * 	BuildVersion only (Debug and Release),after number can cancel
 *
 * Module Using libconfig, RM-sdk, zlog-latest-stable, ffmpeg-2.4.2
 * libx264-20141126, redis-client, zeromq-4.1.0
 * class data member example m_xxx or m_pxxx
 * public data struct example
 * global function name G_xxx()
 *
 * Local all class begin by CA+*
 * include *.so library: libconfig-1.4.8 , zlog-latest-stable
 *   ffmpeg-2.4.2, redis-client, zeromq-4.1.0, libx264-20141126
 *   vo-aacenc_0.1.3
 *
 * Compile time library added in the property <config++,boost_thread,x264,
 *  hiredis, avdevice, avutil, avformat, avcodec, VMS_MDVR_SDK_RM, zlog,
 *  pthread, uuid>
 *
 * 引用32lib <libboost_thread.so> 需要符号链接64的lib文件libboost_thread-mt.so.*
 *
 * This project adopts(采用) the single vendor and more threads process tasks.
 *
 * Due to the advantages unique to Linux,Linux process is more efficient
 * and lightweight,so Linux preferred process programming
 *
 * 调试子进程方式:
 * 在当前目录新建一个.gdbinit文件
 * 加入set follow-fork-mode child
 **/

/*
 * c++ head include
 */
#include <iostream>
#include "CAPublicStruct.h"
#include "CASdkManagement.h"

/*
 * All namespace
 */
using namespace std;

/*
 * unuesd par argc,argv;;
 * 主函数中各个厂家sdk信息处理方式暂时采用一家，如果多sdk需要修改处理流程
 * 保证项目时间要求没有考虑
 */
int main(int argc, char**argv) {
	Config cfg;
	CASdkManagement *psdkManagement = NULL;
	pSdkConfInfo pSdkConfNode = NULL;
	CALog *log = NULL;
	int ret = 0;

	// Read the file. If there is an error, report it and exit.
	try {
		cfg.readFile("VideoAccess.conf");
	} catch (const FileIOException &fioex){
		std::cerr << "I/O error while reading file." << std::endl;
		return (EXIT_FAILURE);
	} catch (const ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
				<< " - " << pex.getError() << std::endl;
		return (EXIT_FAILURE);
	}

	try {
		pSdkConfNode = new SdkConfInfo();
		ret = G_InitGlobalDefine(&cfg, pSdkConfNode);
		if (ret) {
			std::cerr << "G_InitGlobalDefine() flase!" << endl;
			return (EXIT_FAILURE);
		}
		log = CALog::GetInstance(g_data.ZlogConfPath, "video_cat");
		G_SetSecurityLimit(RLIMIT_NOFILE, 65535, 65535);
		atexit(G_FinitGlobalData);
	} catch (const SettingNotFoundException &sfexp) {
		std::cerr << "VideoAccess.conf is not complete!" << endl;
		return (EXIT_FAILURE);
	} catch (const CAException &caexp) {
		std::cerr << caexp.what() << endl;
		return (EXIT_FAILURE);
	}

	/*
	 * Exception handing,Reload the various manufacturers to sdk
	 * Each manufacturer sdk process monitoring.
	 * Unable to process returns said to the wrong
	 */
	try {

		/*
			 * Enable process maintenance program
			 * par: argc > 1; argv[1] = "-r";
		*/
		if (g_data.EnableDaemon && 1 == argc) {
			log->Info("Start Main, Enable daemon!\n\tCurrentRunningPath=%s\n\t"
		"Main ThreadId=%lu;Process PID=%d",
		g_data.CurrentPath.c_str(), pthread_self(), getpid());
			G_EnableDaemon();
			G_ShadowDaemon(argv[0]);
		}
		else{
			log->Info("Start Main, Disable daemon!\nCurrentRunningPath=%s\n"
					"Main ThreadId=%lu;Process PID=%d",
					g_data.CurrentPath.c_str(), pthread_self(), getpid());
		}
		psdkManagement = new CASdkManagement();
		if (psdkManagement->SetSdkConfInfo(pSdkConfNode)) {
			if (g_data.EnableDaemon) {
				log->Info("Main SetSdkConfInfo false!");
			} else {
				std::cerr << "Main SetSdkConfInfo false!!" << endl;
			}
			return (EXIT_FAILURE);
		}
		psdkManagement->StartLoadSdk();
	} catch (const CAException &caexp) {
		log->Info(caexp.what());
		std::cerr << caexp.what() << endl;
	}

	log->Info("End Main!");
	CALog::Finish();
	delete pSdkConfNode;
	delete psdkManagement;
	return 0;
}
