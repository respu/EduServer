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

	const RIO_CQ& GetCompletionQueue(int threadId) { return mRioCompletionQueue[threadId]; }
	
	static RIO_EXTENSION_FUNCTION_TABLE mRioFunctionTable;

private:

	

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);
	static RIO_CQ mRioCompletionQueue[MAX_RIO_THREAD + 1];

	SOCKET	mListenSocket;

};


extern RIOManager* GRioManager;

#define RIO	RIOManager::mRioFunctionTable
