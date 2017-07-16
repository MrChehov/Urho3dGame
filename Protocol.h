
#pragma once

#pragma pack (push, 1)

#define MSG_IDCLIENT  0x10000001
class CMsgIdClient
{
public:
	unsigned long nIdClient;

};

#define MSG_CLICKITEM  0x10000002
class CMsgClickItem
{
public:
	unsigned long nIdClient;
	float         x;
	float         y;
};

#define MSG_UPDATEITEM 0x10000003
class CMsgUpdateItem
{
public:
	unsigned long nIdClient;
	float         x;
	float         y;
};
#pragma pack (pop)