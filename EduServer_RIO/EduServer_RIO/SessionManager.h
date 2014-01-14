#pragma once
#include <map>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager() : mCurrentConnectionCount(0)	{}

	bool PrepareSessionPool(int maxSession);

	ClientSession* CreateClientSession();

	void DeleteClientSession(ClientSession* client);

	int IncreaseConnectionCount() { return InterlockedIncrement(&mCurrentConnectionCount); }
	int DecreaseConnectionCount() { return InterlockedDecrement(&mCurrentConnectionCount); }


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

	FastSpinlock mLock;

	volatile long mCurrentConnectionCount;
};

extern SessionManager* GSessionManager;
