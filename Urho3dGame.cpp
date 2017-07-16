

#pragma once 



#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "Urho3dGame.h"

#include <Urho3D/DebugNew.h>


URHO3D_DEFINE_APPLICATION_MAIN(CUrho3dGame)

CUrho3dGame::CUrho3dGame(Context* context) : CGameHandler(context)
{

}

void CUrho3dGame::Start()
{
    // Execute base class startup
    CGameHandler::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CGameHandler::CreateUI();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    CGameHandler::InitMouseMode(MM_FREE);
}

void CUrho3dGame::CreateScene()
{
    m_pScene = new Scene(context_);

    // Create scene content on the server only
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Create octree and physics world with default settings. Create them as local so that they are not needlessly replicated
    // when a client connects
    m_pScene->CreateComponent<Octree>(LOCAL);
    m_pScene->CreateComponent<PhysicsWorld>(LOCAL);

    // All static scene content and the camera are also created as local, so that they are unaffected by scene replication and are
    // not removed from the client upon connection. Create a Zone component first for ambient lighting & fog control.
    Node* zoneNode = m_pScene->CreateChild("Zone", LOCAL);
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
    //zone->SetFogStart(100.0f);
    //zone->SetFogEnd(300.0f);

    // Create a directional light without shadows
    Node* lightNode = m_pScene->CreateChild("DirectionalLight", LOCAL);
    lightNode->SetDirection(Vector3(0.0f, -1.0f, 0.0f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(0.2f, 0.2f, 0.2f));
    light->SetSpecularIntensity(1.0f);

    // Create a "floor" consisting of several tiles. Make the tiles physical but leave small cracks between them
    for (int y = -10; y < 10; ++y)
    {
        for (int x = -10; x < 10; ++x)
        {
			//float k = 20.2f;
			float k = 1.0f;
			//float s = 20.0f;
			float s = 0.95f;
            Node* floorNode = m_pScene->CreateChild("FloorTile", LOCAL);
            floorNode->SetPosition(Vector3(x * k, -0.5f, y * k + 0.5f));
            floorNode->SetScale(Vector3(s, 1.0f, s));
            StaticModel* floorObject = floorNode->CreateComponent<StaticModel>();
            floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));


            RigidBody* body = floorNode->CreateComponent<RigidBody>();
            body->SetFriction(1.0f);
            CollisionShape* shape = floorNode->CreateComponent<CollisionShape>();
			shape->SetSphere(1.0f);

			// Init nad add items to array
			CItem pItem;
	
			pItem.floorObject         = floorObject;
			pItem.floorSphere.radius_ = 0.5f;
			pItem.floorSphere.center_ = Vector3(x * k, -0.5f, y * k + 0.5f);

			m_pItems.push_back(pItem);
        }
    }

    // Create the camera. Limit far clip distance to match the fog
    // The camera needs to be created into a local node so that each client can retain its own camera, that is unaffected by
    // network messages. Furthermore, because the client removes all replicated scene nodes when connecting to a server scene,
    // the screen would become blank if the camera node was replicated (as only the locally created camera is assigned to a
    // viewport in SetupViewports() below)

    m_pCameraNode = m_pScene->CreateChild("Camera", LOCAL);
    Camera* camera = m_pCameraNode->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    m_pCameraNode->SetPosition(Vector3(0.0f, 28.0f, 0.0f));
	m_pCameraNode->LookAt(Vector3(0.0f, 0.0f, 0.0f));
}

void CUrho3dGame::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, m_pScene, m_pCameraNode->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void CUrho3dGame::SubscribeToEvents()
{

    // Subscribe to button actions
    SubscribeToEvent(m_pButtonConnect, E_RELEASED, URHO3D_HANDLER(CUrho3dGame, HandleConnect));
    SubscribeToEvent(m_pButtonDisconnect, E_RELEASED, URHO3D_HANDLER(CUrho3dGame, HandleDisconnect));
    SubscribeToEvent(m_pButtonStartServer, E_RELEASED, URHO3D_HANDLER(CUrho3dGame, HandleStartServer));

    // Subscribe to network events
    SubscribeToEvent(E_SERVERCONNECTED, URHO3D_HANDLER(CUrho3dGame, HandleToServerConnected));
    SubscribeToEvent(E_SERVERDISCONNECTED, URHO3D_HANDLER(CUrho3dGame, HandleFromServerDisconnected));
    SubscribeToEvent(E_CONNECTFAILED, URHO3D_HANDLER(CUrho3dGame, HandleConnectFailed));

    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(CUrho3dGame, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(CUrho3dGame, HandleClientDisconnected));

	SubscribeToEvent(E_NETWORKMESSAGE, URHO3D_HANDLER(CUrho3dGame, HandleNetworkMessage));
}

void CUrho3dGame::HandleConnect(StringHash eventType, VariantMap& eventData)
{
    Network* network = GetSubsystem<Network>();
    String address   = m_pTextEdit->GetText().Trimmed();

    if (address.Empty())
        address = "localhost"; // Use localhost to connect if nothing else specified

    // Connect to server, specify scene to use as a client for replication
    m_nIdClient = 0; // Reset own object ID from possible previous connection
    bool nIsConnect = network->Connect(address, SERVER_PORT, m_pScene);

	if(nIsConnect)
	{
		m_pStateText->SetText("Connected success!");
		UpdateButtons();
	}
	else
	{
		m_pStateText->SetText("Error Connect");
	}
}

