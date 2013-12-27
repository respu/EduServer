#pragma once
#include <map>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager()	{}

	ClientSession* CreateClientSession(SOCKET sock);

	void DeleteClientSession(ClientSession* client);


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

	FastSpinlock mLock;

};

extern SessionManager* GSessionManager;
