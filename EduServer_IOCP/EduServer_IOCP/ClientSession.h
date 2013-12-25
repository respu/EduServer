#pragma once

#define BUFSIZE	4096

class ClientSession ;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_ACCEPT
} ;

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
	void	Disconnect();
	
	


private:
	bool			mConnected ;
	SOCKET			mSocket ;

	SOCKADDR_IN		mClientAddr ;
		
	

} ;




