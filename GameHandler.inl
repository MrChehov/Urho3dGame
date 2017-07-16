
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>

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
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include <Urho3D/DebugNew.h>



CGameHandler::CGameHandler(Context* context) : Application(context)
{
}

void CGameHandler::Setup()
{
	m_nIsServer = false;
	m_nIdClientFree[0] = true;
	m_nIdClientFree[1] = true;
	m_nIdClientFree[2] = true;
	m_nIdClientFree[3] = true;

    // Modify engine startup parameters
    engineParameters_["WindowTitle"] = GetTypeName();
    //engineParameters_["LogName"]     = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
    engineParameters_["FullScreen"]  = false;
    engineParameters_["Headless"]    = false;
    engineParameters_["Sound"]       = false;

    // Construct a search path to find the resource prefix with two entries:
    // The first entry is an empty path which will be substituted with program/bin directory -- this entry is for binary when it is still in build tree
    // The second and third entries are possible relative paths from the installed program/bin directory to the asset directory -- these entries are for binary when it is in the Urho3D SDK installation location
    if (!engineParameters_.Contains("ResourcePrefixPaths"))
        engineParameters_["ResourcePrefixPaths"] = ";../share/Resources;../share/Urho3D/Resources";
}

void CGameHandler::Start()
{
	SetWindowTitleAndIcon();

    // Subscribe mouse button down event
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(CGameHandler, HandleMouseButtonDown));
    // Subscribe key down event
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(CGameHandler, HandleKeyDown));
    // Subscribe key up event
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(CGameHandler, HandleKeyUp));
}

void CGameHandler::SetWindowTitleAndIcon()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Graphics* graphics = GetSubsystem<Graphics>();
    Image* icon = cache->GetResource<Image>("Textures/UrhoIcon.png");
    graphics->SetWindowIcon(icon);
    graphics->SetWindowTitle("Urho3D");
}

void CGameHandler::Stop()
{
	engine_->DumpResources(true);
	ExitProcess(0);
}

void CGameHandler::InitMouseMode(MouseMode mode)
{
    m_useMouseMode = mode;

    Input* input = GetSubsystem<Input>();

    if (GetPlatform() != "Web")
    {
        if (m_useMouseMode == MM_FREE)
            input->SetMouseVisible(true);

        Console* console = GetSubsystem<Console>();
        if (m_useMouseMode != MM_ABSOLUTE)
        {
            input->SetMouseMode(m_useMouseMode);
            if (console && console->IsVisible())
                input->SetMouseMode(MM_ABSOLUTE, true);
        }
    }
    else
    {
        input->SetMouseVisible(true);
        SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(CGameHandler, HandleMouseModeRequest));
        SubscribeToEvent(E_MOUSEMODECHANGED, URHO3D_HANDLER(CGameHandler, HandleMouseModeChange));
    }
}

// If the user clicks the canvas, attempt to switch to relative mouse mode on web platform
void CGameHandler::HandleMouseModeRequest(StringHash eventType, VariantMap& eventData)
{
    Console* console = GetSubsystem<Console>();
    if (console && console->IsVisible())
        return;
    Input* input = GetSubsystem<Input>();
    if (m_useMouseMode == MM_ABSOLUTE)
        input->SetMouseVisible(false);
    else if (m_useMouseMode == MM_FREE)
        input->SetMouseVisible(true);

    input->SetMouseMode(m_useMouseMode);
}

void CGameHandler::HandleMouseModeChange(StringHash eventType, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
    input->SetMouseVisible(!mouseLocked);
}


void CGameHandler::HandleKeyUp(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyUp;

    int key = eventData[P_KEY].GetInt();

    // Close console (if open) or exit when ESC is pressed
    if (key == KEY_ESCAPE)
    {
        Console* console = GetSubsystem<Console>();
        if (console && console->IsVisible())
            console->SetVisible(false);
        else
        {
            if (GetPlatform() == "Web")
            {
                GetSubsystem<Input>()->SetMouseVisible(true);
                if (m_useMouseMode != MM_ABSOLUTE)
                    GetSubsystem<Input>()->SetMouseMode(MM_FREE);
            }
            else
                engine_->Exit();
        }
    }
}

void CGameHandler::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;
    int key = eventData[P_KEY].GetInt();
}


