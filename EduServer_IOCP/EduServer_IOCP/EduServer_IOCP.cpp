// EduServer_IOCP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"

//#include "../Share/PacketType.h"

int NUM_OF_IO_THREADS = 4;


int _tmain(int argc, _TCHAR* argv[])
{
	/// for dump on crash
	SetUnhandledExceptionFilter(ExceptionFilter);

	/// Global Managers
	GSessionManager = new SessionManager;


	/// set num of I/O threads
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	NUM_OF_IO_THREADS = si.dwNumberOfProcessors;

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	/// Create I/O Completion Port
	HANDLE hCp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hCp == NULL)
		return -1;

	/// create TCP socket
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return -1;

	int opt = 1;
	::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(LISTEN_PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int ret = bind(listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR)
		return -1;

	/// listen
	ret = listen(listenSocket, SOMAXCONN);
	if (ret == SOCKET_ERROR)
		return -1;

	/// I/O Thread
	for (int i = 0; i < NUM_OF_IO_THREADS; ++i)
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoThread, hCp, 0, (unsigned int*)&dwThreadId);
		if (hThread == NULL)
			return -1;
	}
	

	/// accept loop
	while (true)
	{
		SOCKET acceptedSock = accept(listenSocket, NULL, NULL);
		if (acceptedSock == INVALID_SOCKET)
		{
			printf_s("accept: invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		/// 소켓 정보 구조체 할당과 초기화
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		/// 클라 접속 처리
		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect();
		}

		
	}


	/// winsock finalizing
	WSACleanup();


	delete GSessionManager;

	return 0;
}

unsigned int WINAPI IoThread(LPVOID lpParam)
{
	HANDLE hCp = (HANDLE)lpParam;

	while (true)
	{

	}

	return 0;
}