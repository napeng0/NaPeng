#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include<stdio.h>
#include<errno.h>
#include"NetWork.h"
#include"EventManager\EventManager.h"
#include"EVentManager\Events.h"
#include"Utilities\String.h"


#pragma comment(lib, "Ws2_32")
#define EXIT_ASSERT ASSERT(0)

const char* BinaryPacket::s_Type = "BinaryPacket";
const char* TextPacket::s_Type = "TextPacket";

BaseSocketManager* g_pSocketManager = NULL;


TextPacket::TextPacket(const char* text)
	:BinaryPacket((u_long)(strlen(text) + 2))
{
	MemCpy(text, strlen(text), 0);
	MemCpy("\r\n", 2, 2);
	*(u_long*)m_pData = 0;
}


NetSocket::NetSocket()
{
	m_Sock = INVALID_SOCKET;
	m_DeleteFlag = 0;
	m_SendOfs = 0;
	m_TimeOut = 0;

	m_RecvOfs = m_RecvBegin = 0;
	m_Internal = 0;
	m_IsBinaryProtocol = 1;
}


NetSocket::NetSocket(SOCKET newSock, unsigned int hostIP)
{
	m_DeleteFlag = 0;
	m_SendOfs = 0;
	m_TimeOut = 0;

	m_IsBinaryProtocol = 1;

	m_RecvOfs = m_RecvBegin = 0;
	m_Internal = 0;

	m_TimeCreated = timeGetTime();

	m_Sock = newSock;
	m_IpAddr = hostIP;

	m_Internal = g_pSocketManager->IsInternal(m_IpAddr);

	setsockopt(m_Sock, SOL_SOCKET, SO_DONTLINGER, NULL, 0);
}


NetSocket::~NetSocket()
{
	if (m_Sock != INVALID_SOCKET)
	{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
	}
}


