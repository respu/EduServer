#include "stdafx.h"
#include "EduServer_RIO.h"
#include "RioBufferManager.h"
#include "RIOManager.h"
#include "ClientSession.h"
#include "SessionManager.h"


RIO_EXTENSION_FUNCTION_TABLE RIOManager::mRioFunctionTable = { 0, };

RIOManager* GRioManager = nullptr;

RIOManager::RIOManager() : mListenSocket(NULL)
{
}


RIOManager::~RIOManager()
{
}


bool RIOManager::Initialize()
{
	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
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
	for (int i = 0; i < MAX_RIO_THREAD; ++i)
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
	/// winsock finalizing
	WSACleanup();

}

unsigned int WINAPI RIOManager::IoWorkerThread(LPVOID lpParam)
{
	LIoThreadId = reinterpret_cast<int>(lpParam);
	LRioBufferManager = new RioBufferManager(LIoThreadId);

	if (false == LRioBufferManager->PrepareRioBuffers())
		return -1;

	RIO_CQ rioCompletionQueue = mRioFunctionTable.RIOCreateCompletionQueue(MAX_CQ_SIZE_PER_RIO_THREAD, 0);
	if (rioCompletionQueue == RIO_INVALID_CQ)
		return -1;

	RIORESULT results[MAX_RIO_RESULT];

	while (true)
	{
		memset(results, 0, sizeof(results));
		
		ULONG numResults = mRioFunctionTable.RIODequeueCompletion(rioCompletionQueue, results, MAX_RIO_RESULT);
		
		if (0 == numResults)
		{
			Sleep(1);
		}
		else if (RIO_CORRUPT_CQ == numResults)
		{
			printf_s("RIO CORRUPT CQ \n");
			CRASH_ASSERT(false);
		}


		for (ULONG i = 0; i < numResults; ++i)
		{
			RioIoContext* context = reinterpret_cast<RioIoContext*>(results[i].RequestContext);
			ClientSession* client = context->mSessionObject;
			ULONG transferred = results[i].BytesTransferred;

			CRASH_ASSERT(context && client);

			bool welldone = true;
			if (transferred == 0)
			{
				welldone = false;
			}
			else if (IO_RECV == context->mIoType)
			{
				client->RecvCompletion(transferred);

				/// echo back
				if (false == client->PostSend())
					welldone = false;

				if (welldone && false == client->PostRecv())
					welldone = false;
		
			}
			else if (IO_SEND == context->mIoType)
			{
			
				client->SendCompletion(transferred);

				if (context->mRioBuf.Length != transferred)
				{
					printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->mRioBuf.Length, transferred);
					welldone = false ;
				}
			}
			else
			{
				printf_s("Unknown I/O Type: %d\n", context->mIoType);
				CRASH_ASSERT(false);
			}

			if (!welldone)
			{
				client->Disconnect(DR_COMPLETION_ERROR);
			}

			LRioBufferManager->PushRioIoContext(context);
		}

 	}

	delete LRioBufferManager;
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