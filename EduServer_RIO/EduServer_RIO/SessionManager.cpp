#include "stdafx.h"
#include "FastSpinlock.h"
#include "ClientSession.h"
#include "SessionManager.h"


SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);
	client->AddRef();

	mLock.EnterLock();
	{
		mClientList.insert(ClientList::value_type(sock, client));
	}
	mLock.LeaveLock();

	return client;
}


void SessionManager::DeleteClientSession(ClientSession* client)
{
	mLock.EnterLock();
	{
		mClientList.erase(client->mSocket);
	}
	mLock.LeaveLock();

	delete client;
}