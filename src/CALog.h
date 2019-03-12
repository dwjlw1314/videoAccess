/*
 * CALog.h
 *
 *  Created on: Dec 1, 2015
 *      Author: Xiangbo liu
 */

#ifndef CALOG_H_
#define CALOG_H_

#include <iostream>
#include <map>
#include <zlog.h>
#include <stdlib.h>
#include <boost/noncopyable.hpp>
#include "CAException.h"

//The temporary lifting could't find the problem
typedef struct zlog_category_s zlog_category;

class CALog : public boost::noncopyable{

private:
	static std::map<std::string, CALog*> *m_pLogMap;
	CALog* m_pLog;
	zlog_category *m_pCat;

	CALog(const std::string &catalogName);

public:
	///
	///it should be called at the first time,  confFilePath must be given
	///
	static CALog* GetInstance(const std::string &confFilePath, const std::string &catalogName);

	///
	///get the CLog object with the catalogName
	///
	static CALog* GetInstance(const std::string &catalogName ="");
	static void Finish();

	void Debug(const char* format = "%s", ...);
	void Info(const char* format = "%s", ...);
	void Notice(const char* format = "%s", ...);
	void Warn(const char* format = "%s", ...);
	void Error(const char* format = "%s", ...);
	void Fatal(const char* format = "%s", ...);
	virtual ~CALog();
};

#endif /* CALOG_H_ */
