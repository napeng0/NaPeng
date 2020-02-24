#pragma once
#include"EventManager.h"
#include"GameCode\GameCode.h"
#include"LUA\ScriptEvent.h"


void RegisterEngineScriptEvents();

extern GameViewId INVALID_GAMEVIEW_ID;
class EvtData_New_Actor : public BaseEventData
{
	ActorId m_ActorId;
	GameViewId m_ViewId;

public:
	static const EventType s_EventType;

public:
	EvtData_New_Actor()
	{
		m_ActorId = INVALID_ACTOR_ID;
		m_ViewId = INVALID_GAMEVIEW_ID;
	}

	explicit EvtData_New_Actor(ActorId actorId, GameViewId viewId = INVALID_GAMEVIEW_ID)
	{
		m_ActorId = actorId;
		m_ViewId = viewId;
	}



	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_ActorId;
		in >> m_ViewId;
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_ActorId << " ";
		out << m_ViewId << " ";
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_New_Actor(m_ActorId, m_ViewId));
	}

	virtual const char* VGetName() const
	{
		return "EvtData_New_Actor";
	}

	const ActorId GetActorId() const
	{
		return m_ActorId;
	}

	GameViewId GetViewId() const
	{
		return m_ViewId;
	}
};

class EvtData_Destroy_Actor : public BaseEventData
{
	ActorId m_ActorId;

public:
	static const EventType s_EventType;

public:

	explicit EvtData_Destroy_Actor(ActorId actorId= INVALID_ACTOR_ID)

	{
		m_ActorId = actorId;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Destroy_Actor(m_ActorId));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_ActorId;
		
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_ActorId;
	}


	virtual const char* VGetName() const
	{
		return "EvtData_Destroy_Actor";
	}

	const ActorId GetActorId() const
	{
		return m_ActorId;
	}
};


class EvtData_Move_Actor : public BaseEventData
{
	ActorId m_ActorId;
	Mat4x4 m_Matrix;

public:
	static const EventType s_EventType;

public:

	EvtData_Move_Actor()
	{
		m_ActorId = INVALID_ACTOR_ID;
	}

	explicit EvtData_Move_Actor(ActorId actorId, const Mat4x4& matrix)

	{
		m_ActorId = actorId;
		m_Matrix = matrix;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Move_Actor(m_ActorId, m_Matrix));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_ActorId << " ";
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				out << m_Matrix.m[i][j] << " ";
			}
		}
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_ActorId ;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				in >> m_Matrix.m[i][j];
			}
		}
	}


	virtual const char* VGetName() const
	{
		return "EvtData_Move_Actor";
	}

	const ActorId GetActorId() const
	{
		return m_ActorId;
	}

	const Mat4x4& GetMatrix() const
	{
		return m_Matrix;
	}

};


class EvtData_New_Render_Component : public BaseEventData
{
	ActorId m_ActorId;
	shared_ptr<SceneNode> m_pSceneNode;

public:
	static const EventType s_EventType;

public:

	EvtData_New_Render_Component()
	{
		m_ActorId = INVALID_ACTOR_ID;
	}

	explicit EvtData_New_Render_Component(ActorId actorId, shared_ptr<SceneNode> pSceneNode)

	{
		m_ActorId = actorId;
		m_pSceneNode = pSceneNode;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_New_Render_Component(m_ActorId, m_pSceneNode));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		ERROR(VGetName() + std::string("should not be serialized!"));
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		ERROR(VGetName() + std::string("should not be serialized!"));
	}


	virtual const char* VGetName() const
	{
		return "EvtData_New_Render_Component";
	}

	const ActorId GetActorId() const
	{
		return m_ActorId;
	}

	shared_ptr<SceneNode> GetSceneNode() const
	{
		return m_pSceneNode;
	}

};


class EvtData_Modified_Render_Component : public BaseEventData
{
	ActorId m_ActorId;

public:
	static const EventType s_EventType;

public:

	EvtData_Modified_Render_Component()
	{
		m_ActorId = INVALID_ACTOR_ID;
	}

	explicit EvtData_Modified_Render_Component(ActorId actorId)

