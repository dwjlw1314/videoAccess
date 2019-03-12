/*
 * CAFileManager.h
 *
 *  Created on: Mar 16, 2015
 *      Author: diwenjie
 */

//test used

#ifndef CAFILEMANAGER_H_
#define CAFILEMANAGER_H_

#include "CAPublicStruct.h"

class CAFileManager
{
public:
	CAFileManager();
	virtual ~CAFileManager();

	bool fopen(const std::string&, const std::string&);
	void fclose() const;

	size_t fread(char *, size_t, size_t) const;
	size_t fwrite(const char *, size_t, size_t);

	char *fgets(char *, int) const;
	void fprintf(const char *format, ...);

	void vsfprintf(const char *format, ...);

	off_t size() const;
	bool eof() const;

	void reset_read() const;
	void reset_write();

private:
	CAFileManager(const CAFileManager&){m_pFile=NULL;m_Wptr=-1;m_Rptr = -1;}
	CAFileManager& operator=(const CAFileManager&)
	{
		return *this;
	}

protected:
	std::string m_Path;
	std::string m_Mode;
	mutable FILE *m_pFile;
	mutable long m_Rptr;
	long m_Wptr;
};

#endif /* CAFILEMANAGER_H_ */
