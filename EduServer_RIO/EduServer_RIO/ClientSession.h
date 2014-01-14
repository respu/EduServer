#pragma once
#include "ObjectPool.h"
#include "CircularBuffer.h"
#include "RioContext.h"




class SessionManager;

class ClientSession : public ObjectPool<ClientSession>
{
public:
	ClientSession(SOCKET sock) 
		: mSocket(sock), mConnected(false), mRefCount(0)
	{
		memset(&mClientAddr, 0, sizeof(SOCKADDR_IN)) ;
	}

	~ClientSession() {}

	bool	OnConnect(SOCKADDR_IN* addr);
	bool	IsConnected() const { return mConnected; }


	bool	PostRecv();
	void	RecvCompletion(DWORD transferred);

	bool	PostSend();
	void	SendCompletion(DWORD transferred);
	
	void	Disconnect(DisconnectReason dr);
	
	void	AddRef();
	void	ReleaseRef();

private:
	bool			mConnected ;
	SOCKET			mSocket ;

	SOCKADDR_IN		mClientAddr ;
		
	FastSpinlock	mSessionLock;


	volatile long	mRefCount;

	friend class SessionManager;
} ;