	{
		m_ActorId = actorId;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Modified_Render_Component(m_ActorId));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_ActorId;
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_ActorId;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Modified_Render_Component";
	}

	const ActorId GetActorId() const
	{
		return m_ActorId;
	}

};


class EvtData_Environment_Loaded : public BaseEventData
{

public:
	static const EventType s_EventType;

public:

	EvtData_Environment_Loaded() {}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Environment_Loaded());
	}


	virtual const char* VGetName() const
	{
		return "EvtData_Environment_Loaded";
	}

};


class EvtData_Remote_Environment_Loaded : public BaseEventData
{
public:
	static const EventType s_EventType;

	EvtData_Remote_Environment_Loaded(void) { }
	virtual const EventType& VGetEventType(void) const { return s_EventType; }
	virtual IEventDataPtr VCopy(void) const
	{
		return IEventDataPtr(New EvtData_Remote_Environment_Loaded());
	}
	virtual const char* VGetName(void) const { return "EvtData_Remote_Environment_Loaded"; }
};


class EvtData_Request_Start_Game : public BaseEventData
{

public:
	static const EventType s_EventType;

public:

	EvtData_Request_Start_Game() {}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Request_Start_Game());
	}


	virtual const char* VGetName() const
	{
		return "EvtData_Request_Start_Game";
	}

};


class EvtData_Remote_Client : public BaseEventData
{
	int m_SocketId;
	int m_IpAddress;

public:
	static const EventType s_EventType;

public:

	EvtData_Remote_Client()
	{
		m_SocketId = 0;
		m_IpAddress = 0;
	}

	explicit EvtData_Remote_Client(const int socketId, const int ipAddress)
	{
		m_SocketId = socketId;
		m_IpAddress = ipAddress;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Remote_Client(m_SocketId, m_IpAddress));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_SocketId<< " ";
		out << m_IpAddress;
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_SocketId;
		in >> m_IpAddress;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Remote_Client";
	}

	int GetSocketId() const
	{
		return m_SocketId;
	}

	int GetIpAddress() const
	{
		return m_IpAddress;
	}

};


class EvtData_Update_Tick : public BaseEventData
{
	int m_DeltaMs;

public:
	static const EventType s_EventType;

public:

	explicit EvtData_Update_Tick(const int deltaMs)
	{
		m_DeltaMs = deltaMs;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Update_Tick(m_DeltaMs));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		ERROR("Can't serializing update ticks!");
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Update_Tick";
	}

};


class EvtData_Network_Player_Actor_Assignment : public BaseEventData
{
	int m_SocketId;
	ActorId m_ActorId;

public:
	static const EventType s_EventType;

public:

	EvtData_Network_Player_Actor_Assignment()
	{
		m_SocketId = -1;
		m_SocketId = INVALID_ACTOR_ID;
	}

	explicit EvtData_Network_Player_Actor_Assignment(const ActorId actorId, const int socketId)
	{
		m_SocketId = socketId;
		m_ActorId = actorId;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Network_Player_Actor_Assignment(m_ActorId, m_SocketId));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_ActorId << " ";
		out << m_SocketId;
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_ActorId;
		in >> m_SocketId;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Network_Player_Actor_Assignment";
	}

	int GetSocketId() const
	{
		return m_SocketId;
	}

};


class EvtData_Decompress_Request : public BaseEventData
{
	std::wstring m_ZipFileName;
	std::string m_FileName;

public:
	static const EventType s_EventType;

public:

	explicit EvtData_Decompress_Request(std::wstring zipFileName, std::string fileName)
	{
		m_ZipFileName = zipFileName;
		m_FileName = fileName;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Decompress_Request(m_ZipFileName, m_FileName));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		ERROR("Can't serializing decompression request");
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Decompression_Request";
	}

	const std::wstring& GetZipFileName() const
	{
		return m_ZipFileName;
	}

	const std::string& GetFileName() const
	{
		return m_FileName;
	}

};


