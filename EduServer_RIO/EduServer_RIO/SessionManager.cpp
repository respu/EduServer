#include "stdafx.h"
#include "FastSpinlock.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

bool SessionManager::PrepareSessionPool(int maxSession)
{

	return true;
}

ClientSession* SessionManager::CreateClientSession()
{
	ClientSession* client = new ClientSession();
	client->AddRef();

// 	mLock.EnterLock();
// 	{
// 		mClientList.insert(ClientList::value_type(sock, client));
// 	}
// 	mLock.LeaveLock();

	return client;
}


void SessionManager::DeleteClientSession(ClientSession* client)
{
// 	mLock.EnterLock();
// 	{
// 		mClientList.erase(client->mSocket);
// 	}
// 	mLock.LeaveLock();

	delete client;
}