#include "stdafx.h"
#include "ClientSession.h"


bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN)) ;

	/// 소켓을 넌블러킹으로 바꾸고
	u_long arg = 1 ;
	::ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// nagle 알고리즘 끄기
	int opt = 1 ;
	::setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	opt = 0;
	if (SOCKET_ERROR == ::setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) )
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError()) ;
		return false;
	}
	
	// iocp 핸들, clientsession을 컴플리션키로...
	//HANDLE handle = CreateIoCompletionPort(mSocket, m_hIOCP, CompletionKey, 0);

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

/*
void ClientSession::OnRead(size_t len)
{
	mRecvBuffer.Commit(len) ;

	/// 패킷 파싱하고 처리
	while ( true )
	{
		/// 패킷 헤더 크기 만큼 읽어와보기
		PacketHeader header ;
		if ( false == mRecvBuffer.Peek((char*)&header, sizeof(PacketHeader)) )
			return ;

		/// 패킷 완성이 되는가? 
		if ( mRecvBuffer.GetStoredSize() < (header.mSize - sizeof(PacketHeader)) )
			return ;

		/// 패킷 핸들링
		switch ( header.mType )
		{
		case PKT_CS_LOGIN:
			{
				LoginRequest inPacket ;
				mRecvBuffer.Read((char*)&inPacket, header.mSize) ;

				/// 로그인은 DB 작업을 거쳐야 하기 때문에 DB 작업 요청한다.
				LoadPlayerDataContext* newDbJob = new LoadPlayerDataContext(mSocket, inPacket.mPlayerId) ;
				GDatabaseJobManager->PushDatabaseJobRequest(newDbJob) ;
			
			}
			break ;

		case PKT_CS_CHAT:
			{
				ChatBroadcastRequest inPacket ;
				mRecvBuffer.Read((char*)&inPacket, header.mSize) ;
				
				ChatBroadcastResult outPacket ;
				outPacket.mPlayerId = inPacket.mPlayerId ;
				strcpy_s(outPacket.mName, mPlayerName) ;
				strcpy_s(outPacket.mChat, inPacket.mChat) ;
		
				/// 채팅은 바로 방송 하면 끝
				if ( !Broadcast(&outPacket) )
					return ;
 
			}
			break ;

		default:
			{
				/// 여기 들어오면 이상한 패킷 보낸거다.
				Disconnect() ;
				return ;
			}
			break ;
		}
	}
}

bool ClientSession::Send(PacketHeader* pkt)
{
	if ( !IsConnected() )
		return false ;

	/// 버퍼 용량 부족인 경우는 끊어버림
	if ( false == mSendBuffer.Write((char*)pkt, pkt->mSize) )
	{
		Disconnect() ;
		return false ;
	}

	/// 보낼 데이터가 있는지 검사
	if ( mSendBuffer.GetContiguiousBytes() == 0 )
	{
		/// 방금전에 write 했는데, 데이터가 없다면 뭔가 잘못된 것
		assert(false) ;
		Disconnect() ;
		return false ;
	}
		
	DWORD sendbytes = 0 ;
	DWORD flags = 0 ;

	WSABUF buf ;
	buf.len = (ULONG)mSendBuffer.GetContiguiousBytes() ;
	buf.buf = (char*)mSendBuffer.GetBufferStart() ;
	
	memset(&mOverlappedSend, 0, sizeof(OverlappedIO)) ;
	mOverlappedSend.mObject = this ;

	// 비동기 입출력 시작
	if ( SOCKET_ERROR == WSASend(mSocket, &buf, 1, &sendbytes, flags, &mOverlappedSend, SendCompletion) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
			return false ;
	}

	IncOverlappedRequest() ;
	return true ;
}

void ClientSession::OnWriteComplete(size_t len)
{
	/// 보내기 완료한 데이터는 버퍼에서 제거
	mSendBuffer.Remove(len) ;

	/// 얼래? 덜 보낸 경우도 있나? (커널의 send queue가 꽉찼거나, Send Completion이전에 또 send 한 경우?)
	if ( mSendBuffer.GetContiguiousBytes() > 0 )
	{
		assert(false) ;
	}

}

bool ClientSession::Broadcast(PacketHeader* pkt)
{
	if ( !Send(pkt) )
		return false ;

	if ( !IsConnected() )
		return false ;

	GClientManager->BroadcastPacket(this, pkt) ;

	return true ;
}

void ClientSession::OnTick()
{
	/// 클라별로 주기적으로 해야될 일은 여기에

	/// 특정 주기로 DB에 위치 저장
	if ( ++mDbUpdateCount == PLAYER_DB_UPDATE_CYCLE )
	{
		mDbUpdateCount = 0 ;
		UpdatePlayerDataContext* updatePlayer = new UpdatePlayerDataContext(mSocket, mPlayerId) ;

		updatePlayer->mPosX = mPosX ;
		updatePlayer->mPosY = mPosY ;
		updatePlayer->mPosZ = mPosZ ;
		strcpy_s(updatePlayer->mComment, "updated_test") ; ///< 일단은 테스트를 위해
		GDatabaseJobManager->PushDatabaseJobRequest(updatePlayer) ;
	}
	
}

void ClientSession::DatabaseJobDone(DatabaseJobContext* result)
{
	CRASH_ASSERT( mSocket == result->mSockKey ) ;
	

	const type_info& typeInfo = typeid(*result) ;

	if ( typeInfo == typeid(LoadPlayerDataContext) )
	{
		LoadPlayerDataContext* login = dynamic_cast<LoadPlayerDataContext*>(result) ;

		LoginDone(login->mPlayerId, login->mPosX, login->mPosY, login->mPosZ, login->mPlayerName) ;
	
	}
	else if ( typeInfo == typeid(UpdatePlayerDataContext) )
	{
		UpdateDone() ;
	}
	else
	{
		CRASH_ASSERT(false) ;
	}

}

void ClientSession::UpdateDone()
{
	/// 콘텐츠를 넣기 전까지는 딱히 해줄 것이 없다. 단지 테스트를 위해서..
	printf("DEBUG: Player[%d] Update Done\n", mPlayerId) ;
}



void ClientSession::LoginDone(int pid, double x, double y, double z, const char* name)
{
	LoginResult outPacket ;

	outPacket.mPlayerId = mPlayerId = pid ;
	outPacket.mPosX = mPosX = x ;
	outPacket.mPosY = mPosY = y ;
	outPacket.mPosZ = mPosZ = z ;
	strcpy_s(mPlayerName, name) ;
	strcpy_s(outPacket.mName, name) ;

	Send(&outPacket) ;

	mLogon = true ;
}



///////////////////////////////////////////////////////////

void CALLBACK RecvCompletion(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	ClientSession* fromClient = static_cast<OverlappedIO*>(lpOverlapped)->mObject ;
	
	fromClient->DecOverlappedRequest() ;

	if ( !fromClient->IsConnected() )
		return ;

	/// 에러 발생시 해당 세션 종료
	if ( dwError || cbTransferred == 0 )
	{
		fromClient->Disconnect() ;
		return ;
	}

	/// 받은 데이터 처리
	fromClient->OnRead(cbTransferred) ;

	/// 다시 받기
	if ( false == fromClient->PostRecv() )
	{
		fromClient->Disconnect() ;
		return ;
	}
}


void CALLBACK SendCompletion(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	ClientSession* fromClient = static_cast<OverlappedIO*>(lpOverlapped)->mObject ;

	fromClient->DecOverlappedRequest() ;

	if ( !fromClient->IsConnected() )
		return ;

	/// 에러 발생시 해당 세션 종료
	if ( dwError || cbTransferred == 0 )
	{
		fromClient->Disconnect() ;
		return ;
	}

	fromClient->OnWriteComplete(cbTransferred) ;

}

*/