bool NetSocket::Connect(unsigned int ip, unsigned int port, bool forceCoalesce)
{
	struct sockaddr_in sockAddr;
	int x = 1;

	if ((m_Sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return false;

	if (!forceCoalesce)
	{
		setsockopt(m_Sock, IPPROTO_TCP, TCP_NODELAY, (char *)&x, sizeof(x));
	}

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl(ip);
	sockAddr.sin_port = htons(port);

	if (connect(m_Sock, (struct sockaddr*)&sockAddr, sizeof(sockAddr)))
	{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
		return false;
	}

	return true;
}


void NetSocket::Send(shared_ptr<IPacket> pPacket, bool clearTimeOut)
{
	if (clearTimeOut)
		m_TimeOut = 0;

	m_OutList.push_back(pPacket);
}


void NetSocket::SetBlocking(bool blocking)
{
#ifdef WIN32
	unsigned long val = blocking ? 0 : 1;
	ioctlsocket(m_Sock, FIONBIO, &val);
#else
	int val = fcntl(m_Sock, F_GETFL, 0);
	if (blocking)
		val &= ~(O_NONBLOCK);
	else
		val |= O_NONBLOCK;

	fcntl(m_Sock, F_SETFL, val);
#endif
}


void NetSocket::VHandleOutput()
{
	int fSent = 0;
	do
	{
		ASSERT(!m_OutList.empty());
		PacketList::iterator i = m_OutList.begin();

		shared_ptr<IPacket> pPacket = *i;
		const char* buf = pPacket->VGetData();
		int len = (int)(pPacket->VGetSize());

		int rc = send(m_Sock, buf + m_SendOfs, len - m_SendOfs, 0);
		if (rc > 0)
		{
			g_pSocketManager->AddToOutbound(rc);
			m_SendOfs += rc;
			fSent = 1;
		}
		else if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			HandleException();
			fSent = 0;
		}
		else
		{
			fSent = 0;
		}

		if (m_SendOfs == pPacket->VGetSize())
		{
			m_OutList.pop_front();
			m_SendOfs = 0;
		}

	} while (fSent && !m_OutList.empty());
}


void NetSocket::VHandleInput()
{
	bool isRecieved = false;
	u_long packetSize = 0;
	int rc = recv(m_Sock, m_RecvBuf + m_RecvBegin + m_RecvOfs, RECV_BUFFER_SIZE - (m_RecvBegin + m_RecvOfs), 0);

	char recievedNumStr[1024];
	sprintf_s(recievedNumStr, 1024, "Incoming: %6d bytes. Begin %6d Offset %4d\n", rc, m_RecvBegin, m_RecvOfs);
	LOG("Network", recievedNumStr);

	if (rc == 0)
	{
		return;
	}

	if (rc == SOCKET_ERROR)
	{
		m_DeleteFlag = 1;
		return;
	}

	const int ulSize = sizeof(u_long);
	unsigned int newData = m_RecvOfs + rc;
	int processedData = 0;

	while (newData > ulSize)
	{
		// There are two types of packets at the lowest level of our design:
		// BinaryPacket - Sends the size as a positive 4 byte integer
		// TextPacket - Sends 0 for the size, the parser will search for a CR

		packetSize = *(u_long*)(m_RecvBuf + m_RecvBegin);
		packetSize = ntohl(packetSize);

		if (m_IsBinaryProtocol)
		{
			// We still haven't recieved whole packet
			if (newData < packetSize)
				break;

			if (packetSize > MAX_PACKET_SIZE)
			{
				// Buffer overflows!
				HandleException();
				return;
			}

			if (newData >= packetSize)
			{
				
				shared_ptr<BinaryPacket> pPacket(New BinaryPacket(&m_RecvBuf[m_RecvBegin + ulSize], packetSize - ulSize));
				m_InList.push_back(pPacket);
				isRecieved = true;
				processedData += packetSize;
				newData -= packetSize;
				m_RecvBegin += packetSize;
			}
		}
		else
		{
			// The text protocol waits for a carraige return and creates a string
			char* cr = (char*)(memchr(&m_RecvBuf[m_RecvBegin+ m_RecvOfs], 0x0a, rc));
			if (cr)
			{
				*(cr + 1) = 0;
				shared_ptr<TextPacket> pPacket(New TextPacket(&m_RecvBuf[m_RecvBegin]));
				m_InList.push_back(pPacket);
				packetSize = cr - &m_RecvBuf[m_RecvBegin];
				isRecieved = true;

				processedData += packetSize;
				newData -= packetSize;
				m_RecvBegin += packetSize;
			}
		}
	}
	g_pSocketManager->AddToInbound(rc);
	m_RecvOfs = newData;

	if (isRecieved)
	{
		if (m_RecvOfs == 0)
		{
			m_RecvBegin = 0;
		}
		else if (m_RecvBegin + m_RecvOfs + MAX_PACKET_SIZE > RECV_BUFFER_SIZE)
		{
			// We don't want to overrun the buffer - so we copy the leftover bits 
			// to the beginning of the recieve buffer and start over
			int leftover = m_RecvOfs;
			memcpy(m_RecvBuf, &m_RecvBuf[m_RecvBegin], m_RecvOfs);
			m_RecvBegin = 0;
		}
	}
}


NetListenSocket::NetListenSocket(int port)
{
	m_Port = 0;
	Init(port);
}


void NetListenSocket::Init(int port)
{
	struct sockaddr_in sock;
	int value = 1;

	if ((m_Sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		ASSERT(0&&"Can't create socket");
	}

	if (setsockopt(m_Sock, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value)) == SOCKET_ERROR)
	{
		perror("NetListenSocket::Init: setsockopt");
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
		ASSERT(0&&"NetListenSocket Error: Init failed to set socket options");

	}

	memset(&sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = ADDR_ANY;
	sock.sin_port = htons(port);

	// bind to port
	if (bind(m_Sock, (struct sockaddr*)&sock, sizeof(sock)) == SOCKET_ERROR)
	{
		perror("NetListenSocket::Init: bind");
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
		ASSERT(0&&"NetListenSocket Error: Init failed to bind");
	}

	SetBlocking(false);

	// Start listening
	if (listen(m_Sock, 256) == SOCKET_ERROR)
	{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
		ASSERT(0&&"NetListenSocket Error: Init failed to listen");
	}

	m_Port = port;
}


void NetListenSocket::InitScan(int portMin, int portMax)
{
	struct sockaddr_in sock;
	int port, x = 1;

	if ((m_Sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		EXIT_ASSERT;
			exit(1);
	}

	if (setsockopt(m_Sock, SOL_SOCKET, SO_REUSEADDR, (char*)&x, sizeof(x)) == SOCKET_ERROR)
	{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
		EXIT_ASSERT;
			exit(1);
	}

	memset(&sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	for (port = portMin; port < portMax; ++port)
	{
		sock.sin_port = htons(port);
		// bind to port
		if (bind(m_Sock, (struct sockaddr*)&sock, sizeof(sock)) != SOCKET_ERROR)
			break;
	}

	if (port == portMax)
	{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
		EXIT_ASSERT;
			exit(1);
	}

	SetBlocking(false);

	// Start listening
	if (listen(m_Sock, 8) == SOCKET_ERROR)
	{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
		EXIT_ASSERT;
			exit(1);
	}

	m_Port = port;
}


SOCKET NetListenSocket::AcceptConnection(unsigned int *pAddr)
{
	SOCKET newSock;
	struct sockaddr_in sockAddr;
	int size = sizeof(sockAddr);

	if ((newSock = accept(m_Sock, (struct sockaddr*)&sockAddr, &size)) == INVALID_SOCKET)
		return INVALID_SOCKET;

	if (getpeername(newSock, (struct sockaddr*)&sockAddr, &size) == SOCKET_ERROR)
	{
		closesocket(newSock);
		return INVALID_SOCKET;
	}
	*pAddr = ntohl(sockAddr.sin_addr.s_addr);
	return newSock;
}


BaseSocketManager::BaseSocketManager()
{
	m_Inbound = 0;
	m_Outbound = 0;
	m_MaxOpenSockets = 0;
	m_SubnetMask = 0;
	m_Subnet = 0xffffffff;
	m_NextSocketId = 0;

	g_pSocketManager = this;
	ZeroMemory(&m_WsaData, sizeof(WSADATA));
}



bool BaseSocketManager::Init()
{
	if (WSAStartup(0x0202, &m_WsaData) == 0)
		return true;
	else
	{
		ERROR("WSAStartup failure!");
		return false;
	}
}



void BaseSocketManager::Shutdown()
{

	while (!m_SockList.empty())
	{
		delete *m_SockList.begin();
		m_SockList.pop_front();
	}

	WSACleanup();
}


int BaseSocketManager::AddSocket(NetSocket* socket)
{
	socket->m_Id = m_NextSocketId;
	m_SockMap[m_NextSocketId] = socket;
	++m_NextSocketId;

	m_SockList.push_front(socket);
	if (m_SockList.size() > m_MaxOpenSockets)
		++m_MaxOpenSockets;

	return socket->m_Id;
}


void BaseSocketManager::RemoveSocket(NetSocket* socket)
{
	m_SockList.remove(socket);
	m_SockMap.erase(socket->m_Id);
	SAFE_DELETE(socket);
}


NetSocket* BaseSocketManager::FindSocket(int sockId)
{
	SocketIdMap::iterator i = m_SockMap.find(sockId);
	if (i == m_SockMap.end())
	{
		return NULL;
	}
	return i->second;
}



int BaseSocketManager::GetIpAddress(int sockId)
{
	NetSocket *pSocket = FindSocket(sockId);
	if (pSocket)
	{
		return pSocket->GetIpAddress();
	}
	else
	{
		return 0;
	}
}



bool BaseSocketManager::Send(int sockId, shared_ptr<IPacket> packet)
{
	NetSocket* pSock = FindSocket(sockId);
	if (!pSock)
		return false;
	pSock->Send(packet);
	return true;
}



void BaseSocketManager::DoSelect(int pauseMicroSecs, bool handleInput)
{
	timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = pauseMicroSecs;    // 100 microseconds is 0.1 milliseconds or .0001 seconds

	fd_set inp_set, out_set, exc_set;
	int maxdesc;

	FD_ZERO(&inp_set);
	FD_ZERO(&out_set);
	FD_ZERO(&exc_set);

	maxdesc = 0;

	// Set everything up for the select
	for (SocketList::iterator i = m_SockList.begin(); i != m_SockList.end(); ++i)
	{
		NetSocket* pSock = *i;
		if ((pSock->m_DeleteFlag & 1) || pSock->m_Sock == INVALID_SOCKET)
			continue;

		if (handleInput)
			FD_SET(pSock->m_Sock, &inp_set);

		FD_SET(pSock->m_Sock, &exc_set);

		if (pSock->VHasOutput())
			FD_SET(pSock->m_Sock, &out_set);

		if ((int)pSock->m_Sock > maxdesc)
			maxdesc = (int)pSock->m_Sock;

	}

	int selRet = 0;

	// Do the select (duration passed in as timeVal, NULL to block until event)
	selRet = select(maxdesc + 1, &inp_set, &out_set, &exc_set, &timeVal);
	if (selRet == SOCKET_ERROR)
	{
		PrintError();
		return;
	}

	// Handle input, output, and exceptions

	if (selRet)
	{
		for (SocketList::iterator i = m_SockList.begin(); i != m_SockList.end(); ++i)
		{
			NetSocket* pSock = *i;

			if ((pSock->m_DeleteFlag & 1) || pSock->m_Sock == INVALID_SOCKET)
				continue;

			if (FD_ISSET(pSock->m_Sock, &exc_set))
			{
				pSock->HandleException();
			}

			if (!(pSock->m_DeleteFlag & 1) && FD_ISSET(pSock->m_Sock, &out_set))
			{
				pSock->VHandleOutput();
			}

			if (handleInput
				&& !(pSock->m_DeleteFlag & 1) && FD_ISSET(pSock->m_Sock, &inp_set))
			{
				pSock->VHandleInput();
			}
		}
	}

	unsigned int timeNow = timeGetTime();

	// Handle deleting any sockets
	SocketList::iterator i = m_SockList.begin();
	while (i != m_SockList.end())
	{
		NetSocket* pSock = *i;
		if (pSock->m_TimeOut)
		{
			if (pSock->m_TimeOut < timeNow)
			{
				pSock->VTimeOut();
			}
		}

		if (pSock->m_DeleteFlag & 1)
		{
			switch (pSock->m_DeleteFlag)
			{
			case 1:
				g_pSocketManager->RemoveSocket(pSock);
				i = m_SockList.begin();
				break;
			case 3:
				pSock->m_DeleteFlag = 2;
				if (pSock->m_Sock != INVALID_SOCKET)
				{
					closesocket(pSock->m_Sock);
					pSock->m_Sock = INVALID_SOCKET;
				}
				break;
			}
		}

		++i;

	}
}


bool BaseSocketManager::IsInternal(unsigned int ipAddr)
{
	if (!m_SubnetMask)
		return false;

	if ((ipAddr & m_SubnetMask) == m_Subnet)
		return false;

	return true;
}



unsigned int BaseSocketManager::GetHostByName(const std::string& hostName)
{
	//This will retrieve the ip details and put it into pHostEnt structure
	struct hostent* pHostEnt;
	pHostEnt = gethostbyname(hostName.c_str());
	struct sockaddr_in tmpSockAddr; //Placeholder for the ip address

	if (pHostEnt == NULL)
	{
		ASSERT(0 && "gethostbyname() failed");
		return 0;
	}

	memcpy(&tmpSockAddr.sin_addr, pHostEnt->h_addr, pHostEnt->h_length);
	return ntohl(tmpSockAddr.sin_addr.s_addr);
}



const char *BaseSocketManager::GetHostByAddr(unsigned int ip)
{
	static char host[256];

	int netip = htonl(ip);
	struct hostent* lpHostEnt = gethostbyaddr((const char*)&netip, 4, PF_INET);

	if (lpHostEnt)
	{
		strcpy_s(host, 256, lpHostEnt->h_name);
		return host;
	}

	return NULL;
}


void BaseSocketManager::PrintError()
{
	int realError = WSAGetLastError();
	char* reason;

	switch (realError)
	{
	case WSANOTINITIALISED: reason = (char*)"A successful WSAStartup must occur before using this API."; break;
	case WSAEFAULT: reason = (char*)"The Windows Sockets implementation was unable to allocated needed resources for its internal operations, or the readfds, writefds, exceptfds, or timeval parameters are not part of the user address space."; break;
	case WSAENETDOWN: reason = (char*)"The network subsystem has failed."; break;
	case WSAEINVAL: reason = (char*)"The timeout value is not valid, or all three descriptor parameters were NULL."; break;
	case WSAEINTR: reason = (char*)"The (blocking) call was canceled via WSACancelBlockingCall."; break;
	case WSAEINPROGRESS: reason = (char*)"A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function."; break;
	case WSAENOTSOCK: reason = (char*)"One of the descriptor sets contains an entry which is not a socket."; break;
	default: reason = (char*)"Unknown.";
	}

	char buffer[256];
	sprintf(buffer, "SOCKET error: %s", reason);
	LOG("Network", buffer);
}

bool ClientSocketManager::Connect()
{
	if (!BaseSocketManager::Init())
		return false;

	RemoteEventSocket* pSocket = New RemoteEventSocket;

	if (!pSocket->Connect(GetHostByName(m_HostName), m_Port))
	{
		SAFE_DELETE(pSocket);
		return false;
	}
	AddSocket(pSocket);
	return true;
}




void GameServerListenSocket::VHandleInput()
{
	unsigned int ipAddr;
	SOCKET newSock = AcceptConnection(&ipAddr);

	int value = 1;
	setsockopt(newSock, SOL_SOCKET, SO_DONTLINGER, (char *)&value, sizeof(value));

	if (newSock != INVALID_SOCKET)
	{
		RemoteEventSocket* sock = New RemoteEventSocket(newSock, ipAddr);
		int sockId = g_pSocketManager->AddSocket(sock);
		int ipAddress = g_pSocketManager->GetIpAddress(sockId);
		shared_ptr<EvtData_Remote_Client> pEvent(New EvtData_Remote_Client(sockId, ipAddress));
		IEventManager::Get()->VQueueEvent(pEvent);
	}
}


void RemoteEventSocket::VHandleInput()
{
	NetSocket::VHandleInput();

	// Traverse the list of m_InList packets and do something useful with them
	while (!m_InList.empty())
	{
		shared_ptr<IPacket> packet = *m_InList.begin();
		m_InList.pop_front();
		if (!strcmp(packet->VGetType(), BinaryPacket::s_Type))
		{
			const char* buf = packet->VGetData();
			int size = (int)(packet->VGetSize());

			std::istrstream in(buf + sizeof(u_long), (size - sizeof(u_long)));

			int type;
			in >> type;
			switch (type)
			{
			case NETMSG_EVENT:
				CreateEvent(in);
				break;

			case NETMSG_PLAYER_LOGININ:
			{
				int serverSockId, actorId;
				in >> serverSockId;
				in >> actorId;
				in >> g_pApp->m_Options.m_Level;		
				shared_ptr<EvtData_Network_Player_Actor_Assignment> pEvent(New EvtData_Network_Player_Actor_Assignment(actorId, serverSockId));
				IEventManager::Get()->VQueueEvent(pEvent);
				break;
			}

			default:
				ERROR("Unknown message type.");
			}
		}
		else if (!strcmp(packet->VGetType(), TextPacket::s_Type))
		{
			LOG("Network", packet->VGetData() + sizeof(u_long));
		}
	}
}


void RemoteEventSocket::CreateEvent(std::istrstream &in)
{
	
	EventType eventType;
	in >> eventType;

	IEventDataPtr pEvent(CREATE_EVENT(eventType));
	if (pEvent)
	{
		pEvent->VDeserialize(in);
		IEventManager::Get()->VQueueEvent(pEvent);
	}
	else
	{
		ERROR("ERROR Unknown event type from remote: 0x" + ToStr(eventType, 16));
	}
}




void NetworkEventForwarder::ForwardEvent(IEventDataPtr pEventData)
{
	std::ostrstream out;

	out << (int)(RemoteEventSocket::NETMSG_EVENT) << " ";
	out << pEventData->VGetEventType() << " ";
	pEventData->VSerialize(out);
	out << "\r\n";

	shared_ptr<BinaryPacket> eventMsg(New BinaryPacket(out.rdbuf()->str(), (u_long)out.pcount()));

	g_pSocketManager->Send(m_SockId, eventMsg);
}



NetworkGameView::NetworkGameView()
{
	m_SockId = INVALID_SOCKET_ID;
	m_ActorId = INVALID_ACTOR_ID;
	IEventManager::Get()->VAddListener(MakeDelegate(this, &NetworkGameView::NewActorDelegate), EvtData_New_Actor::s_EventType);
}



void NetworkGameView::AttachRemotePlayer(int sockId)
{
	m_SockId = sockId;

	std::ostrstream out;

	out << (int)(RemoteEventSocket::NETMSG_PLAYER_LOGININ) << " ";
	out << m_SockId << " ";
	out << m_ActorId << " ";
	out << g_pApp->m_Options.m_Level << " ";
	out << "\r\n";

	shared_ptr<BinaryPacket> gvidMsg(New BinaryPacket(out.rdbuf()->str(), (u_long)out.pcount()));
	g_pSocketManager->Send(m_SockId, gvidMsg);
}



void NetworkGameView::VOnAttach(GameViewId viewId, ActorId aid)
{
	m_ViewId = viewId;
	m_ActorId = aid;
}




void NetworkGameView::VOnUpdate(unsigned long deltaMs)
{
	if (m_ActorId != INVALID_ACTOR_ID)
	{
		IEventManager::Get()->VRemoveListener(MakeDelegate(this, &NetworkGameView::NewActorDelegate), EvtData_New_Actor::s_EventType);
	}
};



void NetworkGameView::NewActorDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_New_Actor> pCastEventData = static_pointer_cast<EvtData_New_Actor>(pEventData);
	ActorId actorId = pCastEventData->GetActorId();
	StrongActorPtr pActor = MakeStrongPtr(g_pApp->m_pGame->VGetActor(actorId));

	if (pActor && pActor->GetType() == "Character")
	{
		if (pCastEventData->GetViewId() == m_ViewId)
		{
			m_ActorId = actorId;
			shared_ptr<EvtData_Network_Player_Actor_Assignment> pEvent(New EvtData_Network_Player_Actor_Assignment(m_ActorId, m_SockId));
			IEventManager::Get()->VQueueEvent(pEvent);
		}
	}
}






