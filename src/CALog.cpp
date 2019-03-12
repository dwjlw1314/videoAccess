/*
 * CALog.cpp
 *
 *  Created on: Dec 1, 2015
 *      Author: Xiangbo liu
 */

#include "CALog.h"

std::map<std::string, CALog*> *CALog::m_pLogMap = NULL;

CALog::CALog(const std::string &catalogName) {
	m_pCat = zlog_get_category(catalogName.c_str());
	if(m_pCat== NULL)
		throw CAException(2001) ;
	m_pLog = this;
}

CALog* CALog::GetInstance(const std::string& confFilePath,
		const std::string& catalogName) {
	int rc = 0;
	CALog* tmp = NULL;
	rc = zlog_init(confFilePath.c_str());
	if (!rc) //0 zlog_init is success
	{
		if (m_pLogMap != NULL) {
			std::cout << "m_pLogMap not equal null" << std::endl;
			std::map<std::string, CALog*>::iterator tmp;
			for (tmp = m_pLogMap->begin(); tmp != m_pLogMap->end(); tmp++)
				delete tmp->second;
			m_pLogMap->clear();
		} else {
			m_pLogMap = new std::map<std::string, CALog*>();
		}
		tmp = new CALog(catalogName);
		m_pLogMap->insert(std::pair<std::string, CALog*>(catalogName, tmp));
	} else {
		std::cout
				<< "Fail to intialize log,please check the VideoZlog.cfg and format!!"
				<< std::endl;
		exit(0);
	}
	return tmp;
}


CALog* CALog::GetInstance(const std::string& catalogName) {
	if (m_pLogMap == NULL) {
		throw CAException(2002);
	}
	CALog* tmp = NULL;
	std::map<std::string, CALog*>::iterator ite;
	bool first = catalogName.empty();
	if (!first){
		ite = m_pLogMap->find(catalogName);
	}
	else
		ite = m_pLogMap->find("video_cat");

	if (ite == m_pLogMap->end()) {
		tmp = new CALog(catalogName);
		m_pLogMap->insert(std::pair<std::string, CALog*>(catalogName, tmp));
		return tmp;
	} else
		return ite->second;
}

void CALog::Finish(){
	if (m_pLogMap != NULL) {
		std::map<std::string, CALog*>::iterator tmp;
		for (tmp = m_pLogMap->begin(); tmp != m_pLogMap->end(); tmp++)
			delete tmp->second;
		m_pLogMap->clear();
	}
	zlog_fini();
}
void CALog::Debug(const char* format, ...) {
//#ifdef DEBUG
	if (NULL == format)
			return;
	char msg[1024] = "0";
	va_list arg_ptr = {};
	va_start(arg_ptr,format);
	vsprintf(msg,format,arg_ptr);
	va_end(arg_ptr);
	zlog_debug(m_pCat, msg);
//#endif
}


void CALog::Info(const char* format, ...) {
	if (NULL == format)
				return;
		char msg[1024] = "0";
		va_list arg_ptr = {};
		va_start(arg_ptr,format);
		vsprintf(msg,format,arg_ptr);
		va_end(arg_ptr);
		zlog_info(m_pCat, msg);}

void CALog::Notice(const char* format, ...) {
	if (NULL == format)
				return;
		char msg[1024] = "0";
		va_list arg_ptr = {};
		va_start(arg_ptr,format);
		vsprintf(msg,format,arg_ptr);
		va_end(arg_ptr);
		zlog_debug(m_pCat, msg);}

void CALog::Warn(const char* format, ...) {
	if (NULL == format)
				return;
		char msg[1024] = "0";
		va_list arg_ptr = {};
		va_start(arg_ptr,format);
		vsprintf(msg,format,arg_ptr);
		va_end(arg_ptr);
		zlog_warn(m_pCat, msg);}

void CALog::Error(const char* format, ...) {
	if (NULL == format)
				return;
		char msg[1024] = "0";
		va_list arg_ptr = {};
		va_start(arg_ptr,format);
		vsprintf(msg,format,arg_ptr);
		va_end(arg_ptr);
		zlog_error(m_pCat, msg);}

void CALog::Fatal(const char* format, ...) {
	if (NULL == format)
				return;
		char msg[1024] = "0";
		va_list arg_ptr = {};
		va_start(arg_ptr,format);
		vsprintf(msg,format,arg_ptr);
		va_end(arg_ptr);
		zlog_fatal(m_pCat, msg);}

CALog::~CALog() {
	zlog_fini();
}

