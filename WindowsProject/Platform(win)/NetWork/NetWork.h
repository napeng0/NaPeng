#pragma once

#include<sys\types.h>
#include"EventManager\EventManager.h"
#include"GameCode\interface.h"
#include<WinSock2.h>


#define MAX_PACKET_SIZE (256)
#define RECV_BUFFER_SIZE (MAX_PACKET_SIZE * 512)
#define MAX_QUEUE_PER_PLAYER (10000)
#define MAGIC_NUMBER		(0x1f2e3d4c)
#define IPMANGLE(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|((d)))
#define INVALID_SOCKET_ID (-1)



class NetSocket;

//Packet interface;
class IPacket
{
public:
	virtual const char* VGetType() const = 0;
	virtual const char* VGetData() const = 0;
	virtual u_long VGetSize() const = 0;
	virtual ~IPacket() { }
};


class BinaryPacket : public IPacket
{
protected:
	char *m_pData;

public:
	inline BinaryPacket(const char * data, u_long size);
	inline BinaryPacket(u_long size);
	virtual ~BinaryPacket() { SAFE_DELETE(m_pData); }
	virtual const char* VGetType() const { return s_Type; }
	virtual const char* VGetData() const { return m_pData; }
	virtual u_long VGetSize() const { return ntohl(*(u_long *)m_pData); }
	inline void MemCpy(const char* data, size_t size, int destOffset);

	static const char* s_Type;
};


inline BinaryPacket::BinaryPacket(const char* data, u_long size)
{
	m_pData = New char[size + sizeof(u_long)];
	ASSERT(m_pData);
	*(u_long*)m_pData = htonl(size + sizeof(u_long));
	memcpy(m_pData + sizeof(u_long), data, size);
}


inline BinaryPacket::BinaryPacket(u_long size)
{
	m_pData = New char[size + sizeof(u_long)];
	ASSERT(m_pData);
	*(u_long *)m_pData = htonl(size + sizeof(u_long));
}


inline void BinaryPacket::MemCpy(const char* data, size_t size, int destOffset)
{
	ASSERT(size + destOffset <= VGetSize() - sizeof(u_long));
	memcpy(m_pData + sizeof(u_long) + destOffset , data, size);
}


class TextPacket : public BinaryPacket
{
public:
	TextPacket(const char* text);
	virtual const char* VGetType() const { return s_Type; }

	static const char *s_Type;
};


class NetSocket
{
	friend class BaseSocketManager;
	typedef std::list< shared_ptr<IPacket> > PacketList;

protected:
	SOCKET m_Sock;
	int m_Id;				// SocketId

	// If deleteFlag has bit 2 set, exceptions only close the
	//   socket and set to INVALID_SOCKET, and do not delete the NetSocket
	int m_DeleteFlag;

	PacketList m_OutList;
	PacketList m_InList;

	char m_RecvBuf[RECV_BUFFER_SIZE];
	unsigned int m_RecvOfs, m_RecvBegin;
	bool m_IsBinaryProtocol;

	int m_SendOfs;
	unsigned int m_TimeOut;
	unsigned int m_IpAddr;

	int m_Internal;
	int m_TimeCreated;

public:
	NetSocket();											// Clients use this to initialize a NetSocket prior to calling Connect.
	NetSocket(SOCKET newSock, unsigned int hostIP);		// Servers use this when new clients attach.
	virtual ~NetSocket();

	bool Connect(unsigned int ip, unsigned int port, bool forceCoalesce = 0);
	void SetBlocking(bool blocking);
	void Send(shared_ptr<IPacket> pPacket, bool clearTimeOut = 1);

	virtual int  VHasOutput() { return !m_OutList.empty(); }
	virtual void VHandleOutput();
	virtual void VHandleInput();
	virtual void VTimeOut() { m_TimeOut = 0; }

	void HandleException() { m_DeleteFlag |= 1; }

	void SetTimeOut(unsigned int ms = 45 * 1000) { m_TimeOut = timeGetTime() + ms; }

	int GetIpAddress() { return m_IpAddr; }


};


