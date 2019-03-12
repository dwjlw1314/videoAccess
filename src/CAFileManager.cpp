/*
 * CAFileManager.cpp
 *
 *  Created on: Mar 16, 2015
 *      Author: diwenjie
 */

#include "CAFileManager.h"

CAFileManager::CAFileManager()
	:m_pFile(NULL),m_Rptr(0),m_Wptr(0)
{
	// TODO Auto-generated constructor stub
}

CAFileManager::~CAFileManager()
{
	// TODO Auto-generated destructor stub
}

bool CAFileManager::fopen(const std::string& path, const std::string& mode)
{
	m_Path = path;
	m_Mode = mode;
	m_pFile = ::fopen(path.c_str(), mode.c_str());

	return m_pFile ? true : false;
}

void CAFileManager::fclose() const
{
	if (m_pFile)
	{
		::fclose(m_pFile);
		m_pFile = NULL;
	}
}

size_t CAFileManager::fread(char *ptr, size_t size, size_t nmemb) const
{
	size_t r = 0;
	if (m_pFile)
	{
		fseek(m_pFile, m_Rptr, SEEK_SET);
		r = ::fread(ptr, size, nmemb, m_pFile);
		m_Rptr = ftell(m_pFile);
	}
	return r;
}

size_t CAFileManager::fwrite(const char *ptr, size_t size, size_t nmemb)
{
	size_t r = 0;
	if (m_pFile)
	{
		fseek(m_pFile, m_Wptr, SEEK_SET);
		r = ::fwrite(ptr, size, nmemb, m_pFile);
		m_Wptr = ftell(m_pFile);
	}
	fflush(m_pFile);
	return r;
}

char *CAFileManager::fgets(char *s, int size) const
{
	char *r = NULL;
	if (m_pFile)
	{
		fseek(m_pFile, m_Rptr, SEEK_SET);
		r = ::fgets(s, size, m_pFile);
		m_Rptr = ftell(m_pFile);
	}
	return r;
}


void CAFileManager::fprintf(const char *format, ...)
{
	if (!m_pFile)
		return;
	va_list ap;
	va_start(ap, format);
	fseek(m_pFile, m_Rptr, SEEK_SET);
	vfprintf(m_pFile, format, ap);
	m_Rptr = ftell(m_pFile);
	va_end(ap);
}

void CAFileManager::vsfprintf(const char *format, ...)
{
	if (!m_pFile)
		return;
	va_list ap;
	va_start(ap, format);
	fseek(m_pFile, m_Rptr, SEEK_SET);
	vfprintf(m_pFile, format, ap);
	m_Rptr = ftell(m_pFile);
	va_end(ap);
}

void CAFileManager::reset_read() const
{
	m_Rptr = 0;
}


void CAFileManager::reset_write()
{
	m_Wptr = 0;
}

bool CAFileManager::eof() const
{
	if (m_pFile)
	{
		if (feof(m_pFile))
			return true;
	}
	return false;
}


off_t CAFileManager::size() const
{
	struct stat st;
	if (stat(m_Path.c_str(), &st) == -1)
	{
		return 0;
	}
	return st.st_size;
}
