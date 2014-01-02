#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	FastSpinlockGuard criticalSection(mSessionLock);

	CRASH_ASSERT(LThreadType == THREAD_MAIN_ACCEPT);

	/// make socket non-blocking
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// turn off nagle
	int opt = 1 ;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) )
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError()) ;
		return false;
	}
	
	HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0);
	if (handle != GIocpManager->GetComletionPort())
	{
		printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();

	return PreRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	FastSpinlockGuard criticalSection(mSessionLock);

	if ( !IsConnected() )
		return ;
	
	LINGER lingerOption ;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)) )
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}

	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
	
	GSessionManager->DecreaseConnectionCount();

	closesocket(mSocket) ;

	mConnected = false ;
}

bool ClientSession::PreRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = 0;
	recvContext->mWsaBuf.buf = nullptr;

	/// start async recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
			return false;
	}

	return true;
}

bool ClientSession::PostRecv()
{
	FastSpinlockGuard criticalSection(mSessionLock);

	if (!IsConnected())
		return false;

	if (0 == mBuffer.GetFreeSpaceSize())
		return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = (ULONG)mBuffer.GetFreeSpaceSize();
	recvContext->mWsaBuf.buf = mBuffer.GetBuffer();
	

	/// start real recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
			return false;
	}

	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(mSessionLock);

	mBuffer.Commit(transferred);
}

bool ClientSession::PostSend()
{
	FastSpinlockGuard criticalSection(mSessionLock);

	if (!IsConnected())
		return false;

	if ( 0 == mBuffer.GetContiguiousBytes() )
		return true;

	OverlappedSendContext* sendContext = new OverlappedSendContext(this);

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG) mBuffer.GetContiguiousBytes(); 
	sendContext->mWsaBuf.buf = mBuffer.GetBufferStart();

	/// start async send
	if (SOCKET_ERROR == WSASend(mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
			return false;
	}

	return true;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(mSessionLock);

	mBuffer.Remove(transferred);
}