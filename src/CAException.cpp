/*
 * CAException.cpp
 *
 *  Modified on: Dec 2, 2015
 *      Author: Xiangbo liu
 */
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif
#include "CAException.h"

std::map<int,std::string>* CAException::m_pExceptMap = NULL;

CAException::CAException(const std::string& desc): m_desc(desc){
	this->m_code = 0;
}

CAException::CAException(int code): m_code(code){
	if(m_pExceptMap == NULL)
		initError();
	std::map<int,std::string>::iterator ite;
	for(ite=m_pExceptMap->begin();ite!=m_pExceptMap->end();ite++){
	    if(this->m_code == ite->first){
	    	this->m_desc=ite->second;
	    }
	}
    this->m_desc="NO EXCEPTION";
}

const char* CAException::what() const throw () {
	return this->m_desc.c_str();
}

void CAException::initError() {
	m_pExceptMap = new std::map<int,std::string>();
	(*m_pExceptMap)[1002]="MUTEX_INIT_EXCEPTION CAThreadMutex pthread_mutexattr_init error";
	(*m_pExceptMap)[1003]="MUTEX_DEADLOCK_EXCEPTION CAThreadMutex lock error";
	(*m_pExceptMap)[1004]="MUTEX_LOCK_EXCEPTION CAThreadMutex lock error";
	(*m_pExceptMap)[1005]="MUTEX_TRYLOCK_EXCEPTION CAThreadMutex tryLock error";
	(*m_pExceptMap)[1006]="MUTEX_UNLOCK_EXCEPTION CAThreadMutex unlock error";
	(*m_pExceptMap)[2001]="can not find the category in config file!";
	(*m_pExceptMap)[2002]="Please call CALog(const std::string &path, const std::string &catalogName!";
	//(*m_pExceptMap)[3001]="CAThreadMutex pthread_mutexattr_init error";
	(*m_pExceptMap)[3001]="zmq_ctx_new error";
	(*m_pExceptMap)[3002]="zmq_socket error";
	(*m_pExceptMap)[3003]="zmq_bind error";
}