class BaseSocketManager
{
protected:
	WSADATA m_WsaData;

	typedef std::list<NetSocket*> SocketList;
	typedef std::map<int, NetSocket*> SocketIdMap;

	SocketList m_SockList;
	SocketIdMap m_SockMap;

	int m_NextSocketId;
	unsigned int m_Inbound;
	unsigned int m_Outbound;
	unsigned int m_MaxOpenSockets;
	unsigned int m_SubnetMask;
	unsigned int m_Subnet;

	NetSocket* FindSocket(int sockId);

public:
	BaseSocketManager();
	virtual ~BaseSocketManager() { Shutdown(); }

	void DoSelect(int pauseMs, bool handleInput = true);

	bool Init();
	void Shutdown();
	void PrintError();

	int AddSocket(NetSocket* socket);
	void RemoveSocket(NetSocket* socket);

	unsigned int GetHostByName(const std::string& hostName);
	const char* GetHostByAddr(unsigned int ip);

	int GetIpAddress(int sockId);

	void SetSubnet(unsigned int subnet, unsigned int subnetMask)
	{
		m_Subnet = subnet;
		m_SubnetMask = subnetMask;
	}
	bool IsInternal(unsigned int ipAddr);

	bool Send(int sockId, shared_ptr<IPacket> pPacket);

	void AddToOutbound(int rc) { m_Outbound += rc; }
	void AddToInbound(int rc) { m_Inbound += rc; }

};


extern BaseSocketManager* g_pSocketManager;


class ClientSocketManager : public BaseSocketManager
{
	std::string m_HostName;
	unsigned int m_Port;

public:
	ClientSocketManager(const std::string& hostName, unsigned int port)
	{
		m_HostName = hostName;
		m_Port = port;
	}

	bool Connect();
};


class NetListenSocket : public NetSocket
{
public:
	NetListenSocket() { };
	NetListenSocket(int port);

	void Init(int port);
	void InitScan(int portMin, int portMax);
	SOCKET AcceptConnection(unsigned int* pAddr);

	unsigned short m_Port;
};


class GameServerListenSocket : public NetListenSocket
{
public:
	GameServerListenSocket(int port) { Init(port); }

	virtual void VHandleInput();
};


class RemoteEventSocket : public NetSocket
{
public:
	enum
	{
		NETMSG_EVENT,
		NETMSG_PLAYER_LOGININ,
	};

	// Server accepting a client
	RemoteEventSocket(SOCKET newSock, unsigned int hostIP)
		: NetSocket(newSock, hostIP)
	{
	}

	// Client attach to server
	RemoteEventSocket() { };

	virtual void VHandleInput();

protected:
	void CreateEvent(std::istrstream &in);
};


class NetworkEventForwarder
{
public:
	NetworkEventForwarder(int sockId) { m_SockId = sockId; }

	// Delegate that forwards events through the network.
	void ForwardEvent(IEventDataPtr pEventData);

protected:
	int m_SockId;
};



class NetworkGameView : public IGameView
{
protected:
	GameViewId m_ViewId;
	ActorId m_ActorId;
	int m_SockId;


public:
	NetworkGameView();

	// IGameView Implementation 
	virtual HRESULT VOnRestore() { return S_OK; }
	virtual void VOnRender(double time, float elapsedTime) { }
	virtual HRESULT VOnLostDevice() { return S_OK; }
	virtual GameViewType VGetType() { return GV_REMOTE; }
	virtual GameViewId VGetId() const { return m_ViewId; }
	virtual void VOnAttach(GameViewId vid, ActorId aid);
	virtual LRESULT CALLBACK VOnMsgProc(AppMsg msg) { return 0; }
	virtual void VOnUpdate(unsigned long deltaMs);

	void NewActorDelegate(IEventDataPtr pEventData);
	void SetPlayerActorId(ActorId actorId) { m_ActorId = actorId; }
	void AttachRemotePlayer(int sockId);

	int HasRemotePlayerAttached() { return m_SockId != INVALID_SOCKET_ID; }


};