class EvtData_Decompression_Progress : public BaseEventData
{
	std::wstring m_ZipFileName;
	std::string m_FileName;
	int m_Progress;
	void* m_Buffer;

public:
	static const EventType s_EventType;

public:

	explicit EvtData_Decompression_Progress(int progress, std::wstring zipFileName, std::string fileName, void* buffer)
	{
		m_ZipFileName = zipFileName;
		m_FileName = fileName;
		m_Progress = progress;
		m_Buffer = buffer;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Decompression_Progress(m_Progress, m_ZipFileName, m_FileName, m_Buffer));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		ERROR("Can't serializing decompression request");
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Decompression_Progress";
	}

};

//This event is sent from server to client
class EvtData_Request_New_Actor : public BaseEventData
{
	std::string m_ActorResource;
	bool m_HasInitialTransform;
	Mat4x4 m_InitialTransform;
	ActorId m_ServerActorId;
	GameViewId m_ViewId;

public:
	static const EventType s_EventType;

public:

	explicit EvtData_Request_New_Actor(const string& actorResource, const Mat4x4* initTransform = NULL, const ActorId serverId= INVALID_ACTOR_ID, const GameViewId viewId= INVALID_GAMEVIEW_ID)
	{
		m_ActorResource= actorResource;
		if (initTransform)
		{
			m_HasInitialTransform = true;
			m_InitialTransform = *initTransform;
		}
		else
		{
			m_HasInitialTransform = false;
		}

		m_ServerActorId = serverId;
		m_ViewId = viewId;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Request_New_Actor(m_ActorResource, m_HasInitialTransform? &m_InitialTransform: NULL, m_ServerActorId, m_ViewId));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_ActorResource << " ";
		out << m_HasInitialTransform << " ";
		if (m_HasInitialTransform)
		{
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					out << m_InitialTransform.m[i][j] << " ";
				}
			}
		}

		out << m_ServerActorId << " ";
		out << m_ViewId << " ";
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_ActorResource;
		in >> m_HasInitialTransform;
		if (m_HasInitialTransform)
		{
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					in >> m_InitialTransform.m[i][j];
				}
			}
		}

		in >> m_ServerActorId;
		in >> m_ViewId;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Request_New_Actor";
	}

	const std::string& GetActorResource() const
	{
		return m_ActorResource;
	}

	const Mat4x4* GetInitialTransform() const
	{
		return m_HasInitialTransform ? &m_InitialTransform : NULL;
	}

	const ActorId GetServerActorId() const
	{
		return m_ServerActorId;
	}

	GameViewId GetViewId() const
	{
		return m_ViewId;
	}
};



//This event is sent by any system to request game logic to destroy an actor
class EvtData_Request_Destroy_Actor : public ScriptEvent
{
	ActorId m_ActorId;

public:
	static const EventType s_EventType;

public:

	EvtData_Request_Destroy_Actor()
	{
		m_ActorId = INVALID_ACTOR_ID;
	}

	explicit EvtData_Request_Destroy_Actor(ActorId actorId)
	{
		m_ActorId = actorId;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_Request_Destroy_Actor(m_ActorId));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_ActorId;
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_ActorId;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_Request_Destroy_Actor";
	}

	ActorId GetActorId() const
	{
		return m_ActorId;
	}

	virtual bool VBuildEventFromScript()
	{
		if (m_EventData.IsInteger())
		{
			m_ActorId = m_EventData.GetInteger();
			return true;
		}

		return false;
	}

	EXPORT_FOR_SCRIPT_EVENT(EvtData_Request_Destroy_Actor);
};


class EvtData_PlaySound : public ScriptEvent
{
	std::string m_SoundResource;

public:
	static const EventType s_EventType;

public:

	EvtData_PlaySound() {}

	explicit EvtData_PlaySound(const std::string& soundResource)
	{
		m_SoundResource = soundResource;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_PlaySound(m_SoundResource));
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_SoundResource;
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_SoundResource;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_PlaySound";
	}

	const std::string& GetResource() const
	{
		return m_SoundResource;
	}

	virtual bool VBuildEventFromScript();

	EXPORT_FOR_SCRIPT_EVENT(EvtData_PlaySound);
};













