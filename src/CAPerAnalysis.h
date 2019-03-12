/*
 * CAPerAnalysis.h
 *
 * Performance Analysis class
 * now test function run times
 *
 *  Created on: Feb 12, 2015
 *      Author: diwenjie
 */

#ifndef CAPERANALYSIS_H_
#define CAPERANALYSIS_H_

#include <time.h>
#include <sys/time.h>
#include <stdio.h>

class CAPerAnalysis
{
public:
	CAPerAnalysis(const char* func_name);
	virtual ~CAPerAnalysis();

private:
	struct timeval m_tv;
	const char* m_pFunc_name;
};

#define PERANALYSIS() CAPerAnalysis ____CAPerAnalysis_instance## \
			__LINE__(__FUNCTION__)

/*
 * example:
 * void func()
 * {
 * 	  PERANALYSIS();
 * 	  sleep(100);
 * 	  printf("%s\n","this function end!!");
 * }
 */

#endif /* CAPERANALYSIS_H_ */
