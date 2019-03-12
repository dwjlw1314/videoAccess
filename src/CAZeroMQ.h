/*
 * CAZeroMQ.h
 *
 *  Created on: Dec 2, 2015
 *      Author: Xiangbo liu
 */

#ifndef CAZEROMQ_H_
#define CAZEROMQ_H_

#include <zmq.h>
#include <map>
#include "CAPublicStruct.h"
#include "CAThreadMutex.h"
#include "CAException.h"
#include "CALog.h"

#define DEFAULT_PORT 5558

class CAZeroMQ: public noncopyable {
public:
	///
	static CAZeroMQ* GetInstance(const char* connType,const char* addr, const int port, const int sockType);
	int SendData(uint8_t* pdata, size_t size, ZmqData *pExData) throw(CAException);
	int RecvData(uint8_t* pdata) throw(CAException);

private:
	static std::map<std::string,CAZeroMQ*> *m_pMqMap;
    void *m_pSocket;
	void *m_pZmqctx;  //can not release memory if same it as a part variable
    CALog *m_pLog;
    CAThreadMutex* m_pThreadMutex;

	CAZeroMQ(const char* endpoint, const int sockType);
	int Send(const ZmqData &data) throw(CAException);
	virtual ~CAZeroMQ();
};

#endif /* CAZEROMQ_H_ */
