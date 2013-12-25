#pragma once
#include <map>
#include <WinSock2.h>

class ClientSession;

class SessionManager
{
public:
	SessionManager()	{}

	ClientSession* CreateClientSession(SOCKET sock);


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

};

extern SessionManager* GSessionManager;
