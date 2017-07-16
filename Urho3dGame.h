
#pragma once 

#include "GameHandler.h"
#include "Protocol.h"

#ifdef SendMessage
#undef SendMessage
#endif

class CUrho3dGame : public CGameHandler
{
	URHO3D_OBJECT(CUrho3dGame, CGameHandler);

public:
    /// Construct.
    CUrho3dGame(Context* context);

    /// Setup after engine initialization and before running the main loop.
    virtual void Start();

private:
	
	void  CreateScene();
	void  SetupViewport();

    /// Subscribe to update, UI and network events.
    void SubscribeToEvents();
    /// Handle pressing the connect button.
    void HandleConnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the disconnect button.
    void HandleDisconnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the start server button.
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Handle server connected
    void HandleToServerConnected(StringHash eventType, VariantMap& eventData);
	/// Handle server disconnected
	void HandleFromServerDisconnected(StringHash eventType, VariantMap& eventData);
	/// Handle connect failed
	void HandleConnectFailed(StringHash eventType, VariantMap& eventData);
     /// Handle a client connecting to the server.
    void HandleClientConnected(StringHash eventType, VariantMap& eventData);
    /// Handle a client disconnecting from the server.
    void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);
	// Event called when client click left button on the item
	void EventClickItem(float x, float y);
	// Handle networks messages. Look file Protocol.h
	void HandleNetworkMessage(StringHash eventType, VariantMap &eventData);

};