void CGameHandler::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
	using namespace MouseButtonDown;
	int button = eventData[P_BUTTON].GetInt();

	if (button == MOUSEB_LEFT)
	{
        Input* input = GetSubsystem<Input>();
        Graphics* graphics = GetSubsystem<Graphics>();

        float x = (float)input->GetMousePosition().x_ / graphics->GetWidth();
        float y = (float)input->GetMousePosition().y_ / graphics->GetHeight();

		EventClickItem(x, y);
	}
}

void CGameHandler::CreateUI()
{

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();
    UIElement* root = ui->GetRoot();
    XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);


    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it can interact with the login UI
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto(uiStyle);
    ui->SetCursor(cursor);

    // Set starting position of the cursor at the rendering window center
    Graphics* graphics = GetSubsystem<Graphics>();
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

    // Construct the state text element
    m_pStateText = ui->GetRoot()->CreateChild<Text>();
    m_pStateText->SetText("Not connected");
    m_pStateText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    //Position the text relative to the screen center
    m_pStateText->SetHorizontalAlignment(HA_CENTER);
    m_pStateText->SetVerticalAlignment(VA_CENTER);
    m_pStateText->SetPosition(0, graphics->GetHeight() / 4);

    // Hide until connected
    //instructionsText_->SetVisible(false);

    m_pButtonContainer = root->CreateChild<UIElement>();
    m_pButtonContainer->SetFixedSize(500, 20);
    m_pButtonContainer->SetPosition(20, 20);
    m_pButtonContainer->SetLayoutMode(LM_HORIZONTAL);

    m_pTextEdit = m_pButtonContainer->CreateChild<LineEdit>();
    m_pTextEdit->SetStyleAuto();

    m_pButtonConnect     = CreateButton("Connect", 90);
    m_pButtonDisconnect  = CreateButton("Disconnect", 100);
    m_pButtonStartServer = CreateButton("Start Server", 110);

    UpdateButtons();
}

Button* CGameHandler::CreateButton(const String& text, int width)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    Button* button = m_pButtonContainer->CreateChild<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);

    Text* buttonText = button->CreateChild<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

void CGameHandler::UpdateButtons()
{
    Network* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();
    bool serverRunning = network->IsServerRunning();

    // Show and hide buttons so that eg. Connect and Disconnect are never shown at the same time
    m_pButtonConnect->SetVisible(!serverConnection && !serverRunning);
    m_pButtonDisconnect->SetVisible(serverConnection || serverRunning);
    m_pButtonStartServer->SetVisible(!serverConnection && !serverRunning);
    m_pTextEdit->SetVisible(!serverConnection && !serverRunning);
}

void CGameHandler::UpdateItem(unsigned nIdClient, float x, float y)
{
	static const char *pMaterials[4] = {"Materials/Mushroom.xml", "Materials/StoneEnvMap.xml", "Materials/Terrain.xml", "Materials/UrhoDecal.xml"};

	if(nIdClient >= MAX_CLIENTS)
		return;

	Camera *pCamera   = m_pCameraNode->GetComponent<Camera>();
	Ray     cameraRay = pCamera->GetScreenRay(x, y);

	int i, nMax = m_pItems.size();

	for(i = 0; i < nMax; i++)
	{
		float dist = cameraRay.HitDistance(m_pItems[i].floorSphere);

		if(_finite(dist))
		{
			ResourceCache* cache = GetSubsystem<ResourceCache>();
			m_pItems[i].floorObject->SetMaterial(cache->GetResource<Material>(pMaterials[nIdClient]));
			m_pItems[i].nIdClient = nIdClient;
			m_pItems[i].x = x;
			m_pItems[i].y = y;
			break;
		}
	}
}

unsigned CGameHandler::GetIdClientFree()
{
	if(m_nIdClientFree[0]) return(0);
	if(m_nIdClientFree[1]) return(1);
	if(m_nIdClientFree[2]) return(2);
	if(m_nIdClientFree[3]) return(3);

	return(NO_CLIENT);
}

void CGameHandler::SetIdClientState(unsigned nId, bool nIsFree)
{
	if(nId < MAX_CLIENTS)
		m_nIdClientFree[nId] = nIsFree;
}
