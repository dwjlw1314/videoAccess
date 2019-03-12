/*
 * CAPerAnalysis.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: diwenjie
 */

#include "CAPerAnalysis.h"

CAPerAnalysis::CAPerAnalysis(const char* func_name)
{
	// TODO Auto-generated constructor stub
	gettimeofday(&m_tv, NULL);
	m_pFunc_name = func_name;
}

CAPerAnalysis::~CAPerAnalysis()
{
	// TODO Auto-generated destructor stub
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	long cost = (tv2.tv_sec - m_tv.tv_sec) * 1000000
				+ (tv2.tv_usec - m_tv.tv_usec);
	printf("\r\nfunction_name=%s;run_cost=%ld\r\n",m_pFunc_name,cost);
	//Add post to some manager
}

