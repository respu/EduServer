#pragma once

class ClientSession;

class RIOManager
{
public:
	RIOManager();
	~RIOManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	bool StartAcceptLoop();


	

private:

	enum 
	{
		MAX_RIO_THREAD = 1,
		MAX_RIO_RESULT = 1024,
		MAX_SEND_RQ_SIZE_PER_SOCKET = 32,
		MAX_RECV_RQ_SIZE_PER_SOCKET = 32,
		MAX_CLIENT_PER_RIO_THREAD = 2000,
		MAX_CQ_SIZE_PER_RIO_THREAD = (MAX_SEND_RQ_SIZE_PER_SOCKET + MAX_RECV_RQ_SIZE_PER_SOCKET) * MAX_CLIENT_PER_RIO_THREAD,

	};

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

//	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
//	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
//	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);

private:


	SOCKET	mListenSocket;
	
	static RIO_EXTENSION_FUNCTION_TABLE mRioFunctionTable ;
};


extern RIOManager* GRioManager;