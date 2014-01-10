#include "stdafx.h"
#include "EduServer_RIO.h"
#include "RIOManager.h"
#include "ClientSession.h"
#include "SessionManager.h"


RIOManager* GRioManager = nullptr;

RIOManager::RIOManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{
}


RIOManager::~RIOManager()
{
}


bool RIOManager::Initialize()
{
	/// set num of I/O threads
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	mIoThreadCount = si.dwNumberOfProcessors;

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;


	/// Create I/O Completion Port
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (mCompletionPort == NULL)
		return false;

	/// create TCP socket
	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET)
		return false;

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(LISTEN_PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		return false;

	/// RIO 함수 테이블 가져오기
	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD dwBytes = 0;

	if ( WSAIoctl(mListenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID), (void**)&mRioFunctionTable, sizeof(mRioFunctionTable), &dwBytes, NULL, NULL) )
		return false;
	
	return true;
}


bool RIOManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (LPVOID)(i + 1), 0, (unsigned int*)&dwThreadId);
		if (hThread == NULL)
			return false;
	}

	return true;
}


bool RIOManager::StartAcceptLoop()
{
	/// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
		return false;


	/// accept loop
	while (true)
	{
		SOCKET acceptedSock = accept(mListenSocket, NULL, NULL);
		if (acceptedSock == INVALID_SOCKET)
		{
			printf_s("accept: invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		/// new client session (should not be under any session locks)
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		/// connection establishing and then issuing recv
		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect(DR_ONCONNECT_ERROR);
		}
	}

	return true;
}

void RIOManager::Finalize()
{
	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}

unsigned int WINAPI RIOManager::IoWorkerThread(LPVOID lpParam)
{
	LIoThreadId = reinterpret_cast<int>(lpParam);
	HANDLE hComletionPort = GRioManager->GetComletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ClientSession* asCompletionKey = nullptr;

		int ret = GetQueuedCompletionStatus(hComletionPort, &dwTransferred, (PULONG_PTR)&asCompletionKey, (LPOVERLAPPED*)&context, INFINITE);

		if (ret == 0 || dwTransferred == 0)
		{
			int gle = GetLastError();

			/// check time out first 
			if (gle == WAIT_TIMEOUT)
				continue;

			if (ret && context->mIoType == IO_RECV_ZERO)
			{
				; ///< do nothing... 
			}
			else
			{
				CRASH_ASSERT(nullptr != asCompletionKey);

				/// In most cases in here: ERROR_NETNAME_DELETED(64)

				asCompletionKey->Disconnect(DR_COMPLETION_ERROR);

				DeleteIoContext(context);

				continue;
			}
		}


		bool completionOk = false;
		switch (context->mIoType)
		{
		case IO_SEND:
			//completionOk = SendCompletion(asCompletionKey, static_cast<OverlappedSendContext*>(context), dwTransferred);
			break;

		case IO_RECV_ZERO:
			//completionOk = PreReceiveCompletion(asCompletionKey, static_cast<OverlappedPreRecvContext*>(context), dwTransferred);
			break;

		case IO_RECV:
			//completionOk = ReceiveCompletion(asCompletionKey, static_cast<OverlappedRecvContext*>(context), dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			CRASH_ASSERT(false);
			break;
		}

		if (!completionOk)
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_IO_REQUEST_ERROR);
		}

		DeleteIoContext(context);
	}

	return 0;
}

/*
bool RIOManager::PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred)
{
	// real receive...
	return client->PostRecv();
}

bool RIOManager::ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred)
{
	client->RecvCompletion(dwTransferred);

	/// echo back
	if (false == client->PostSend())
		return false;

	// zero receive
	return client->PreRecv();
}

bool RIOManager::SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred)
{
	client->SendCompletion(dwTransferred);

	if (context->mWsaBuf.len != dwTransferred)
	{
		printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred);
		return false;
	}

	return true;
}
*/