/*
 * CAException.h
 *
 *  Modified on: Dec 2, 2015
 *      Author: Xiangbo liu
 */
#ifndef _Sockets_CAException_H
#define _Sockets_CAException_H

#include <string>
#include <exception>
#include <map>

class CAException:std::exception
{
public:
	CAException(const std::string& desc);
	CAException(int code = 0);

	virtual ~CAException() throw(){}

	//copy constructor
	CAException(const CAException& tmp) { this->m_code = tmp.m_code;this->m_desc = tmp.m_desc;}

	/** Returns a C-style character string describing the general cause
	     *  of the current error.  */
	virtual const char* what() const throw();

private:
	std::string m_desc;
	int m_code;
    static std::map<int,std::string> *m_pExceptMap;

    void initError();
};

#endif // _Sockets_CAException_H

