#pragma once
#include "FastSpinlock.h"

#define BUFSIZE	4096

class ClientSession ;
class SessionManager;
enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_ACCEPT
} ;

enum DisconnectReason
{
	DR_NONE,
	DR_RECV_ZERO,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(ClientSession* owner, IOType ioType) : mSessionObject(owner), mIoType(ioType)
	{
		memset(&mOverlapped, 0, sizeof(OVERLAPPED));
		memset(mBuffer, 0, BUFSIZE);
	}

	OVERLAPPED		mOverlapped ;
	ClientSession*	mSessionObject ;
	IOType			mIoType ;
	char			mBuffer[BUFSIZE];
} ;


class ClientSession
{
public:
	ClientSession(SOCKET sock) 
		: mSocket(sock), mConnected(false)
	{
		memset(&mClientAddr, 0, sizeof(SOCKADDR_IN)) ;
	}

	~ClientSession() {}

	bool	OnConnect(SOCKADDR_IN* addr);
	bool	IsConnected() const { return mConnected; }

	bool	PostRecv();
	void	Disconnect(DisconnectReason dr);
	

private:
	bool			mConnected ;
	SOCKET			mSocket ;

	SOCKADDR_IN		mClientAddr ;
		
	FastSpinlock	mLock;

	friend class SessionManager;
} ;