void CUrho3dGame::HandleDisconnect(StringHash eventType, VariantMap& eventData)
{
    Network* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();
    // If we were connected to server, disconnect. Or if we were running a server, stop it. In both cases clear the
    // scene of all replicated content, but let the local nodes & components (the static world + camera) stay
    if (serverConnection)
    {
        serverConnection->Disconnect();
        m_pScene->Clear(true, false);
        m_nIdClient = 0;
    }
    // Or if we were running a server, stop it
    else if (network->IsServerRunning())
    {
        network->StopServer();
        m_pScene->Clear(true, false);
		m_pStateText->SetText("Not connected");
    }

    UpdateButtons();
}

void CUrho3dGame::HandleStartServer(StringHash eventType, VariantMap& eventData)
{

    Network* network = GetSubsystem<Network>();
    bool nIsStarted = network->StartServer(SERVER_PORT);

	if(nIsStarted)
	{
		m_nIsServer = true;
		m_pStateText->SetText("Server started");
		UpdateButtons();
	}
	else
	{
		m_pStateText->SetText("Error: Server start");
	}
}

void CUrho3dGame::HandleToServerConnected(StringHash eventType, VariantMap& eventData)
{
    UpdateButtons();
}

void CUrho3dGame::HandleFromServerDisconnected(StringHash eventType, VariantMap& eventData)
{
	m_pStateText->SetText("Client disconnected");
    UpdateButtons();
}

void CUrho3dGame::HandleConnectFailed(StringHash eventType, VariantMap& eventData)
{
	m_pStateText->SetText("Connected failed");
    UpdateButtons();
}


void CUrho3dGame::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client connects, assign to scene to begin scene replication
    Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    newConnection->SetScene(m_pScene);

    // Find free IdClient
    unsigned nIdClient = GetIdClientFree();
	SetIdClientState(nIdClient, false);

	if(m_pServerObjects.Size() < 4)
	{
       m_pServerObjects[newConnection] = nIdClient;
	
	   int n = m_pServerObjects.Size();
	   char pBuf[64];
	   sprintf_s(pBuf, "Connected: %d client(c)", n);
	   m_pStateText->SetText(pBuf);
	}

    // Finally send the client ID
	
	CMsgIdClient msg;
	msg.nIdClient = nIdClient;
	newConnection->SendMessage(MSG_IDCLIENT, true, true, (const unsigned char* )&msg, sizeof(msg));


	// Initialize items for new client
	int i, nMax = m_pItems.size();

	for(i = 0; i < nMax; i++)
	{
	   if(m_pItems[i].nIdClient == NO_CLIENT)
		   continue;

	   CMsgUpdateItem msg;
	   msg.nIdClient = m_pItems[i].nIdClient;
	   msg.x         = m_pItems[i].x;
	   msg.y         = m_pItems[i].y;

	   newConnection->SendMessage(MSG_UPDATEITEM, true, true, (const unsigned char* )&msg, sizeof(msg));
	}
}

void CUrho3dGame::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client disconnects, remove the controlled object
    Connection* connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	SetIdClientState(m_pServerObjects[connection], true);
    m_pServerObjects.Erase(connection);

	int n = m_pServerObjects.Size();
	char pBuf[64];
	sprintf_s(pBuf, "Connected: %d client(c)", n);
	m_pStateText->SetText(pBuf);
}

void CUrho3dGame::EventClickItem(float x, float y)
{
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();

	if(!serverConnection || m_nIsServer)
		return;

	CMsgClickItem msg;
	msg.nIdClient = m_nIdClient;
	msg.x         = x;
	msg.y         = y;

	serverConnection->SendMessage(MSG_CLICKITEM, true, true, (const unsigned char* )&msg, sizeof(msg));
}


void CUrho3dGame::HandleNetworkMessage(StringHash eventType, VariantMap &eventData)
{
	switch(eventData[NetworkMessage::P_MESSAGEID].GetInt())
	{
	   case MSG_IDCLIENT:
	   {
		   CMsgIdClient *msg = (CMsgIdClient *)&eventData[NetworkMessage::P_DATA].GetBuffer()[0];
		    m_nIdClient = msg->nIdClient;

	       if(m_nIdClient == NO_CLIENT)
	       {
		      m_pStateText->SetText("Error connect. Server is busy.");
	       }
	       else
	       {
	          char pBuf[64];
	          sprintf_s(pBuf, "IdClient = %d", m_nIdClient);
	          m_pStateText->SetText(pBuf);
	       }
	   }
	   break;
	   case MSG_CLICKITEM:
	   {
		   CMsgClickItem *msg = (CMsgClickItem *)&eventData[NetworkMessage::P_DATA].GetBuffer()[0];
		   Network* network = GetSubsystem<Network>();

		   // Update server item
		   UpdateItem(msg->nIdClient, msg->x, msg->y);

		   // Send broadcast message for clients
		   CMsgUpdateItem upd;
		   upd.nIdClient = msg->nIdClient;
		   upd.x         = msg->x;
		   upd.y         = msg->y;

		   network->BroadcastMessage(MSG_UPDATEITEM, true, true, (const unsigned char* )&upd, sizeof(upd));
	   }
	   break;
	   case MSG_UPDATEITEM:
	   {
		  CMsgUpdateItem *msg = (CMsgUpdateItem *)&eventData[NetworkMessage::P_DATA].GetBuffer()[0];
		  UpdateItem(msg->nIdClient, msg->x, msg->y);
	   }
	   break;
	}
}
