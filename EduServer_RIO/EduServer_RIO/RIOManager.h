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

	HANDLE GetComletionPort()	{ return mCompletionPort; }
	int	GetIoThreadCount()		{ return mIoThreadCount; }


private:

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

//	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
//	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
//	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);

private:

	HANDLE	mCompletionPort;
	int		mIoThreadCount;

	SOCKET	mListenSocket;

	RIO_EXTENSION_FUNCTION_TABLE mRioFunctionTable ;
	RIO_CQ mRioCompletionQueue;

};

extern RIOManager* GRioManager;