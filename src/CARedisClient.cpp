/*
 * CARedisClient.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: diwenjie
 */

#include "CARedisClient.h"

bool CARedisClient::m_InitStatus = false;

CARedisClient::CARedisClient(const char* ip, int port)
	:m_errInfo("")
{
	m_pLog = CALog::GetInstance(g_data.ZlogRmCname);
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	m_pRC = redisConnectWithTimeout(ip, port, timeout);
	if (m_pRC == NULL || m_pRC->err)
	{
		if (m_pRC)
		{
			m_errInfo = "Connection error:";
			m_errInfo += m_pRC->errstr;
			redisFree(m_pRC);
			CARedisClient::m_InitStatus = false;
		}
		else
		{
			m_errInfo = "Connection error: can't allocate redis context.";
		}

	}
	else
	{
		CARedisClient::m_InitStatus = true;
	}
}

CARedisClient::~CARedisClient()
{
	// TODO Auto-generated destructor stub
	if(m_pRC)
	{
		redisFree(m_pRC);
	}
}

redisContext* CARedisClient::ReturnRedisContext()
{
	return m_pRC;
}

string& CARedisClient::GetErrorMsg()
{
	return m_errInfo;
}

int CARedisClient::ListAdd(const char* listName, const char* val)
{
	//delete
	ListDelete(listName,val);
    //add
	redisReply * reply = (redisReply *)redisCommand(m_pRC,"LPUSH %s %s",listName, val);
    freeReplyObject(reply);
	return 0;
}
int CARedisClient::ListDelete(const char* listName, const char* val)
{
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"LREM %s 0 %s",listName, val);
    freeReplyObject(reply);
	return 0;
}
int CARedisClient::ListLength(const char* listName)
{
	int ret = 0;
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"LLEN %s",listName);
	if(reply && reply->type == REDIS_REPLY_INTEGER)
	{
		ret = reply->integer;
	}
    freeReplyObject(reply);
	return ret;
}
int CARedisClient::ListClear(const char* listName)
{
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"LTRIM %s 0 0",listName);
	reply = (redisReply *) redisCommand(m_pRC,"LPOP %s",listName);
    freeReplyObject(reply);
	return 0;
}
vector< string > CARedisClient::ListGetValues(const char* listName)
{
	unsigned int j=0;
	vector<string> vecList;
	redisReply * reply = (redisReply *)redisCommand(m_pRC,"LRANGE %s 0 -1",listName);
    if (reply && reply->type == REDIS_REPLY_ARRAY)
    {
    	vecList.clear();
        for (;j < reply->elements; j++)
        {
        	string value;
        	std::cout<<j<<":"<<reply->element[j]->str<<std::endl;
        	value = reply->element[j]->str;
        	vecList.push_back(value);
        }
    }
    freeReplyObject(reply);
    return vecList;

}
//if hash has field of fieldName ,return 1;
//if hash doesn't have field of fieldName, or hash is not exist, return 0.
int CARedisClient::HashExists(const char* hashName,const char* fieldName)
{
	int ret = 0;
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"HEXISTS %s %s",hashName,fieldName);
	if(reply && reply->type == REDIS_REPLY_INTEGER)
	{
		ret = reply->integer;
	}
    freeReplyObject(reply);
    return ret;
}
int CARedisClient::HashAdd(const char* hashName, const char* fieldName, const char* val)
{
	//delete
//	HashDelete(hashName,fieldName);
	//add
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"HSET %s %s %s",hashName,fieldName,val);
    freeReplyObject(reply);
    m_pLog->Info("HashAdd key:%s,value:%s",hashName,val);
	return 0;
}
int CARedisClient::HashDelete(const char* hashName, const char* fieldName)
{
	m_pLog->Info("HashDelete key:%s",hashName);
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"HDEL %s %s",hashName,fieldName);
    freeReplyObject(reply);
	return 0;
}
int CARedisClient::HashLength(const char* hashName)
{
	int ret = 0;
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"HLEN %s",hashName);
	if(reply && reply->type == REDIS_REPLY_INTEGER)
	{
		ret = reply->integer;
	}
    freeReplyObject(reply);
	return ret;
}
const char* CARedisClient::HashGetValue(const char* hashName, const char* fieldName)
{
	string val;
	redisReply * reply =(redisReply *) redisCommand(m_pRC,"HGET %s %s",hashName,fieldName);
	if(reply && reply->type == REDIS_REPLY_STRING)
	{
		val = reply->str;
	}
    freeReplyObject(reply);
    return val.c_str();
}

