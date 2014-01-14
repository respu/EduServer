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


	
	static RIO_EXTENSION_FUNCTION_TABLE mRioFunctionTable;

private:

	

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);


	SOCKET	mListenSocket;
	
};


extern RIOManager* GRioManager;