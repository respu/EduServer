#include "stdafx.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	assert(LThreadType == THREAD_MAIN_ACCEPT);

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN)) ;

	/// 소켓을 넌블러킹으로 바꾸고
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// nagle 알고리즘 끄기
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

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port)) ;
	
	mConnected = true ;

	return PostRecv() ;
}

bool ClientSession::PostRecv()
{
	if ( !IsConnected() )
		return false ;

	//TODO: pooling...
	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV) ;

	DWORD recvbytes = 0 ;
	DWORD flags = 0 ;
	WSABUF buf ;
	buf.len = BUFSIZE;
	buf.buf = recvContext->mBuffer ;


	/// 비동기 입출력 시작
	if ( SOCKET_ERROR == WSARecv(mSocket, &buf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
			return false ;
	}

	return true ;
}

void ClientSession::Disconnect()
{
	if ( !IsConnected() )
		return ;

	printf_s("[DEBUG] Client Disconnected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port)) ;

	::shutdown(mSocket, SD_BOTH) ;
	::closesocket(mSocket) ;

	mConnected = false ;
}

