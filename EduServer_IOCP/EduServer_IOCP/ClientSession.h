#pragma once
#include "ObjectPool.h"
#include "CircularBuffer.h"

#define BUFSIZE	65536

class ClientSession ;
class SessionManager;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT
} ;

enum DisconnectReason
{
	DR_NONE,
	DR_RECV_ZERO,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(const ClientSession* owner, IOType ioType) : mSessionObject(owner), mIoType(ioType)
	{
		memset(&mOverlapped, 0, sizeof(OVERLAPPED));
		memset(&mWsaBuf, 0, sizeof(WSABUF));
	}

	OVERLAPPED				mOverlapped ;
	const ClientSession*	mSessionObject ;
	IOType					mIoType ;
	WSABUF					mWsaBuf;
	
} ;

struct OverlappedSendContext : public OverlappedIOContext, public ObjectPool<OverlappedSendContext>
{
	OverlappedSendContext(const ClientSession* owner) : OverlappedIOContext(owner, IO_SEND)
	{
	}
};

struct OverlappedRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedRecvContext>
{
	OverlappedRecvContext(const ClientSession* owner) : OverlappedIOContext(owner, IO_RECV)
	{
	}
};

struct OverlappedPreRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedPreRecvContext>
{
	OverlappedPreRecvContext(const ClientSession* owner) : OverlappedIOContext(owner, IO_RECV_ZERO)
	{
	}
};




class ClientSession : public ObjectPool<ClientSession>
{
public:
	ClientSession(SOCKET sock) 
		: mSocket(sock), mConnected(false), mBuffer(BUFSIZE)
	{
		memset(&mClientAddr, 0, sizeof(SOCKADDR_IN)) ;
	}

	~ClientSession() {}

	bool	OnConnect(SOCKADDR_IN* addr);
	bool	IsConnected() const { return mConnected; }

	bool	PreRecv() const ; ///< zero byte recv

	bool	PostRecv();
	void	RecvCompletion(DWORD transferred);

	bool	PostSend();
	void	SendCompletion(DWORD transferred);
	
	void	Disconnect(DisconnectReason dr);
	

private:
	bool			mConnected ;
	SOCKET			mSocket ;

	SOCKADDR_IN		mClientAddr ;
		
	FastSpinlock	mSessionLock;

	CircularBuffer	mBuffer;

	friend class SessionManager;
} ;




