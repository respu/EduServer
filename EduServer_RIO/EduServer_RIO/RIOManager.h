#pragma once

class ClientSession;
class RioBufferManager;

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

	

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

//	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
//	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
//	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);

private:


	SOCKET	mListenSocket;
	
	static RIO_EXTENSION_FUNCTION_TABLE mRioFunctionTable ;

	friend class RioBufferManager;
};


extern RIOManager* GRioManager;