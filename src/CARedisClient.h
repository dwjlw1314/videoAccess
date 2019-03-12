/*
 * CARedisClient.h
 *
 *  Created on: Feb 12, 2015
 *      Author: diwenjie
 *  Compile time is added in the property <hiredis>
 */

#ifndef CAREDISCLIENT_H_
#define CAREDISCLIENT_H_

#include <hiredis.h>
#include <iostream>
#include <vector>
#include "CAPublicStruct.h"
#include "CALog.h"
using std::vector;
using std::string;

#define KEYPRE "VAS_ACCESS:"
#define FIELD "VAADDRESS"

class CARedisClient
{
public:
	static bool m_InitStatus;
	CARedisClient(const char* ip, int port);
	virtual ~CARedisClient();

	string& GetErrorMsg();

/*
 * Other write redis method
 */
	int ListAdd(const char* listName, const char* val);
	int ListDelete(const char* listName, const char* val);
	int ListClear(const char* listName);
	int ListLength(const char* listName);
	vector<string> ListGetValues(const char* listName);

/*
 * 视频接入需要到写reids方法
 * key: CHANNEL_mdvrid:channelid
 * field: VAADDRESS 为视频接入地址;
 * value: <VAADDRESS,ipValue:portValue>
 * if hash has field of fieldName ,return 1;
 * if hash doesn't have field of fieldName, or hash is not exist, return 0.
 */
	int HashExists(const char* hashName,const char* fieldName);
	int HashAdd(const char* hashName, const char* fieldName, const char* val);
	int HashDelete(const char* hashName, const char* fieldName);
    const char* HashGetValue(const char* hashName, const char* fieldName);

    /*
     * dwj add, solve reconnect redis err;
     */
    redisContext* ReturnRedisContext();

/*
* return the count of field in hash.
* if hash is not exist, return 0.
*/
    int HashLength(const char* hashName);

private:
	redisContext *m_pRC;
	CALog *m_pLog;
	string m_errInfo;
};

#endif /* CAREDISCLIENT_H_ */
