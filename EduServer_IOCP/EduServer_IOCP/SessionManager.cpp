#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "FastSpinlock.h"

SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);

	mLock.EnterLock();
	mClientList.insert(ClientList::value_type(sock, client));
	mLock.LeaveLock();

	return client;
}

