#include "stdafx.h"
#include "Exception.h"
#include "EduServer_RIO.h"
#include "ClientSession.h"
#include "RIOManager.h"
#include "SessionManager.h"


ClientSession::ClientSession() 
: mSocket(NULL), mConnected(false), mRefCount(0), mCircularBuffer(nullptr), mRioBufferId(NULL), mRioBufferPointer(nullptr)
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
}

ClientSession::~ClientSession()
{
	RIOManager::mRioFunctionTable.RIODeregisterBuffer(mRioBufferId);
	VirtualFreeEx(GetCurrentProcess(), mRioBufferPointer, SESSION_BUFFER_SIZE, MEM_RELEASE);
	delete mCircularBuffer;
}


bool ClientSession::RioInitialize()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	const unsigned __int64 granularity = systemInfo.dwAllocationGranularity; ///< maybe 64K

	CRASH_ASSERT(SESSION_BUFFER_SIZE % granularity == 0);

	mRioBufferPointer = reinterpret_cast<char*>(VirtualAllocEx(GetCurrentProcess(), 0, SESSION_BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (mRioBufferPointer == nullptr)
	{
		printf_s("VirtualAllocEx Error: %d\n", GetLastError());
		return false;
	}

	mCircularBuffer = new CircularBuffer(mRioBufferPointer, SESSION_BUFFER_SIZE);

	mRioBufferId = RIOManager::mRioFunctionTable.RIORegisterBuffer(mRioBufferPointer, SESSION_BUFFER_SIZE);

	if (mRioBufferId == RIO_INVALID_BUFFERID)
	{
		printf_s("RIORegisterBuffer Error: %d\n", GetLastError());
		return false;
	}

	return true;
}

bool ClientSession::OnConnect(SOCKET socket, SOCKADDR_IN* addr)
{
	FastSpinlockGuard criticalSection(mSessionLock);

	CRASH_ASSERT(LIoThreadId == MAIN_THREAD_ID);

	mSocket = socket;

	/// make socket non-blocking
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// turn off nagle
	int opt = 1 ;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();

	return PostRecv() ;
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

	ReleaseRef();

	mConnected = false ;
	mSocket = NULL;
}



bool ClientSession::PostRecv()
{
	FastSpinlockGuard criticalSection(mSessionLock);

	if (!IsConnected())
		return false;
	/*
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
		{
			DeleteIoContext(recvContext);
			printf_s("ClientSession::PostRecv Error : %d\n", GetLastError());
			return false;
		}
			
	}
	*/
	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(mSessionLock);

	//mBuffer.Commit(transferred);
}

bool ClientSession::PostSend()
{
	FastSpinlockGuard criticalSection(mSessionLock);

	if (!IsConnected())
		return false;
	/*
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
		{
			DeleteIoContext(sendContext);
			printf_s("ClientSession::PostSend Error : %d\n", GetLastError());

			return false;
		}
			
	}

*/

	return true;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(mSessionLock);

	//mBuffer.Remove(transferred);
}


void ClientSession::AddRef()
{
	CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
}

void ClientSession::ReleaseRef()
{
	long ret = InterlockedDecrement(&mRefCount);
	CRASH_ASSERT(ret >= 0);
	
	if (ret == 0)
	{
		GSessionManager->DeleteClientSession(this);
	}
}


