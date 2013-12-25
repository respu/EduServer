#pragma once

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	bool StartAcceptLoop();

	HANDLE GetComletionPort()	{ return mCompletionPort; }
	int	GetIoThreadCount()		{ return mIoThreadCount;  }


	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

private:

	HANDLE	mCompletionPort;
	int		mIoThreadCount;

	SOCKET	mListenSocket;


};

extern IocpManager* GIocpManager;