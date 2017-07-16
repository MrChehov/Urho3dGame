
#pragma once

#define URHO3D_STATIC_DEFINE
#define _CRT_SECURE_NO_WARNINGS

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>
#include <vector>

#pragma comment(lib, "Urho3d.lib")

namespace Urho3D
{

class Node;
class Scene;
class Sprite;

class Button;
class Connection;
class Scene;
class Text;
class UIElement;
}

using namespace Urho3D;

// UDP port we will use
static const unsigned short SERVER_PORT = 2345;
#define NO_CLIENT  -1
#define MAX_CLIENTS 4

class CGameHandler : public Application
{
    // Enable type information.
    URHO3D_OBJECT(CGameHandler, Application);

public:
	class CItem
	{
	public:		
		unsigned     nIdClient;
		StaticModel* floorObject;
		Sphere       floorSphere;
		float        x, y;
	public:
		CItem()
		{
			nIdClient = -1;
		}

	};
public:
    /// Construct.
    CGameHandler(Context* context);

	/// Setup before engine initialization. Modifies the engine parameters.
	virtual void Setup();
    /// Setup after engine initialization. Creates the logo, console & debug HUD.
    virtual void Start();
    /// Cleanup after the main loop. Called by Application.
    virtual void Stop();

protected:

    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    virtual String GetScreenJoystickPatchString() const { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

	void     SetWindowTitleAndIcon();
	void     InitMouseMode(MouseMode mode);
	void     CreateUI();
	void     UpdateItem(unsigned nIdClient, float x, float y);
	Button*  CreateButton(const String& text, int width);
	void     UpdateButtons();
	unsigned GetIdClientFree();
	void     SetIdClientState(unsigned nId, bool nIsFree);

	virtual void EventClickItem(float x, float y) = 0;


protected:

    /// Mouse mode option to use in the sample.
    MouseMode            m_useMouseMode;
	SharedPtr<Scene>     m_pScene;
	SharedPtr<Node>      m_pCameraNode;
	SharedPtr<Text>      m_pStateText;

	SharedPtr<UIElement> m_pButtonContainer;
	SharedPtr<LineEdit>  m_pTextEdit;
	SharedPtr<Button>    m_pButtonConnect;
	SharedPtr<Button>    m_pButtonDisconnect;
	SharedPtr<Button>    m_pButtonStartServer;
	unsigned             m_nIdClient;

	bool                 m_nIdClientFree[MAX_CLIENTS];

	/// Mapping from client connections to controllable objects.
    HashMap<Connection*, unsigned> m_pServerObjects;

	std::vector<CItem>   m_pItems;
	CItem                m_pItem;
	bool                 m_nIsServer;

private:

	void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);
    /// Handle request for mouse mode on web platform.
    void HandleMouseModeRequest(StringHash eventType, VariantMap& eventData);
    /// Handle request for mouse mode change on web platform.
    void HandleMouseModeChange(StringHash eventType, VariantMap& eventData);
    /// Handle key down event to process key controls common to all samples.
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    /// Handle key up event to process key controls common to all samples.
    void HandleKeyUp(StringHash eventType, VariantMap& eventData);

};

#include "GameHandler.inl"