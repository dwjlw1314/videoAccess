/*
 * CAZeroMQ.cpp
 *
 *  Created on: Dec 2, 2015
 *      Author: Xiangbo liu
 */
#include "CAZeroMQ.h"

std::map<std::string, CAZeroMQ*> *CAZeroMQ::m_pMqMap = NULL;

CAZeroMQ* CAZeroMQ::GetInstance(const char* connType, const char* addr,
		const int port, const int sockType) {

	string endpoint = connType;
	endpoint += "://";
	endpoint += addr;
	endpoint += ":";
	endpoint += boost::lexical_cast<string>(port);
	CAZeroMQ* tmp = NULL;
	if (!m_pMqMap) {
		m_pMqMap = new std::map<std::string, CAZeroMQ*>();
		tmp = new CAZeroMQ(endpoint.c_str(), sockType);
		m_pMqMap->insert(std::pair<string, CAZeroMQ*>(endpoint, tmp));
	} else {
		std::map<std::string, CAZeroMQ*>::iterator ite;
		ite = m_pMqMap->find(endpoint);
		if (ite != m_pMqMap->end())
			tmp = ite->second;
	}
	return tmp;
}

int CAZeroMQ::SendData(uint8_t* pdata, size_t size, ZmqData *pExData)
		throw (CAException) {
	size_t leftSize = size;
	ZmqData data = *pExData;

//	this->m_pLog->Debug("Send the size of data=%d", leftSize);

	m_pThreadMutex->lock();
	while (1) {
		if (leftSize != 0) {
			memset(data.fData, 0, SENDSIZE);
			if (leftSize < SENDSIZE)
				data.fActualSize = leftSize;
			else
				data.fActualSize = SENDSIZE;
			memcpy(data.fData, pdata, data.fActualSize);
			pdata += data.fActualSize;
			leftSize -= data.fActualSize;
		} else
			break;
		try {
			if (Send(data)) {
				m_pThreadMutex->unlock();
				return FUNCTION_ERROR;
			}
		} catch (CAException &excp) {
			this->m_pLog->Error("%s", excp.what());
		}
	}

	m_pThreadMutex->unlock();
	return FUNCTION_SUCCESS;
}

int CAZeroMQ::RecvData(uint8_t* pdata) throw (CAException) {
	zmq_msg_t m_msg;

	zmq_msg_init(&m_msg);
	int size = zmq_msg_recv(&m_msg, m_pSocket, 0);
	pdata = (uint8_t*) malloc(size);
	memcpy(pdata, zmq_msg_data(&m_msg), size);
	zmq_msg_close(&m_msg);
	free(pdata);
	return size;
}

CAZeroMQ::CAZeroMQ(const char * endpoint, const int sockType) {
	this->m_pLog = CALog::GetInstance(g_data.ZlogRmCname);
	this->m_pThreadMutex = new CAThreadMutex(g_data.ZlogRmCname);

	//zmq_ctx_new() need to release when the data send over
	m_pZmqctx = zmq_ctx_new();
	if (!m_pZmqctx) {
		m_pLog->Info("zmq_ctx_new error:%s;", (char*) zmq_strerror(errno));
		throw CAException(3001);
	}

	m_pSocket = zmq_socket(m_pZmqctx, sockType);
	if (!m_pSocket) {
		zmq_ctx_destroy(m_pZmqctx);
		m_pLog->Info("zmq_socket error:%s;", (char*) zmq_strerror(errno));
		throw CAException(3002);
	}

	if (zmq_bind(m_pSocket, endpoint)) {
		zmq_ctx_destroy(m_pZmqctx);
		m_pLog->Info("zmq_bind error:%s;", (char*) zmq_strerror(errno));
		throw CAException(3003);
	}

	m_pLog->Info("CAZeroMQ::CAZeroMQ");
}

#if 0
std::map<string, long> sendpktnum_map;
#endif

int CAZeroMQ::Send(const ZmqData &data) throw (CAException) {
	zmq_msg_t m_msg;

#if 0
	string msg = data.handle;
	std::map<string, long>::iterator ite = sendpktnum_map.find(msg);
	if (ite == sendpktnum_map.end()) {
		sendpktnum_map[msg] = 1;
	} else {
//		ite->second++;
		string str = ".ts";
		FILE* fp = fopen(((ite->first)+str).c_str(), "a+");
		fwrite(&data, sizeof(data), 1, fp);
		fclose(fp);
//		m_pLog->Debug("handle:%s,send num:%ld", msg.c_str(), ite->second);
	}
#endif

//	m_pLog->Debug(
//			"Send() to ZeroMQ!!!! uuid=[%s],mdvrid = [%s],sendFlag=[%d],channelid = [%d],streamid = [%d],statecode=[%d],filename=[%s]",
//			data.uuid, data.mdvrid, data.sendFlag, data.channelid,
//			data.streamid, data.statecode, data.filename);

	zmq_msg_init_size(&m_msg, sizeof(ZmqData));
	//有内存复制操作,可能影响性能,需要技术调整,(UDP方式)
	memcpy(zmq_msg_data(&m_msg), &data, sizeof(ZmqData));

	if (zmq_msg_send(&m_msg, m_pSocket, 0) == -1) {
		m_pLog->Info("zmq_msg_send error:%s;", (char*) zmq_strerror(errno));
		return FUNCTION_ERROR;
	}
	zmq_msg_close(&m_msg);
	return FUNCTION_SUCCESS;
}

CAZeroMQ::~CAZeroMQ() {
	zmq_close(m_pSocket);
	zmq_ctx_destroy(m_pZmqctx);
	delete this->m_pThreadMutex;
	m_pLog->Debug("CAZeroMQ::~CAZeroMQ");
}

