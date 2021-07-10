/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation filefov


#include "EditorDefs.h"

#include "EditorViewportWidget.h"

// Qt
#include <QPainter>
#include <QScopedValueRollback>
#include <QCheckBox>
#include <QMessageBox>
#include <QTimer>
#include <QBoxLayout>

// AzCore
#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Console/IConsole.h>

// AzFramework
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Viewport/DisplayContextRequestBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#if defined(AZ_PLATFORM_WINDOWS)
#   include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>
#endif // defined(AZ_PLATFORM_WINDOWS)
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>                   // for AzFramework::InputDeviceMouse
#include <AzFramework/Viewport/ViewportControllerList.h>

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

// AtomToolsFramework
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>

// CryCommon
#include <CryCommon/HMDBus.h>

// AzFramework
#include <AzFramework/Render/IntersectorInterface.h>

// Editor
#include "Util/fastlib.h"
#include "CryEditDoc.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "Objects/DisplayContext.h"
#include "DisplaySettings.h"
#include "Include/IObjectManager.h"
#include "Include/IDisplayViewport.h"
#include "Objects/ObjectManager.h"
#include "ProcessInfo.h"
#include "IPostEffectGroup.h"
#include "EditorPreferencesPageGeneral.h"
#include "ViewportManipulatorController.h"
#include "LegacyViewportCameraController.h"
#include "EditorViewportSettings.h"

#include "ViewPane.h"
#include "CustomResolutionDlg.h"
#include "AnimationContext.h"
#include "Objects/SelectionGroup.h"
#include "Core/QtEditorApplication.h"

// ComponentEntityEditorPlugin
#include <Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h>

// LmbrCentral
#include <LmbrCentral/Rendering/EditorCameraCorrectionBus.h>

// Atom
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MatrixUtils.h>

#include <QtGui/private/qhighdpiscaling_p.h>

#include <IEntityRenderState.h>
#include <IPhysics.h>
#include <IStatObj.h>

AZ_CVAR(
    bool, ed_visibility_logTiming, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Output the timing of the new IVisibilitySystem query");
AZ_CVAR(bool, ed_useNewCameraSystem, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Use the new Editor camera system");

namespace SandboxEditor
{
    bool UsingNewCameraSystem()
    {
        return ed_useNewCameraSystem;
    }
} // namespace SandboxEditor

EditorViewportWidget* EditorViewportWidget::m_pPrimaryViewport = nullptr;

namespace AzFramework
{
    extern InputChannelId CameraFreeLookButton;
    extern InputChannelId CameraFreePanButton;
    extern InputChannelId CameraOrbitLookButton;
    extern InputChannelId CameraOrbitDollyButton;
    extern InputChannelId CameraOrbitPanButton;
} // namespace AzFramework

#if AZ_TRAIT_OS_PLATFORM_APPLE
void StopFixedCursorMode();
void StartFixedCursorMode(QObject *viewport);
#endif

#define RENDER_MESH_TEST_DISTANCE (0.2f)
#define CURSOR_FONT_HEIGHT  8.0f

//! Viewport settings for the EditorViewportWidget
struct EditorViewportSettings : public AzToolsFramework::ViewportInteraction::ViewportSettings
{
    bool GridSnappingEnabled() const override;
    float GridSize() const override;
    bool ShowGrid() const override;
    bool AngleSnappingEnabled() const override;
    float AngleStep() const override;
};

static const EditorViewportSettings g_EditorViewportSettings;

namespace AZ::ViewportHelpers
{
    static const char TextCantCreateCameraNoLevel[] = "Cannot create camera when no level is loaded.";

    class EditorEntityNotifications
        : public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
    public:
        EditorEntityNotifications(EditorViewportWidget& renderViewport)
            : m_renderViewport(renderViewport)
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        ~EditorEntityNotifications() override
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        }

        // AzToolsFramework::EditorEntityContextNotificationBus
        void OnStartPlayInEditor() override
        {
            m_renderViewport.OnStartPlayInEditor();
        }
        void OnStopPlayInEditor() override
        {
            m_renderViewport.OnStopPlayInEditor();
        }
    private:
        EditorViewportWidget& m_renderViewport;
    };
} // namespace AZ::ViewportHelpers

//////////////////////////////////////////////////////////////////////////
// EditorViewportWidget
//////////////////////////////////////////////////////////////////////////

EditorViewportWidget::EditorViewportWidget(const QString& name, QWidget* parent)
    : QtViewport(parent)
    , m_Camera(GetIEditor()->GetSystem()->GetViewCamera())
    , m_camFOV(gSettings.viewports.fDefaultFov)
    , m_defaultViewName(name)
    , m_renderViewport(nullptr) //m_renderViewport is initialized later, in SetViewportId
{
    // need this to be set in order to allow for language switching on Windows
    setAttribute(Qt::WA_InputMethodEnabled);
    LockCameraMovement(true);

    EditorViewportWidget::SetViewTM(m_Camera.GetMatrix());
    m_defaultViewTM.SetIdentity();

    if (GetIEditor()->GetViewManager()->GetSelectedViewport() == nullptr)
    {
        GetIEditor()->GetViewManager()->SelectViewport(this);
    }

    GetIEditor()->RegisterNotifyListener(this);

    m_displayContext.pIconManager = GetIEditor()->GetIconManager();
    GetIEditor()->GetUndoManager()->AddListener(this);

    m_PhysicalLocation.SetIdentity();

    // The renderer requires something, so don't allow us to shrink to absolutely nothing
    // This won't in fact stop the viewport from being shrunk, when it's the centralWidget for
    // the MainWindow, but it will stop the viewport from getting resize events
    // once it's smaller than that, which from the renderer's perspective works out
    // to be the same thing.
    setMinimumSize(50, 50);

    OnCreate();

    setMouseTracking(true);

    Camera::EditorCameraRequestBus::Handler::BusConnect();
    m_editorEntityNotifications = AZStd::make_unique<AZ::ViewportHelpers::EditorEntityNotifications>(*this);
    AzFramework::AssetCatalogEventBus::Handler::BusConnect();

    auto handleCameraChange = [this](const AZ::Matrix4x4&)
    {
        UpdateCameraFromViewportContext();
    };

    m_cameraViewMatrixChangeHandler = AZ::RPI::ViewportContext::MatrixChangedEvent::Handler(handleCameraChange);
    m_cameraProjectionMatrixChangeHandler = AZ::RPI::ViewportContext::MatrixChangedEvent::Handler(handleCameraChange);

    m_manipulatorManager = GetIEditor()->GetViewManager()->GetManipulatorManager();
    if (!m_pPrimaryViewport)
    {
        SetAsActiveViewport();
    }
}

//////////////////////////////////////////////////////////////////////////
EditorViewportWidget::~EditorViewportWidget()
{
    if (m_pPrimaryViewport == this)
    {
        m_pPrimaryViewport = nullptr;
    }

    DisconnectViewportInteractionRequestBus();
    m_editorEntityNotifications.reset();
    Camera::EditorCameraRequestBus::Handler::BusDisconnect();
    OnDestroy();
    GetIEditor()->GetUndoManager()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
// EditorViewportWidget message handlers
//////////////////////////////////////////////////////////////////////////
int EditorViewportWidget::OnCreate()
{
    CreateRenderContext();

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::resizeEvent(QResizeEvent* event)
{
    PushDisableRendering();
    QtViewport::resizeEvent(event);
    PopDisableRendering();

    const QRect rcWindow = rect().translated(mapToGlobal(QPoint()));

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE, rcWindow.left(), rcWindow.top());

    m_rcClient = rect();
    m_rcClient.setBottomRight(WidgetToViewport(m_rcClient.bottomRight()));

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE, width(), height());

    // We queue the window resize event because the render overlay may be hidden.
    // If the render overlay is not visible, the native window that is backing it will
    // also be hidden, and it will not resize until it becomes visible.
    m_windowResizedEvent = true;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    // Do not call CViewport::OnPaint() for painting messages
    // FIXME: paintEvent() isn't the best place for such logic. Should listen to proper eNotify events and to the stuff there instead. (Repeats for other view port classes too).
    CGameEngine* ge = GetIEditor()->GetGameEngine();
    if ((ge && ge->IsLevelLoaded()) || (GetType() != ET_ViewportCamera))
    {
        setRenderOverlayVisible(true);
    }
    else
    {
        setRenderOverlayVisible(false);
        QPainter painter(this); // device context for painting

        // draw gradient background
        const QRect rc = rect();
        QLinearGradient gradient(rc.topLeft(), rc.bottomLeft());
        gradient.setColorAt(0, QColor(80, 80, 80));
        gradient.setColorAt(1, QColor(200, 200, 200));
        painter.fillRect(rc, gradient);

        // if we have some level loaded/loading/new
        // we draw a text
        if (!GetIEditor()->GetLevelFolder().isEmpty())
        {
            const int kFontSize = 200;
            const char* kFontName = "Arial";
            const QColor kTextColor(255, 255, 255);
            const QColor kTextShadowColor(0, 0, 0);
            const QFont font(kFontName, kFontSize / 10.0);
            painter.setFont(font);

            QString friendlyName = QFileInfo(GetIEditor()->GetLevelName()).fileName();
            const QString strMsg = tr("Preparing level %1...").arg(friendlyName);

            // draw text shadow
            painter.setPen(kTextShadowColor);
            painter.drawText(rc, Qt::AlignCenter, strMsg);
            painter.setPen(kTextColor);
            // offset rect for normal text
            painter.drawText(rc.translated(-1, -1), Qt::AlignCenter, strMsg);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::mousePressEvent(QMouseEvent* event)
{
    GetIEditor()->GetViewManager()->SelectViewport(this);

    QtViewport::mousePressEvent(event);
}

AzToolsFramework::ViewportInteraction::MousePick EditorViewportWidget::BuildMousePickInternal(const QPoint& point) const
{
    using namespace AzToolsFramework::ViewportInteraction;

    MousePick mousePick;
    mousePick.m_screenCoordinates = ScreenPointFromQPoint(point);
    const auto& ray = m_renderViewport->ViewportScreenToWorldRay(mousePick.m_screenCoordinates);
    if (ray.has_value())
    {
        mousePick.m_rayOrigin = ray.value().origin;
        mousePick.m_rayDirection = ray.value().direction;
    }
    return mousePick;
}

AzToolsFramework::ViewportInteraction::MousePick EditorViewportWidget::BuildMousePick(const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;

    PreWidgetRendering();
    const MousePick mousePick = BuildMousePickInternal(point);
    PostWidgetRendering();
    return mousePick;
}

AzToolsFramework::ViewportInteraction::MouseInteraction EditorViewportWidget::BuildMouseInteractionInternal(
    const AzToolsFramework::ViewportInteraction::MouseButtons buttons,
    const AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers,
    const AzToolsFramework::ViewportInteraction::MousePick& mousePick) const
{
    using namespace AzToolsFramework::ViewportInteraction;

    MouseInteraction mouse;
    mouse.m_interactionId.m_cameraId = m_viewEntityId;
    mouse.m_interactionId.m_viewportId = GetViewportId();
    mouse.m_mouseButtons = buttons;
    mouse.m_mousePick = mousePick;
    mouse.m_keyboardModifiers = modifiers;
    return mouse;
}

AzToolsFramework::ViewportInteraction::MouseInteraction EditorViewportWidget::BuildMouseInteraction(
    const Qt::MouseButtons buttons, const Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;

    return BuildMouseInteractionInternal(
        BuildMouseButtons(buttons),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(WidgetToViewport(point)));
}

void EditorViewportWidget::InjectFakeMouseMove(int deltaX, int deltaY, Qt::MouseButtons buttons)
{
    // this is required, otherwise the user will see the context menu
    OnMouseMove(Qt::NoModifier, buttons, QCursor::pos() + QPoint(deltaX, deltaY));
    // we simply move the prev mouse position, so the change will be picked up
    // by the next ProcessMouse call
    m_prevMousePos -= QPoint(deltaX, deltaY);
}

//////////////////////////////////////////////////////////////////////////
bool  EditorViewportWidget::event(QEvent* event)
{
    switch (event->type())
    {
    case QEvent::WindowActivate:
        GetIEditor()->GetViewManager()->SelectViewport(this);
        // also kill the keys; if we alt-tab back to the viewport, or come back from the debugger, it's done (and there's no guarantee we'll get the keyrelease event anyways)
        m_keyDown.clear();
        break;

    case QEvent::Shortcut:
        // a shortcut should immediately clear us, otherwise the release event never gets sent
        m_keyDown.clear();
        break;
    }

    return QtViewport::event(event);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::ResetContent()
{
    QtViewport::ResetContent();
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::UpdateContent(int flags)
{
    QtViewport::UpdateContent(flags);
    if (flags & eUpdateObjects)
    {
        m_bUpdateViewport = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::Update()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    if (Editor::EditorQtApplication::instance()->isMovingOrResizing())
    {
        return;
    }

    if (m_rcClient.isEmpty() || GetIEditor()->IsInMatEditMode())
    {
        return;
    }

    if (!isVisible())
    {
        return;
    }

    if (m_updateCameraPositionNextTick)
    {
        auto cameraState = m_renderViewport->GetCameraState();
        AZ::Matrix3x4 matrix;
        matrix.SetBasisAndTranslation(cameraState.m_side, cameraState.m_forward, cameraState.m_up, cameraState.m_position);
        auto m = AZMatrix3x4ToLYMatrix3x4(matrix);

        SetViewTM(m);
        SetFOV(cameraState.m_fovOrZoom);
        m_Camera.SetZRange(cameraState.m_nearClip, cameraState.m_farClip);
    }

    // Reset the camera update flag now that we're finished updating our viewport context
    m_updateCameraPositionNextTick = false;

    // Don't wait for changes to update the focused viewport.
    if (CheckRespondToInput())
    {
        m_bUpdateViewport = true;
    }

    // While Renderer doesn't support fast rendering of the scene to more then 1 viewport
    // render only focused viewport if more then 1 are opened and always update is off.
    if (!m_isOnPaint && m_viewManager->GetNumberOfGameViewports() > 1 && GetType() == ET_ViewportCamera)
    {
        if (m_pPrimaryViewport != this)
        {
            if (CheckRespondToInput()) // If this is the focused window, set primary viewport.
            {
                SetAsActiveViewport();
            }
            else if (!m_bUpdateViewport) // Skip this viewport.
            {
                return;
            }
        }
    }

    const bool isGameMode = GetIEditor()->IsInGameMode();
    const bool isSimulationMode = GetIEditor()->GetGameEngine()->GetSimulationMode();

    // Allow debug visualization in both 'game' (Ctrl-G) and 'simulation' (Ctrl-P) modes
    if (isGameMode || isSimulationMode)
    {
        if (!IsRenderingDisabled())
        {
            // Disable rendering to avoid recursion into Update()
            PushDisableRendering();


            //get debug display interface for the viewport
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, GetViewportId());
            AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

            AzFramework::DebugDisplayRequests* debugDisplay =
                AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);


            // draw debug visualizations
            if (debugDisplay)
            {
                const AZ::u32 prevState = debugDisplay->GetState();
                debugDisplay->SetState(
                    e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOn | e_DepthTestOn);

                AzFramework::EntityDebugDisplayEventBus::Broadcast(
                    &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
                    AzFramework::ViewportInfo{ GetViewportId() }, *debugDisplay);

                debugDisplay->SetState(prevState);
            }

            QtViewport::Update();
            PopDisableRendering();
        }

        // Game mode rendering is handled by CryAction
        if (isGameMode)
        {
            return;
        }
    }

    // Prevents rendering recursion due to recursive Paint messages.
    if (IsRenderingDisabled())
    {
        return;
    }

    PushDisableRendering();

    m_viewTM = m_Camera.GetMatrix(); // synchronize.

    // Render
    {
        // TODO: Move out this logic to a controller and refactor to work with Atom

        OnRender();

        ProcessRenderLisneters(m_displayContext);

        m_displayContext.Flush2D();

        // m_renderer->SwitchToNativeResolutionBackbuffer();

        // 3D engine stats

        CCamera CurCamera = gEnv->pSystem->GetViewCamera();
        gEnv->pSystem->SetViewCamera(m_Camera);

        // Post Render Callback
        {
            PostRenderers::iterator itr = m_postRenderers.begin();
            PostRenderers::iterator end = m_postRenderers.end();
            for (; itr != end; ++itr)
            {
                (*itr)->OnPostRender();
            }
        }

        gEnv->pSystem->SetViewCamera(CurCamera);
    }

    {
        auto start = std::chrono::steady_clock::now();

        m_entityVisibilityQuery.UpdateVisibility(GetCameraState());

        if (ed_visibility_logTiming)
        {
            auto stop = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = stop - start;
            AZ_Printf("Visibility", "FindVisibleEntities (new) - Duration: %f", diff);
        }
    }

    QtViewport::Update();

    PopDisableRendering();
    m_bUpdateViewport = false;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetViewEntity(const AZ::EntityId& viewEntityId, bool lockCameraMovement)
{
    // if they've picked the same camera, then that means they want to toggle
    if (viewEntityId.IsValid() && viewEntityId != m_viewEntityId)
    {
        LockCameraMovement(lockCameraMovement);
        m_viewEntityId = viewEntityId;
        AZStd::string entityName;
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, viewEntityId);
        SetName(QString("Camera entity: %1").arg(entityName.c_str()));
    }
    else
    {
        SetDefaultCamera();
    }

    PostCameraSet();
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::ResetToViewSourceType(const ViewSourceType& viewSourceType)
{
    LockCameraMovement(true);
    m_viewEntityId.SetInvalid();
    m_cameraObjectId = GUID_NULL;
    m_viewSourceType = viewSourceType;
    SetViewTM(GetViewTM());
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::PostCameraSet()
{
    if (m_viewPane)
    {
        m_viewPane->OnFOVChanged(GetFOV());
    }

    GetIEditor()->Notify(eNotify_CameraChanged);
    QScopedValueRollback<bool> rb(m_ignoreSetViewFromEntityPerspective, true);
    Camera::EditorCameraNotificationBus::Broadcast(
        &Camera::EditorCameraNotificationBus::Events::OnViewportViewEntityChanged, m_viewEntityId);
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* EditorViewportWidget::GetCameraObject() const
{
    CBaseObject* pCameraObject = nullptr;

    if (m_viewSourceType == ViewSourceType::SequenceCamera)
    {
        m_cameraObjectId = GetViewManager()->GetCameraObjectId();
    }
    if (m_cameraObjectId != GUID_NULL)
    {
        // Find camera object from id.
        pCameraObject = GetIEditor()->GetObjectManager()->FindObject(m_cameraObjectId);
    }
    else if (m_viewSourceType == ViewSourceType::CameraComponent || m_viewSourceType == ViewSourceType::AZ_Entity)
    {
        AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(
            pCameraObject, m_viewEntityId, &AzToolsFramework::ComponentEntityEditorRequests::GetSandboxObject);
    }
    return pCameraObject;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
    {
        if (GetIEditor()->GetViewManager()->GetGameViewport() == this)
        {
            m_preGameModeViewTM = GetViewTM();
            // this should only occur for the main viewport and no others.
            ShowCursor();

            // If the user has selected game mode, enable outputting to any attached HMD and properly size the context
            // to the resolution specified by the VR device.
            if (gSettings.bEnableGameModeVR)
            {
                const AZ::VR::HMDDeviceInfo* deviceInfo = nullptr;
                EBUS_EVENT_RESULT(deviceInfo, AZ::VR::HMDDeviceRequestBus, GetDeviceInfo);
                AZ_Warning("Render Viewport", deviceInfo, "No VR device detected");

                if (deviceInfo)
                {
                    // Note: This may also need to adjust the viewport size
                    SetActiveWindow();
                    SetFocus();
                    SetSelected(true);
                }
            }
            SetCurrentCursor(STD_CURSOR_GAME);
        }

        if (m_renderViewport)
        {
            m_renderViewport->GetControllerList()->SetEnabled(false);
        }
    }
    break;

    case eNotify_OnEndGameMode:
        if (GetIEditor()->GetViewManager()->GetGameViewport() == this)
        {
            SetCurrentCursor(STD_CURSOR_DEFAULT);
            m_bInRotateMode = false;
            m_bInMoveMode = false;
            m_bInOrbitMode = false;
            m_bInZoomMode = false;

            RestoreViewportAfterGameMode();
        }

        if (m_renderViewport)
        {
            m_renderViewport->GetControllerList()->SetEnabled(true);
        }
        break;

    case eNotify_OnCloseScene:
        m_renderViewport->SetScene(nullptr);
        SetDefaultCamera();
        break;

    case eNotify_OnEndSceneOpen:
        UpdateScene();
        break;

    case eNotify_OnBeginNewScene:
        PushDisableRendering();
        break;

    case eNotify_OnEndNewScene:
        PopDisableRendering();

        {
            AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
            float sx = terrainAabb.GetXExtent();
            float sy = terrainAabb.GetYExtent();

            Matrix34 viewTM;
            viewTM.SetIdentity();
            // Initial camera will be at middle of the map at the height of 2
            // meters above the terrain (default terrain height is 32)
            viewTM.SetTranslation(Vec3(sx * 0.5f, sy * 0.5f, 34.0f));
            SetViewTM(viewTM);

            UpdateScene();
        }
        break;

    case eNotify_OnBeginTerrainCreate:
        PushDisableRendering();
        break;

    case eNotify_OnEndTerrainCreate:
        PopDisableRendering();

        {
            AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
            float sx = terrainAabb.GetXExtent();
            float sy = terrainAabb.GetYExtent();

            Matrix34 viewTM;
            viewTM.SetIdentity();
            // Initial camera will be at middle of the map at the height of 2
            // meters above the terrain (default terrain height is 32)
            viewTM.SetTranslation(Vec3(sx * 0.5f, sy * 0.5f, 34.0f));
            SetViewTM(viewTM);
        }
        break;

    case eNotify_OnBeginLayerExport:
    case eNotify_OnBeginSceneSave:
        PushDisableRendering();
        break;
    case eNotify_OnEndLayerExport:
    case eNotify_OnEndSceneSave:
        PopDisableRendering();
        break;

    case eNotify_OnBeginLoad: // disables viewport input when starting to load an existing level
    case eNotify_OnBeginCreate: // disables viewport input when starting to create a new level
        m_freezeViewportInput = true;
        break;

    case eNotify_OnEndLoad: // enables viewport input when finished loading an existing level
    case eNotify_OnEndCreate: // enables viewport input when finished creating a new level
        m_freezeViewportInput = false;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnRender()
{
    if (m_rcClient.isEmpty())
    {
        // Even in null rendering, update the view camera.
        // This is necessary so that automated editor tests using the null renderer to test systems like dynamic vegetation
        // are still able to manipulate the current logical camera position, even if nothing is rendered.
        GetIEditor()->GetSystem()->SetViewCamera(m_Camera);
        return;
    }

    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);
}

void EditorViewportWidget::OnBeginPrepareRender()
{
    if (!m_debugDisplay)
    {
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, GetViewportId());
        AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

        m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
    }

    if (!m_debugDisplay)
    {
        return;
    }

    m_isOnPaint = true;
    Update();
    m_isOnPaint = false;

    float fNearZ = GetIEditor()->GetConsoleVar("cl_DefaultNearPlane");
    float fFarZ = m_Camera.GetFarPlane();

    CBaseObject* cameraObject = GetCameraObject();
    if (cameraObject)
    {
        AZ::Matrix3x3 lookThroughEntityCorrection = AZ::Matrix3x3::CreateIdentity();
        if (m_viewEntityId.IsValid())
        {
            Camera::CameraRequestBus::EventResult(fNearZ, m_viewEntityId, &Camera::CameraComponentRequests::GetNearClipDistance);
            Camera::CameraRequestBus::EventResult(fFarZ, m_viewEntityId, &Camera::CameraComponentRequests::GetFarClipDistance);
            LmbrCentral::EditorCameraCorrectionRequestBus::EventResult(
                lookThroughEntityCorrection, m_viewEntityId, &LmbrCentral::EditorCameraCorrectionRequests::GetTransformCorrection);
        }

        m_viewTM = cameraObject->GetWorldTM() * AZMatrix3x3ToLYMatrix3x3(lookThroughEntityCorrection);
        m_viewTM.OrthonormalizeFast();

        m_Camera.SetMatrix(m_viewTM);

        int w = m_rcClient.width();
        int h = m_rcClient.height();

        m_Camera.SetFrustum(w, h, GetFOV(), fNearZ, fFarZ);
    }
    else if (m_viewEntityId.IsValid())
    {
        Camera::CameraRequestBus::EventResult(fNearZ, m_viewEntityId, &Camera::CameraComponentRequests::GetNearClipDistance);
        Camera::CameraRequestBus::EventResult(fFarZ, m_viewEntityId, &Camera::CameraComponentRequests::GetFarClipDistance);
        int w = m_rcClient.width();
        int h = m_rcClient.height();

        m_Camera.SetFrustum(w, h, GetFOV(), fNearZ, fFarZ);
    }
    else
    {
        // Normal camera.
        m_cameraObjectId = GUID_NULL;
        int w = m_rcClient.width();
        int h = m_rcClient.height();

        // Don't bother doing an FOV calculation if we don't have a valid viewport
        // This prevents frustum calculation bugs with a null viewport
        if (w <= 1 || h <= 1)
        {
            return;
        }

        float fov = gSettings.viewports.fDefaultFov;

        // match viewport fov to default / selected title menu fov
        if (GetFOV() != fov)
        {
            if (m_viewPane)
            {
                m_viewPane->OnFOVChanged(fov);
                SetFOV(fov);
            }
        }

        // Just for editor: Aspect ratio fix when changing the viewport
        if (!GetIEditor()->IsInGameMode())
        {
            float viewportAspectRatio = float( w ) / h;
            float targetAspectRatio = GetAspectRatio();
            if (targetAspectRatio > viewportAspectRatio)
            {
                // Correct for vertical FOV change.
                float maxTargetHeight = float( w ) / targetAspectRatio;
                fov = 2 * atanf((h * tan(fov / 2)) / maxTargetHeight);
            }
        }
        m_Camera.SetFrustum(w, h, fov, fNearZ);
    }

    GetIEditor()->GetSystem()->SetViewCamera(m_Camera);

    if (GetIEditor()->IsInGameMode())
    {
        return;
    }

    PreWidgetRendering();

    RenderAll();

    // Draw 2D helpers.
    TransformationMatrices backupSceneMatrices;
    m_debugDisplay->DepthTestOff();
    //m_renderer->Set2DMode(m_rcClient.right(), m_rcClient.bottom(), backupSceneMatrices);
    auto prevState = m_debugDisplay->GetState();
    m_debugDisplay->SetState(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOn | e_DepthTestOn);

    if (gSettings.viewports.bShowSafeFrame)
    {
        UpdateSafeFrame();
        RenderSafeFrame();
    }

    AzFramework::ViewportDebugDisplayEventBus::Event(
        AzToolsFramework::GetEntityContextId(), &AzFramework::ViewportDebugDisplayEvents::DisplayViewport2d,
        AzFramework::ViewportInfo{GetViewportId()}, *m_debugDisplay);

    m_debugDisplay->SetState(prevState);
    m_debugDisplay->DepthTestOn();

    PostWidgetRendering();
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::RenderAll()
{
    if (!m_debugDisplay)
    {
        return;
    }

    // allow the override of in-editor visualization
    AzFramework::ViewportDebugDisplayEventBus::Event(
        AzToolsFramework::GetEntityContextId(), &AzFramework::ViewportDebugDisplayEvents::DisplayViewport,
        AzFramework::ViewportInfo{ GetViewportId() }, *m_debugDisplay);

    m_entityVisibilityQuery.DisplayVisibility(*m_debugDisplay);

    if (m_manipulatorManager != nullptr)
    {
        using namespace AzToolsFramework::ViewportInteraction;

        m_debugDisplay->DepthTestOff();
        m_manipulatorManager->DrawManipulators(
            *m_debugDisplay, GetCameraState(),
            BuildMouseInteractionInternal(
                MouseButtons(TranslateMouseButtons(QGuiApplication::mouseButtons())),
                BuildKeyboardModifiers(QGuiApplication::queryKeyboardModifiers()),
                BuildMousePickInternal(WidgetToViewport(mapFromGlobal(QCursor::pos())))));
        m_debugDisplay->DepthTestOn();
    }
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::UpdateSafeFrame()
{
    m_safeFrame = m_rcClient;

    if (m_safeFrame.height() == 0)
    {
        return;
    }

    const bool allowSafeFrameBiggerThanViewport = false;

    float safeFrameAspectRatio = float( m_safeFrame.width()) / m_safeFrame.height();
    float targetAspectRatio = GetAspectRatio();
    bool viewportIsWiderThanSafeFrame = (targetAspectRatio <= safeFrameAspectRatio);
    if (viewportIsWiderThanSafeFrame || allowSafeFrameBiggerThanViewport)
    {
        float maxSafeFrameWidth = m_safeFrame.height() * targetAspectRatio;
        float widthDifference = m_safeFrame.width() - maxSafeFrameWidth;

        m_safeFrame.setLeft(m_safeFrame.left() + widthDifference * 0.5);
        m_safeFrame.setRight(m_safeFrame.right() - widthDifference * 0.5);
    }
    else
    {
        float maxSafeFrameHeight = m_safeFrame.width() / targetAspectRatio;
        float heightDifference = m_safeFrame.height() - maxSafeFrameHeight;

        m_safeFrame.setTop(m_safeFrame.top() + heightDifference * 0.5);
        m_safeFrame.setBottom(m_safeFrame.bottom() - heightDifference * 0.5);
    }

    m_safeFrame.adjust(0, 0, -1, -1); // <-- aesthetic improvement.

    const float SAFE_ACTION_SCALE_FACTOR = 0.05f;
    m_safeAction = m_safeFrame;
    m_safeAction.adjust(m_safeFrame.width() * SAFE_ACTION_SCALE_FACTOR, m_safeFrame.height() * SAFE_ACTION_SCALE_FACTOR,
        -m_safeFrame.width() * SAFE_ACTION_SCALE_FACTOR, -m_safeFrame.height() * SAFE_ACTION_SCALE_FACTOR);

    const float SAFE_TITLE_SCALE_FACTOR = 0.1f;
    m_safeTitle = m_safeFrame;
    m_safeTitle.adjust(m_safeFrame.width() * SAFE_TITLE_SCALE_FACTOR, m_safeFrame.height() * SAFE_TITLE_SCALE_FACTOR,
        -m_safeFrame.width() * SAFE_TITLE_SCALE_FACTOR, -m_safeFrame.height() * SAFE_TITLE_SCALE_FACTOR);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::RenderSafeFrame()
{
    RenderSafeFrame(m_safeFrame, 0.75f, 0.75f, 0, 0.8f);
    RenderSafeFrame(m_safeAction, 0, 0.85f, 0.80f, 0.8f);
    RenderSafeFrame(m_safeTitle, 0.80f, 0.60f, 0, 0.8f);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::RenderSafeFrame(const QRect& frame, float r, float g, float b, float a)
{
    m_debugDisplay->SetColor(r, g, b, a);

    const int LINE_WIDTH = 2;
    for (int i = 0; i < LINE_WIDTH; i++)
    {
        AZ::Vector3 topLeft(frame.left() + i, frame.top() + i, 0);
        AZ::Vector3 bottomRight(frame.right() - i, frame.bottom() - i, 0);
        m_debugDisplay->DrawWireBox(topLeft, bottomRight);
    }
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetAspectRatio() const
{
    return gSettings.viewports.fDefaultAspectRatio;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::RenderSnapMarker()
{
    if (!gSettings.snap.markerDisplay)
    {
        return;
    }

    QPoint point = QCursor::pos();
    ScreenToClient(point);
    Vec3 p = MapViewToCP(point);

    DisplayContext& dc = m_displayContext;

    float fScreenScaleFactor = GetScreenScaleFactor(p);

    Vec3 x(1, 0, 0);
    Vec3 y(0, 1, 0);
    Vec3 z(0, 0, 1);
    x = x * gSettings.snap.markerSize * fScreenScaleFactor * 0.1f;
    y = y * gSettings.snap.markerSize * fScreenScaleFactor * 0.1f;
    z = z * gSettings.snap.markerSize * fScreenScaleFactor * 0.1f;

    dc.SetColor(gSettings.snap.markerColor);
    dc.DrawLine(p - x, p + x);
    dc.DrawLine(p - y, p + y);
    dc.DrawLine(p - z, p + z);

    point = WorldToView(p);

    int s = 8;
    dc.DrawLine2d(point + QPoint(-s, -s), point + QPoint(s, -s), 0);
    dc.DrawLine2d(point + QPoint(-s, s), point + QPoint(s, s), 0);
    dc.DrawLine2d(point + QPoint(-s, -s), point + QPoint(-s, s), 0);
    dc.DrawLine2d(point + QPoint(s, -s), point + QPoint(s, s), 0);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnMenuResolutionCustom()
{
    CCustomResolutionDlg resDlg(width(), height(), parentWidget());
    if (resDlg.exec() == QDialog::Accepted)
    {
        ResizeView(resDlg.GetWidth(), resDlg.GetHeight());

        const QString text = QString::fromLatin1("%1 x %2").arg(resDlg.GetWidth()).arg(resDlg.GetHeight());

        QStringList customResPresets;
        CViewportTitleDlg::LoadCustomPresets("ResPresets", "ResPresetFor2ndView", customResPresets);
        CViewportTitleDlg::UpdateCustomPresets(text, customResPresets);
        CViewportTitleDlg::SaveCustomPresets("ResPresets", "ResPresetFor2ndView", customResPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnMenuCreateCameraEntityFromCurrentView()
{
    Camera::EditorCameraSystemRequestBus::Broadcast(&Camera::EditorCameraSystemRequests::CreateCameraEntityFromViewport);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnMenuSelectCurrentCamera()
{
    CBaseObject* pCameraObject = GetCameraObject();

    if (pCameraObject && !pCameraObject->IsSelected())
    {
        GetIEditor()->BeginUndo();
        IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();
        pObjectManager->ClearSelection();
        pObjectManager->SelectObject(pCameraObject);
        GetIEditor()->AcceptUndo("Select Current Camera");
    }
}

AzFramework::CameraState EditorViewportWidget::GetCameraState()
{
    return m_renderViewport->GetCameraState();
}

AZ::Vector3 EditorViewportWidget::PickTerrain(const AzFramework::ScreenPoint& point)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    return LYVec3ToAZVec3(ViewToWorld(AzToolsFramework::ViewportInteraction::QPointFromScreenPoint(point), nullptr, true));
}

AZ::EntityId EditorViewportWidget::PickEntity(const AzFramework::ScreenPoint& point)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    PreWidgetRendering();

    AZ::EntityId entityId;
    HitContext hitInfo;
    hitInfo.view = this;
    if (HitTest(AzToolsFramework::ViewportInteraction::QPointFromScreenPoint(point), hitInfo))
    {
        if (hitInfo.object && (hitInfo.object->GetType() == OBJTYPE_AZENTITY))
        {
            auto entityObject = static_cast<CComponentEntityObject*>(hitInfo.object);
            entityId = entityObject->GetAssociatedEntityId();
        }
    }

    PostWidgetRendering();

    return entityId;
}

float EditorViewportWidget::TerrainHeight(const AZ::Vector2& position)
{
    return GetIEditor()->GetTerrainElevation(position.GetX(), position.GetY());
}

void EditorViewportWidget::FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntitiesOut)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    visibleEntitiesOut.assign(m_entityVisibilityQuery.Begin(), m_entityVisibilityQuery.End());
}

AzFramework::ScreenPoint EditorViewportWidget::ViewportWorldToScreen(const AZ::Vector3& worldPosition)
{
    return m_renderViewport->ViewportWorldToScreen(worldPosition);
}

bool EditorViewportWidget::IsViewportInputFrozen()
{
    return m_freezeViewportInput;
}

void EditorViewportWidget::FreezeViewportInput(bool freeze)
{
    m_freezeViewportInput = freeze;
}

QWidget* EditorViewportWidget::GetWidgetForViewportContextMenu()
{
    return this;
}

void EditorViewportWidget::BeginWidgetContext()
{
    PreWidgetRendering();
}

void EditorViewportWidget::EndWidgetContext()
{
    PostWidgetRendering();
}

bool EditorViewportWidget::ShowingWorldSpace()
{
    using namespace AzToolsFramework::ViewportInteraction;
    return BuildKeyboardModifiers(QGuiApplication::queryKeyboardModifiers()).Shift();
}

void EditorViewportWidget::SetViewportId(int id)
{
    CViewport::SetViewportId(id);

    // Now that we have an ID, we can initialize our viewport.
    m_renderViewport = new AtomToolsFramework::RenderViewportWidget(this, false);
    if (!m_renderViewport->InitializeViewportContext(id))
    {
        AZ_Warning("EditorViewportWidget", false, "Failed to initialize RenderViewportWidget's ViewportContext");
        return;
    }
    auto viewportContext = m_renderViewport->GetViewportContext();
    m_defaultViewportContextName = viewportContext->GetName();
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom, this);
    layout->setContentsMargins(QMargins());
    layout->addWidget(m_renderViewport);

    viewportContext->ConnectViewMatrixChangedHandler(m_cameraViewMatrixChangeHandler);
    viewportContext->ConnectProjectionMatrixChangedHandler(m_cameraProjectionMatrixChangeHandler);

    m_renderViewport->GetControllerList()->Add(AZStd::make_shared<SandboxEditor::ViewportManipulatorController>());

    if (ed_useNewCameraSystem)
    {
        AzFramework::ReloadCameraKeyBindings();

        auto controller = AZStd::make_shared<AtomToolsFramework::ModularViewportCameraController>();
        controller->SetCameraListBuilderCallback(
            [id](AzFramework::Cameras& cameras)
            {
                auto firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::CameraFreeLookButton);
                auto firstPersonPanCamera =
                    AZStd::make_shared<AzFramework::PanCameraInput>(AzFramework::CameraFreePanButton, AzFramework::LookPan);
                auto firstPersonTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::LookTranslation);
                auto firstPersonWheelCamera = AZStd::make_shared<AzFramework::ScrollTranslationCameraInput>();

                auto orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>();
                orbitCamera->SetLookAtFn(
                    [id](const AZ::Vector3& position, const AZ::Vector3& direction) -> AZStd::optional<AZ::Vector3>
                    {
                        AZStd::optional<AZ::Vector3> lookAtAfterInterpolation;
                        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
                            lookAtAfterInterpolation, id,
                            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::LookAtAfterInterpolation);

                        // initially attempt to use the last set look at point after an interpolation has finished
                        if (lookAtAfterInterpolation.has_value())
                        {
                            return *lookAtAfterInterpolation;
                        }

                        const float RayDistance = 1000.0f;
                        AzFramework::RenderGeometry::RayRequest ray;
                        ray.m_startWorldPosition = position;
                        ray.m_endWorldPosition = position + direction * RayDistance;
                        ray.m_onlyVisible = true;

                        AzFramework::RenderGeometry::RayResult renderGeometryIntersectionResult;
                        AzFramework::RenderGeometry::IntersectorBus::EventResult(
                            renderGeometryIntersectionResult, AzToolsFramework::GetEntityContextId(),
                            &AzFramework::RenderGeometry::IntersectorInterface::RayIntersect, ray);

                        // attempt a ray intersection with any visible mesh and return the intersection position if successful
                        if (renderGeometryIntersectionResult)
                        {
                            return renderGeometryIntersectionResult.m_worldPosition;
                        }

                        // if there is no selection or no intersection, fallback to default camera orbit behavior (ground plane
                        // intersection)
                        return {};
                    });

                auto orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::CameraOrbitLookButton);
                auto orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation);
                auto orbitDollyWheelCamera = AZStd::make_shared<AzFramework::OrbitDollyScrollCameraInput>();
                auto orbitDollyMoveCamera =
                    AZStd::make_shared<AzFramework::OrbitDollyCursorMoveCameraInput>(AzFramework::CameraOrbitDollyButton);
                auto orbitPanCamera =
                    AZStd::make_shared<AzFramework::PanCameraInput>(AzFramework::CameraOrbitPanButton, AzFramework::OrbitPan);

                orbitCamera->m_orbitCameras.AddCamera(orbitRotateCamera);
                orbitCamera->m_orbitCameras.AddCamera(orbitTranslateCamera);
                orbitCamera->m_orbitCameras.AddCamera(orbitDollyWheelCamera);
                orbitCamera->m_orbitCameras.AddCamera(orbitDollyMoveCamera);
                orbitCamera->m_orbitCameras.AddCamera(orbitPanCamera);

                cameras.AddCamera(firstPersonRotateCamera);
                cameras.AddCamera(firstPersonPanCamera);
                cameras.AddCamera(firstPersonTranslateCamera);
                cameras.AddCamera(firstPersonWheelCamera);
                cameras.AddCamera(orbitCamera);
            });

        m_renderViewport->GetControllerList()->Add(controller);
    }
    else
    {
        m_renderViewport->GetControllerList()->Add(AZStd::make_shared<SandboxEditor::LegacyViewportCameraController>());
    }

    m_renderViewport->SetViewportSettings(&g_EditorViewportSettings);

    UpdateScene();

    if (m_pPrimaryViewport == this)
    {
        SetAsActiveViewport();
    }
}

void EditorViewportWidget::ConnectViewportInteractionRequestBus()
{
    AzToolsFramework::ViewportInteraction::ViewportFreezeRequestBus::Handler::BusConnect(GetViewportId());
    AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequestBus::Handler::BusConnect(GetViewportId());
    m_viewportUi.ConnectViewportUiBus(GetViewportId());

    AzFramework::InputSystemCursorConstraintRequestBus::Handler::BusConnect();
}

void EditorViewportWidget::DisconnectViewportInteractionRequestBus()
{
    AzFramework::InputSystemCursorConstraintRequestBus::Handler::BusDisconnect();

    m_viewportUi.DisconnectViewportUiBus();
    AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequestBus::Handler::BusDisconnect();
    AzToolsFramework::ViewportInteraction::ViewportFreezeRequestBus::Handler::BusDisconnect();
}

namespace AZ::ViewportHelpers
{
    void ToggleBool(bool* variable, bool* disableVariableIfOn)
    {
        *variable = !*variable;
        if (*variable && disableVariableIfOn)
        {
            *disableVariableIfOn = false;
        }
    }

    void ToggleInt(int* variable)
    {
        *variable = !*variable;
    }

    void AddCheckbox(QMenu* menu, const QString& text, bool* variable, bool* disableVariableIfOn = nullptr)
    {
        QAction* action = menu->addAction(text);
        QObject::connect(action, &QAction::triggered, action, [variable, disableVariableIfOn] { ToggleBool(variable, disableVariableIfOn);
            });
        action->setCheckable(true);
        action->setChecked(*variable);
    }

    void AddCheckbox(QMenu* menu, const QString& text, int* variable)
    {
        QAction* action = menu->addAction(text);
        QObject::connect(action, &QAction::triggered, action, [variable] { ToggleInt(variable);
            });
        action->setCheckable(true);
        action->setChecked(*variable);
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnTitleMenu(QMenu* menu)
{
    const bool bDisplayLabels = GetIEditor()->GetDisplaySettings()->IsDisplayLabels();
    QAction* action = menu->addAction(tr("Labels"));
    connect(action, &QAction::triggered, this, [bDisplayLabels] {GetIEditor()->GetDisplaySettings()->DisplayLabels(!bDisplayLabels);
        });
    action->setCheckable(true);
    action->setChecked(bDisplayLabels);

    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Safe Frame"), &gSettings.viewports.bShowSafeFrame);
    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Construction Plane"), &gSettings.snap.constructPlaneDisplay);
    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Trigger Bounds"), &gSettings.viewports.bShowTriggerBounds);
    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Icons"), &gSettings.viewports.bShowIcons, &gSettings.viewports.bShowSizeBasedIcons);
    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Size-based Icons"), &gSettings.viewports.bShowSizeBasedIcons, &gSettings.viewports.bShowIcons);
    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Helpers of Frozen Objects"), &gSettings.viewports.nShowFrozenHelpers);

    if (!m_predefinedAspectRatios.IsEmpty())
    {
        QMenu* aspectRatiosMenu = menu->addMenu(tr("Target Aspect Ratio"));

        for (size_t i = 0; i < m_predefinedAspectRatios.GetCount(); ++i)
        {
            const QString& aspectRatioString = m_predefinedAspectRatios.GetName(i);
            QAction* aspectRatioAction = aspectRatiosMenu->addAction(aspectRatioString);
            connect(aspectRatioAction, &QAction::triggered, this, [i, this] {
                const float aspect = m_predefinedAspectRatios.GetValue(i);
                gSettings.viewports.fDefaultAspectRatio = aspect;
            });
            aspectRatioAction->setCheckable(true);
            aspectRatioAction->setChecked(m_predefinedAspectRatios.IsCurrent(i));
        }
    }

    // Set ourself as the active viewport so the following actions create a camera from this view
    GetIEditor()->GetViewManager()->SelectViewport(this);

    CGameEngine* gameEngine = GetIEditor()->GetGameEngine();

    if (Camera::EditorCameraSystemRequestBus::HasHandlers())
    {
        action = menu->addAction(tr("Create camera entity from current view"));
        connect(action, &QAction::triggered, this, &EditorViewportWidget::OnMenuCreateCameraEntityFromCurrentView);

        if (!gameEngine || !gameEngine->IsLevelLoaded())
        {
            action->setEnabled(false);
            action->setToolTip(tr(AZ::ViewportHelpers::TextCantCreateCameraNoLevel));
            menu->setToolTipsVisible(true);
        }
    }

    if (!gameEngine || !gameEngine->IsLevelLoaded())
    {
        action->setEnabled(false);
        action->setToolTip(tr(AZ::ViewportHelpers::TextCantCreateCameraNoLevel));
        menu->setToolTipsVisible(true);
    }

    if (GetCameraObject())
    {
        action = menu->addAction(tr("Select Current Camera"));
        connect(action, &QAction::triggered, this, &EditorViewportWidget::OnMenuSelectCurrentCamera);
    }

    // Add Cameras.
    bool bHasCameras = AddCameraMenuItems(menu);
    EditorViewportWidget* pFloatingViewport = nullptr;

    if (GetIEditor()->GetViewManager()->GetViewCount() > 1)
    {
        for (int i = 0; i < GetIEditor()->GetViewManager()->GetViewCount(); ++i)
        {
            CViewport* vp = GetIEditor()->GetViewManager()->GetView(i);
            if (!vp)
            {
                continue;
            }

            if (viewport_cast<EditorViewportWidget*>(vp) == nullptr)
            {
                continue;
            }

            if (vp->GetViewportId() == MAX_NUM_VIEWPORTS - 1)
            {
                menu->addSeparator();

                QMenu* floatViewMenu = menu->addMenu(tr("Floating View"));

                pFloatingViewport = (EditorViewportWidget*)vp;
                pFloatingViewport->AddCameraMenuItems(floatViewMenu);

                if (bHasCameras)
                {
                    floatViewMenu->addSeparator();
                }

                QMenu* resolutionMenu = floatViewMenu->addMenu(tr("Resolution"));

                QStringList customResPresets;
                CViewportTitleDlg::LoadCustomPresets("ResPresets", "ResPresetFor2ndView", customResPresets);
                CViewportTitleDlg::AddResolutionMenus(resolutionMenu, [this](int width, int height) { ResizeView(width, height); }, customResPresets);
                if (!resolutionMenu->actions().isEmpty())
                {
                    resolutionMenu->addSeparator();
                }
                QAction* customResolutionAction = resolutionMenu->addAction(tr("Custom..."));
                connect(customResolutionAction, &QAction::triggered, this, &EditorViewportWidget::OnMenuResolutionCustom);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::AddCameraMenuItems(QMenu* menu)
{
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }

    AZ::ViewportHelpers::AddCheckbox(menu, "Lock Camera Movement", &m_bLockCameraMovement);
    menu->addSeparator();

    // Camera Sub menu
    QMenu* customCameraMenu = menu->addMenu(tr("Camera"));

    QAction* action = customCameraMenu->addAction("Editor Camera");
    action->setCheckable(true);
    action->setChecked(m_viewSourceType == ViewSourceType::None);
    connect(action, &QAction::triggered, this, &EditorViewportWidget::SetDefaultCamera);

    AZ::EBusAggregateResults<AZ::EntityId> getCameraResults;
    Camera::CameraBus::BroadcastResult(getCameraResults, &Camera::CameraRequests::GetCameras);

    const int numCameras = getCameraResults.values.size();

    // only enable if we're editing a sequence in Track View and have cameras in the level
    bool enableSequenceCameraMenu = (GetIEditor()->GetAnimation()->GetSequence() && numCameras);

    action = customCameraMenu->addAction(tr("Sequence Camera"));
    action->setCheckable(true);
    action->setChecked(m_viewSourceType == ViewSourceType::SequenceCamera);
    action->setEnabled(enableSequenceCameraMenu);
    connect(action, &QAction::triggered, this, &EditorViewportWidget::SetSequenceCamera);

    QVector<QAction*> additionalCameras;
    additionalCameras.reserve(getCameraResults.values.size());

    for (const AZ::EntityId& entityId : getCameraResults.values)
    {
        AZStd::string entityName;
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, entityId);
        action = new QAction(QString(entityName.c_str()), nullptr);
        additionalCameras.append(action);
        action->setCheckable(true);
        action->setChecked(m_viewEntityId == entityId && m_viewSourceType == ViewSourceType::CameraComponent);
        connect(action, &QAction::triggered, this, [this, entityId](bool isChecked)
            {
                if (isChecked)
                {
                    SetComponentCamera(entityId);
                }
                else
                {
                    SetDefaultCamera();
                }
            });
    }

    std::sort(additionalCameras.begin(), additionalCameras.end(), [] (QAction* a1, QAction* a2) {
        return QString::compare(a1->text(), a2->text(), Qt::CaseInsensitive) < 0;
    });

    for (QAction* cameraAction : additionalCameras)
    {
        customCameraMenu->addAction(cameraAction);
    }

    action = customCameraMenu->addAction(tr("Look through entity"));
    bool areAnyEntitiesSelected = false;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(areAnyEntitiesSelected, &AzToolsFramework::ToolsApplicationRequests::AreAnyEntitiesSelected);
    action->setCheckable(areAnyEntitiesSelected || m_viewSourceType == ViewSourceType::AZ_Entity);
    action->setEnabled(areAnyEntitiesSelected || m_viewSourceType == ViewSourceType::AZ_Entity);
    action->setChecked(m_viewSourceType == ViewSourceType::AZ_Entity);
    connect(action, &QAction::triggered, this, [this](bool isChecked)
        {
            if (isChecked)
            {
                AzToolsFramework::EntityIdList selectedEntityList;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
                if (selectedEntityList.size())
                {
                    SetEntityAsCamera(*selectedEntityList.begin());
                }
            }
            else
            {
                SetDefaultCamera();
            }
        });
    return true;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::ResizeView(int width, int height)
{
    const QRect rView = rect().translated(mapToGlobal(QPoint()));
    int deltaWidth = width - rView.width();
    int deltaHeight = height - rView.height();

    if (window()->isFullScreen())
    {
        setGeometry(rView.left(), rView.top(), rView.width() + deltaWidth, rView.height() + deltaHeight);
    }
    else
    {
        QWidget* window = this->window();
        if (window->isMaximized())
        {
            window->showNormal();
        }

        const QSize deltaSize = QSize(width, height) - size();
        window->move(0, 0);
        window->resize(window->size() + deltaSize);
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::ToggleCameraObject()
{
    if (m_viewSourceType == ViewSourceType::SequenceCamera)
    {
        ResetToViewSourceType(ViewSourceType::LegacyCamera);
    }
    else
    {
        ResetToViewSourceType(ViewSourceType::SequenceCamera);
    }
    PostCameraSet();
    GetIEditor()->GetAnimation()->ForceAnimation();
}


//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetCamera(const CCamera& camera)
{
    m_Camera = camera;
    SetViewTM(m_Camera.GetMatrix());
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetCameraMoveSpeed() const
{
    return gSettings.cameraMoveSpeed;
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetCameraRotateSpeed() const
{
    return gSettings.cameraRotateSpeed;
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::GetCameraInvertYRotation() const
{
    return gSettings.invertYRotation;
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetCameraInvertPan() const
{
    return gSettings.invertPan;
}

//////////////////////////////////////////////////////////////////////////
EditorViewportWidget* EditorViewportWidget::GetPrimaryViewport()
{
    return m_pPrimaryViewport;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::focusOutEvent([[maybe_unused]] QFocusEvent* event)
{
    // if we lose focus, the keyboard map needs to be cleared immediately
    if (!m_keyDown.isEmpty())
    {
        m_keyDown.clear();

        releaseKeyboard();
    }
}

void EditorViewportWidget::keyPressEvent(QKeyEvent* event)
{
    // Special case Escape key and bubble way up to the top level parent so that it can cancel us out of any active tool
    // or clear the current selection
    if (event->key() == Qt::Key_Escape)
    {
        QCoreApplication::sendEvent(GetIEditor()->GetEditorMainWindow(), event);
    }

    // NOTE: we keep track of key presses and releases explicitly because the OS/Qt will insert a slight delay between sending
    // key events when the key is held down. This is standard, but makes responding to key events for game style input silly
    // because we want the movement to be butter smooth.
    if (!event->isAutoRepeat())
    {
        m_keyDown.insert(event->key());
    }

    QtViewport::keyPressEvent(event);

#if defined(AZ_PLATFORM_WINDOWS)
    // In game mode on windows we need to forward raw text events to the input system.
    if (GetIEditor()->IsInGameMode() && GetType() == ET_ViewportCamera)
    {
        // Get the QString as a '\0'-terminated array of unsigned shorts.
        // The result remains valid until the string is modified.
        const ushort* codeUnitsUTF16 = event->text().utf16();
        while (ushort codeUnitUTF16 = *codeUnitsUTF16)
        {
            AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputCodeUnitUTF16Event, codeUnitUTF16);
            ++codeUnitsUTF16;
        }
    }
#endif // defined(AZ_PLATFORM_WINDOWS)
}

void EditorViewportWidget::SetViewTM(const Matrix34& viewTM, bool bMoveOnly)
{
    Matrix34 camMatrix = viewTM;

    // If no collision flag set do not check for terrain elevation.
    if (GetType() == ET_ViewportCamera)
    {
        if ((GetIEditor()->GetDisplaySettings()->GetSettings() & SETTINGS_NOCOLLISION) == 0)
        {
            Vec3 p = camMatrix.GetTranslation();
            bool adjustCameraElevation = true;
            auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
            if (terrain)
            {
                AZ::Aabb terrainAabb(terrain->GetTerrainAabb());

                // Adjust the AABB to include all Z values.  Since the goal here is to snap the camera to the terrain height if
                // it's below the terrain, we only want to verify the camera is within the XY bounds of the terrain to adjust the elevation.
                terrainAabb.SetMin(AZ::Vector3(terrainAabb.GetMin().GetX(), terrainAabb.GetMin().GetY(), -AZ::Constants::FloatMax));
                terrainAabb.SetMax(AZ::Vector3(terrainAabb.GetMax().GetX(), terrainAabb.GetMax().GetY(), AZ::Constants::FloatMax));

                if (!terrainAabb.Contains(LYVec3ToAZVec3(p)))
                {
                    adjustCameraElevation = false;
                }
                else if (terrain->GetIsHoleFromFloats(p.x, p.y))
                {
                    adjustCameraElevation = false;
                }
            }

            if (adjustCameraElevation)
            {
                float z = GetIEditor()->GetTerrainElevation(p.x, p.y);
                if (p.z < z + 0.25)
                {
                    p.z = z + 0.25;
                    camMatrix.SetTranslation(p);
                }
            }
        }

        // Also force this position on game.
        if (GetIEditor()->GetGameEngine())
        {
            GetIEditor()->GetGameEngine()->SetPlayerViewMatrix(viewTM);
        }
    }

    CBaseObject* cameraObject = GetCameraObject();
    if (cameraObject)
    {
        // Ignore camera movement if locked.
        if (IsCameraMovementLocked() || (!GetIEditor()->GetAnimation()->IsRecordMode() && !IsCameraObjectMove()))
        {
            return;
        }

        AZ::Matrix3x3 lookThroughEntityCorrection = AZ::Matrix3x3::CreateIdentity();
        if (m_viewEntityId.IsValid())
        {
            LmbrCentral::EditorCameraCorrectionRequestBus::EventResult(
                lookThroughEntityCorrection, m_viewEntityId,
                &LmbrCentral::EditorCameraCorrectionRequests::GetInverseTransformCorrection);
        }

        if (m_pressedKeyState != KeyPressedState::PressedInPreviousFrame)
        {
            CUndo undo("Move Camera");
            if (bMoveOnly)
            {
                // specify eObjectUpdateFlags_UserInput so that an undo command gets logged
                cameraObject->SetWorldPos(camMatrix.GetTranslation(), eObjectUpdateFlags_UserInput);
            }
            else
            {
                // specify eObjectUpdateFlags_UserInput so that an undo command gets logged
                cameraObject->SetWorldTM(camMatrix * AZMatrix3x3ToLYMatrix3x3(lookThroughEntityCorrection), eObjectUpdateFlags_UserInput);
            }
        }
        else
        {
            if (bMoveOnly)
            {
                // Do not specify eObjectUpdateFlags_UserInput, so that an undo command does not get logged; we covered it already when m_pressedKeyState was PressedThisFrame
                cameraObject->SetWorldPos(camMatrix.GetTranslation());
            }
            else
            {
                // Do not specify eObjectUpdateFlags_UserInput, so that an undo command does not get logged; we covered it already when m_pressedKeyState was PressedThisFrame
                cameraObject->SetWorldTM(camMatrix * AZMatrix3x3ToLYMatrix3x3(lookThroughEntityCorrection));
            }
        }
    }
    else if (m_viewEntityId.IsValid())
    {
        // Ignore camera movement if locked.
        if (IsCameraMovementLocked() || (!GetIEditor()->GetAnimation()->IsRecordMode() && !IsCameraObjectMove()))
        {
            return;
        }

        if (m_pressedKeyState != KeyPressedState::PressedInPreviousFrame)
        {
            CUndo undo("Move Camera");
            if (bMoveOnly)
            {
                AZ::TransformBus::Event(
                    m_viewEntityId, &AZ::TransformInterface::SetWorldTranslation,
                    LYVec3ToAZVec3(camMatrix.GetTranslation()));
            }
            else
            {
                AZ::TransformBus::Event(
                    m_viewEntityId, &AZ::TransformInterface::SetWorldTM,
                    LYTransformToAZTransform(camMatrix));
            }
        }
        else
        {
            if (bMoveOnly)
            {
                AZ::TransformBus::Event(
                    m_viewEntityId, &AZ::TransformInterface::SetWorldTranslation,
                    LYVec3ToAZVec3(camMatrix.GetTranslation()));
            }
            else
            {
                AZ::TransformBus::Event(
                    m_viewEntityId, &AZ::TransformInterface::SetWorldTM,
                    LYTransformToAZTransform(camMatrix));
            }
        }

        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    if (m_pressedKeyState == KeyPressedState::PressedThisFrame)
    {
        m_pressedKeyState = KeyPressedState::PressedInPreviousFrame;
    }

    QtViewport::SetViewTM(camMatrix);

    m_Camera.SetMatrix(camMatrix);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::RenderSelectedRegion()
{
    AABB box;
    GetIEditor()->GetSelectedRegion(box);
    if (box.IsEmpty())
    {
        return;
    }

    float x1 = box.min.x;
    float y1 = box.min.y;
    float x2 = box.max.x;
    float y2 = box.max.y;

    DisplayContext& dc = m_displayContext;

    float fMaxSide = MAX(y2 - y1, x2 - x1);
    if (fMaxSide < 0.1f)
    {
        return;
    }
    float fStep = fMaxSide / 100.0f;

    float fMinZ = 0;
    float fMaxZ = 0;

    // Draw yellow border lines.
    dc.SetColor(1, 1, 0, 1);
    float offset = 0.01f;
    Vec3 p1, p2;

    const float defaultTerrainHeight = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
    auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();

    for (float y = y1; y < y2; y += fStep)
    {
        p1.x = x1;
        p1.y = y;
        p1.z = terrain ? terrain->GetHeightFromFloats(p1.x, p1.y) + offset : defaultTerrainHeight + offset;

        p2.x = x1;
        p2.y = y + fStep;
        p2.z = terrain ? terrain->GetHeightFromFloats(p2.x, p2.y) + offset : defaultTerrainHeight + offset;
        dc.DrawLine(p1, p2);

        p1.x = x2;
        p1.y = y;
        p1.z = terrain ? terrain->GetHeightFromFloats(p1.x, p1.y) + offset : defaultTerrainHeight + offset;

        p2.x = x2;
        p2.y = y + fStep;
        p2.z = terrain ? terrain->GetHeightFromFloats(p2.x, p2.y) + offset : defaultTerrainHeight + offset;
        dc.DrawLine(p1, p2);

        fMinZ = min(fMinZ, min(p1.z, p2.z));
        fMaxZ = max(fMaxZ, max(p1.z, p2.z));
    }
    for (float x = x1; x < x2; x += fStep)
    {
        p1.x = x;
        p1.y = y1;
        p1.z = terrain ? terrain->GetHeightFromFloats(p1.x, p1.y) + offset : defaultTerrainHeight + offset;

        p2.x = x + fStep;
        p2.y = y1;
        p2.z = terrain ? terrain->GetHeightFromFloats(p2.x, p2.y) + offset : defaultTerrainHeight + offset;
        dc.DrawLine(p1, p2);

        p1.x = x;
        p1.y = y2;
        p1.z = terrain ? terrain->GetHeightFromFloats(p1.x, p1.y) + offset : defaultTerrainHeight + offset;

        p2.x = x + fStep;
        p2.y = y2;
        p2.z = terrain ? terrain->GetHeightFromFloats(p2.x, p2.y) + offset : defaultTerrainHeight + offset;
        dc.DrawLine(p1, p2);

        fMinZ = min(fMinZ, min(p1.z, p2.z));
        fMaxZ = max(fMaxZ, max(p1.z, p2.z));
    }

    {
        // Draw a box area
        float fBoxOver = fMaxSide / 5.0f;
        float fBoxHeight = fBoxOver + fMaxZ - fMinZ;

        ColorB boxColor(64, 64, 255, 128); // light blue
        ColorB transparent(boxColor.r, boxColor.g, boxColor.b, 0);

        Vec3 base[] = {
            Vec3(x1, y1, fMinZ),
            Vec3(x2, y1, fMinZ),
            Vec3(x2, y2, fMinZ),
            Vec3(x1, y2, fMinZ)
        };


        // Generate vertices
        static AABB boxPrev(AABB::RESET);
        static std::vector<Vec3> verts;
        static std::vector<ColorB> colors;

        if (!IsEquivalent(boxPrev, box))
        {
            verts.resize(0);
            colors.resize(0);
            for (int i = 0; i < 4; ++i)
            {
                Vec3& p = base[i];

                verts.push_back(p);
                verts.push_back(Vec3(p.x, p.y, p.z + fBoxHeight));
                verts.push_back(Vec3(p.x, p.y, p.z + fBoxHeight + fBoxOver));

                colors.push_back(boxColor);
                colors.push_back(boxColor);
                colors.push_back(transparent);
            }
            boxPrev = box;
        }

        // Generate indices
        const int numInds = 4 * 12;
        static vtx_idx inds[numInds];
        static bool bNeedIndsInit = true;
        if (bNeedIndsInit)
        {
            vtx_idx* pInds = &inds[0];

            for (int i = 0; i < 4; ++i)
            {
                int over = 0;
                if (i == 3)
                {
                    over = -12;
                }

                int ind = i * 3;
                *pInds++ = ind;
                *pInds++ = ind + 3 + over;
                *pInds++ = ind + 1;

                *pInds++ = ind + 1;
                *pInds++ = ind + 3 + over;
                *pInds++ = ind + 4 + over;

                ind = i * 3 + 1;
                *pInds++ = ind;
                *pInds++ = ind + 3 + over;
                *pInds++ = ind + 1;

                *pInds++ = ind + 1;
                *pInds++ = ind + 3 + over;
                *pInds++ = ind + 4 + over;
            }
            bNeedIndsInit = false;
        }

        // Draw lines
        for (int i = 0; i < 4; ++i)
        {
            Vec3& p = base[i];

            dc.DrawLine(p, Vec3(p.x, p.y, p.z + fBoxHeight), ColorF(1, 1, 0, 1), ColorF(1, 1, 0, 1));
            dc.DrawLine(Vec3(p.x, p.y, p.z + fBoxHeight), Vec3(p.x, p.y, p.z + fBoxHeight + fBoxOver), ColorF(1, 1, 0, 1), ColorF(1, 1, 0, 0));
        }

        // Draw volume
        dc.DepthWriteOff();
        dc.CullOff();
        dc.pRenderAuxGeom->DrawTriangles(&verts[0], verts.size(), &inds[0], numInds, &colors[0]);
        dc.CullOn();
        dc.DepthWriteOn();
    }
}

Vec3 EditorViewportWidget::WorldToView3D(const Vec3& wp, [[maybe_unused]] int nFlags) const
{
    Vec3 out(0, 0, 0);
    float x, y, z;

    ProjectToScreen(wp.x, wp.y, wp.z, &x, &y, &z);
    if (_finite(x) && _finite(y) && _finite(z))
    {
        out.x = (x / 100) * m_rcClient.width();
        out.y = (y / 100) * m_rcClient.height();
        out.x /= QHighDpiScaling::factor(windowHandle()->screen());
        out.y /= QHighDpiScaling::factor(windowHandle()->screen());
        out.z = z;
    }
    return out;
}

//////////////////////////////////////////////////////////////////////////
QPoint EditorViewportWidget::WorldToView(const Vec3& wp) const
{
    return AzToolsFramework::ViewportInteraction::QPointFromScreenPoint(m_renderViewport->ViewportWorldToScreen(LYVec3ToAZVec3(wp)));
}
//////////////////////////////////////////////////////////////////////////
QPoint EditorViewportWidget::WorldToViewParticleEditor(const Vec3& wp, int width, int height) const
{
    QPoint p;
    float x, y, z;

    ProjectToScreen(wp.x, wp.y, wp.z, &x, &y, &z);
    if (_finite(x) || _finite(y))
    {
        p.rx() = (x / 100) * width;
        p.ry() = (y / 100) * height;
    }
    else
    {
        QPoint(0, 0);
    }
    return p;
}

//////////////////////////////////////////////////////////////////////////
Vec3 EditorViewportWidget::ViewToWorld(
    const QPoint& vp, bool* collideWithTerrain, bool onlyTerrain, bool bSkipVegetation, bool bTestRenderMesh, bool* collideWithObject) const
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AZ_UNUSED(collideWithTerrain)
    AZ_UNUSED(onlyTerrain)
    AZ_UNUSED(bTestRenderMesh)
    AZ_UNUSED(bSkipVegetation)
    AZ_UNUSED(bSkipVegetation)
    AZ_UNUSED(collideWithObject)

    auto ray = m_renderViewport->ViewportScreenToWorldRay(AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(vp));
    if (!ray.has_value())
    {
        return Vec3(0, 0, 0);
    }

    const float maxDistance = 10000.f;
    Vec3 v = AZVec3ToLYVec3(ray.value().direction) * maxDistance;

    if (!_finite(v.x) || !_finite(v.y) || !_finite(v.z))
    {
        return Vec3(0, 0, 0);
    }

    Vec3 colp = AZVec3ToLYVec3(ray.value().origin) + 0.002f * v;

    return colp;
}

//////////////////////////////////////////////////////////////////////////
Vec3 EditorViewportWidget::ViewToWorldNormal(const QPoint& vp, bool onlyTerrain, bool bTestRenderMesh)
{
    AZ_UNUSED(vp)
    AZ_UNUSED(onlyTerrain)
    AZ_UNUSED(bTestRenderMesh)

    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    return Vec3(0, 0, 1);
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::AdjustObjectPosition(const ray_hit& hit, Vec3& outNormal, Vec3& outPos) const
{
    Matrix34A objMat, objMatInv;
    Matrix33 objRot, objRotInv;

    if (hit.pCollider->GetiForeignData() != PHYS_FOREIGN_ID_STATIC)
    {
        return false;
    }

    IRenderNode* pNode = (IRenderNode*) hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC);
    if (!pNode || !pNode->GetEntityStatObj())
    {
        return false;
    }

    IStatObj* pEntObject  = pNode->GetEntityStatObj(hit.partid, 0, &objMat, false);
    if (!pEntObject || !pEntObject->GetRenderMesh())
    {
        return false;
    }

    objRot = Matrix33(objMat);
    objRot.NoScale(); // No scale.
    objRotInv = objRot;
    objRotInv.Invert();

    float fWorldScale = objMat.GetColumn(0).GetLength(); // GetScale
    float fWorldScaleInv = 1.0f / fWorldScale;

    // transform decal into object space
    objMatInv = objMat;
    objMatInv.Invert();

    // put into normal object space hit direction of projection
    Vec3 invhitn = -(hit.n);
    Vec3 vOS_HitDir = objRotInv.TransformVector(invhitn).GetNormalized();

    // put into position object space hit position
    Vec3 vOS_HitPos = objMatInv.TransformPoint(hit.pt);
    vOS_HitPos -= vOS_HitDir * RENDER_MESH_TEST_DISTANCE * fWorldScaleInv;

    IRenderMesh* pRM = pEntObject->GetRenderMesh();

    AABB aabbRNode;
    pRM->GetBBox(aabbRNode.min, aabbRNode.max);
    Vec3 vOut(0, 0, 0);
    if (!Intersect::Ray_AABB(Ray(vOS_HitPos, vOS_HitDir), aabbRNode, vOut))
    {
        return false;
    }

    if (!pRM || !pRM->GetVerticesCount())
    {
        return false;
    }

    if (RayRenderMeshIntersection(pRM, vOS_HitPos, vOS_HitDir, outPos, outNormal))
    {
        outNormal = objRot.TransformVector(outNormal).GetNormalized();
        outPos = objMat.TransformPoint(outPos);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::RayRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal) const
{
    AZ_UNUSED(pRenderMesh);
    AZ_UNUSED(vInPos);
    AZ_UNUSED(vInDir);
    AZ_UNUSED(vOutPos);
    AZ_UNUSED(vOutNormal);
    return false;
    /*SRayHitInfo hitInfo;
    hitInfo.bUseCache = false;
    hitInfo.bInFirstHit = false;
    hitInfo.inRay.origin = vInPos;
    hitInfo.inRay.direction = vInDir.GetNormalized();
    hitInfo.inReferencePoint = vInPos;
    hitInfo.fMaxHitDistance = 0;
    bool bRes = ???->RenderMeshRayIntersection(pRenderMesh, hitInfo, nullptr);
    vOutPos = hitInfo.vHitPos;
    vOutNormal = hitInfo.vHitNormal;
    return bRes;*/
}

void EditorViewportWidget::UnProjectFromScreen(float sx, float sy, float sz, float* px, float* py, float* pz) const
{
    AZ::Vector3 wp;
    wp = m_renderViewport->ViewportScreenToWorld(AzFramework::ScreenPoint{(int)sx, m_rcClient.bottom() - ((int)sy)}, sz).value_or(wp);
    *px = wp.GetX();
    *py = wp.GetY();
    *pz = wp.GetZ();
}

void EditorViewportWidget::ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy, float* sz) const
{
    AzFramework::ScreenPoint screenPosition = m_renderViewport->ViewportWorldToScreen(AZ::Vector3{ptx, pty, ptz});
    *sx = screenPosition.m_x;
    *sy = screenPosition.m_y;
    *sz = 0.f;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const
{
    QRect rc = m_rcClient;

    Vec3 pos0, pos1;
    float wx, wy, wz;
    UnProjectFromScreen(vp.x(), rc.bottom() - vp.y(), 0, &wx, &wy, &wz);
    if (!_finite(wx) || !_finite(wy) || !_finite(wz))
    {
        return;
    }
    if (fabs(wx) > 1000000 || fabs(wy) > 1000000 || fabs(wz) > 1000000)
    {
        return;
    }
    pos0(wx, wy, wz);
    UnProjectFromScreen(vp.x(), rc.bottom() - vp.y(), 1, &wx, &wy, &wz);
    if (!_finite(wx) || !_finite(wy) || !_finite(wz))
    {
        return;
    }
    if (fabs(wx) > 1000000 || fabs(wy) > 1000000 || fabs(wz) > 1000000)
    {
        return;
    }
    pos1(wx, wy, wz);

    Vec3 v = (pos1 - pos0);
    v = v.GetNormalized();

    raySrc = pos0;
    rayDir = v;
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetScreenScaleFactor(const Vec3& worldPoint) const
{
    float dist = m_Camera.GetPosition().GetDistance(worldPoint);
    if (dist < m_Camera.GetNearPlane())
    {
        dist = m_Camera.GetNearPlane();
    }
    return dist;
}
//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetScreenScaleFactor(const CCamera& camera, const Vec3& object_position)
{
    Vec3 camPos = camera.GetPosition();
    float dist = camPos.GetDistance(object_position);
    return dist;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnDestroy()
{
    DestroyRenderContext();
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::CheckRespondToInput() const
{
    if (!Editor::EditorQtApplication::IsActive())
    {
        return false;
    }

    if (!hasFocus() && !m_renderViewport->hasFocus())
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::HitTest(const QPoint& point, HitContext& hitInfo)
{
    hitInfo.camera = &m_Camera;
    hitInfo.pExcludedObject = GetCameraObject();
    return QtViewport::HitTest(point, hitInfo);
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::IsBoundsVisible(const AABB& box) const
{
    // If at least part of bbox is visible then its visible.
    return m_Camera.IsAABBVisible_F(AABB(box.min, box.max));
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::CenterOnSelection()
{
    if (!GetIEditor()->GetSelection()->IsEmpty())
    {
        // Get selection bounds & center
        CSelectionGroup* sel = GetIEditor()->GetSelection();
        AABB selectionBounds = sel->GetBounds();
        CenterOnAABB(selectionBounds);
    }
}

void EditorViewportWidget::CenterOnAABB(const AABB& aabb)
{
    Vec3 selectionCenter = aabb.GetCenter();

    // Minimum center size is 40cm
    const float minSelectionRadius = 0.4f;
    const float selectionSize = std::max(minSelectionRadius, aabb.GetRadius());

    // Move camera 25% further back than required
    const float centerScale = 1.25f;

    // Decompose original transform matrix
    const Matrix34& originalTM = GetViewTM();
    AffineParts affineParts;
    affineParts.SpectralDecompose(originalTM);

    // Forward vector is y component of rotation matrix
    Matrix33 rotationMatrix(affineParts.rot);
    const Vec3 viewDirection = rotationMatrix.GetColumn1().GetNormalized();

    // Compute adjustment required by FOV != 90 degrees
    const float fov = GetFOV();
    const float fovScale = (1.0f / tan(fov * 0.5f));

    // Compute new transform matrix
    const float distanceToTarget = selectionSize * fovScale * centerScale;
    const Vec3 newPosition = selectionCenter - (viewDirection * distanceToTarget);
    Matrix34 newTM = Matrix34(rotationMatrix, newPosition);

    // Set new orbit distance
    m_orbitDistance = distanceToTarget;
    m_orbitDistance = fabs(m_orbitDistance);

    SetViewTM(newTM);
    SandboxEditor::OrbitCameraControlsBus::Event(GetViewportId(), &SandboxEditor::OrbitCameraControlsBus::Events::SetOrbitDistance, m_orbitDistance);
}

void EditorViewportWidget::CenterOnSliceInstance()
{
    AzToolsFramework::EntityIdList selectedEntityList;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    AZ::SliceComponent::SliceInstanceAddress sliceAddress;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(sliceAddress,
        &AzToolsFramework::ToolsApplicationRequestBus::Events::FindCommonSliceInstanceAddress, selectedEntityList);

    if (!sliceAddress.IsValid())
    {
        return;
    }

    AZ::EntityId sliceRootEntityId;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(sliceRootEntityId,
        &AzToolsFramework::ToolsApplicationRequestBus::Events::GetRootEntityIdOfSliceInstance, sliceAddress);

    if (!sliceRootEntityId.IsValid())
    {
        return;
    }

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList{sliceRootEntityId});

    const AZ::SliceComponent::InstantiatedContainer* instantiatedContainer = sliceAddress.GetInstance()->GetInstantiated();

    AABB aabb(Vec3(std::numeric_limits<float>::max()), Vec3(-std::numeric_limits<float>::max()));
    for (AZ::Entity* entity : instantiatedContainer->m_entities)
    {
        CEntityObject* entityObject = nullptr;
        AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(entityObject, entity->GetId(),
            &AzToolsFramework::ComponentEntityEditorRequestBus::Events::GetSandboxObject);
        AABB box;
        entityObject->GetBoundBox(box);
        aabb.Add(box.min);
        aabb.Add(box.max);
    }
    CenterOnAABB(aabb);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetFOV(float fov)
{
    if (m_viewEntityId.IsValid())
    {
        Camera::CameraRequestBus::Event(m_viewEntityId, &Camera::CameraComponentRequests::SetFov, AZ::RadToDeg(fov));
    }
    else
    {
        m_camFOV = fov;
        // Set the active camera's FOV
        {
            AZ::Matrix4x4 clipMatrix;
            AZ::MakePerspectiveFovMatrixRH(
                clipMatrix,
                GetFOV(),
                aznumeric_cast<float>(width()) / aznumeric_cast<float>(height()),
                m_Camera.GetNearPlane(),
                m_Camera.GetFarPlane(),
                true
            );
            m_renderViewport->GetViewportContext()->SetCameraProjectionMatrix(clipMatrix);
        }
    }

    if (m_viewPane)
    {
        m_viewPane->OnFOVChanged(fov);
    }
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetFOV() const
{
    if (m_viewSourceType == ViewSourceType::SequenceCamera)
    {
        CBaseObject* cameraObject = GetCameraObject();

        AZ::EntityId cameraEntityId;
        AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(cameraEntityId, cameraObject, &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);
        if (cameraEntityId.IsValid())
        {
            // component Camera
            float fov = DEFAULT_FOV;
            Camera::CameraRequestBus::EventResult(fov, cameraEntityId, &Camera::CameraComponentRequests::GetFov);
            return AZ::DegToRad(fov);
        }
    }

    if (m_viewEntityId.IsValid())
    {
        float fov = AZ::RadToDeg(m_camFOV);
        Camera::CameraRequestBus::EventResult(fov, m_viewEntityId, &Camera::CameraComponentRequests::GetFov);
        return AZ::DegToRad(fov);
    }

    return m_camFOV;
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::CreateRenderContext()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::DestroyRenderContext()
{
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetDefaultCamera()
{
    if (IsDefaultCamera())
    {
        return;
    }
    ResetToViewSourceType(ViewSourceType::None);
    GetViewManager()->SetCameraObjectId(m_cameraObjectId);
    SetName(m_defaultViewName);
    SetViewTM(m_defaultViewTM);
    PostCameraSet();
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::IsDefaultCamera() const
{
    return m_viewSourceType == ViewSourceType::None;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetSequenceCamera()
{
    if (m_viewSourceType == ViewSourceType::SequenceCamera)
    {
        // Reset if we were checked before
        SetDefaultCamera();
    }
    else
    {
        ResetToViewSourceType(ViewSourceType::SequenceCamera);

        SetName(tr("Sequence Camera"));
        SetViewTM(GetViewTM());

        GetViewManager()->SetCameraObjectId(m_cameraObjectId);
        PostCameraSet();

        // ForceAnimation() so Track View will set the Camera params
        // if a camera is animated in the sequences.
        if (GetIEditor() && GetIEditor()->GetAnimation())
        {
            GetIEditor()->GetAnimation()->ForceAnimation();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void  EditorViewportWidget::SetComponentCamera(const AZ::EntityId& entityId)
{
    ResetToViewSourceType(ViewSourceType::CameraComponent);
    SetViewEntity(entityId);
}

//////////////////////////////////////////////////////////////////////////
void  EditorViewportWidget::SetEntityAsCamera(const AZ::EntityId& entityId, bool lockCameraMovement)
{
    ResetToViewSourceType(ViewSourceType::AZ_Entity);
    SetViewEntity(entityId, lockCameraMovement);
}

void EditorViewportWidget::SetFirstComponentCamera()
{
    AZ::EBusAggregateResults<AZ::EntityId> results;
    Camera::CameraBus::BroadcastResult(results, &Camera::CameraRequests::GetCameras);
    AZStd::sort_heap(results.values.begin(), results.values.end());
    AZ::EntityId entityId;
    if (results.values.size() > 0)
    {
        entityId = results.values[0];
    }
    SetComponentCamera(entityId);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetSelectedCamera()
{
    AZ::EBusAggregateResults<AZ::EntityId> cameraList;
    Camera::CameraBus::BroadcastResult(cameraList, &Camera::CameraRequests::GetCameras);
    if (cameraList.values.size() > 0)
    {
        AzToolsFramework::EntityIdList selectedEntityList;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        for (const AZ::EntityId& entityId : selectedEntityList)
        {
            if (AZStd::find(cameraList.values.begin(), cameraList.values.end(), entityId) != cameraList.values.end())
            {
                SetComponentCamera(entityId);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::IsSelectedCamera() const
{
    CBaseObject* pCameraObject = GetCameraObject();
    if (pCameraObject && pCameraObject == GetIEditor()->GetSelectedObject())
    {
        return true;
    }

    AzToolsFramework::EntityIdList selectedEntityList;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    if ((m_viewSourceType == ViewSourceType::CameraComponent  || m_viewSourceType == ViewSourceType::AZ_Entity)
        && !selectedEntityList.empty()
        && AZStd::find(selectedEntityList.begin(), selectedEntityList.end(), m_viewEntityId) != selectedEntityList.end())
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::CycleCamera()
{
    // None -> Sequence -> LegacyCamera -> ... LegacyCamera -> CameraComponent -> ... CameraComponent -> None
    // AZ_Entity has been intentionally left out of the cycle for now.
    switch (m_viewSourceType)
    {
    case EditorViewportWidget::ViewSourceType::None:
    {
        SetFirstComponentCamera();
        break;
    }
    case EditorViewportWidget::ViewSourceType::SequenceCamera:
    {
        AZ_Error("EditorViewportWidget", false, "Legacy cameras no longer exist, unable to set sequence camera.");
        break;
    }
    case EditorViewportWidget::ViewSourceType::LegacyCamera:
    {
        AZ_Warning("EditorViewportWidget", false, "Legacy cameras no longer exist, using first found component camera instead.");
        SetFirstComponentCamera();
        break;
    }
    case EditorViewportWidget::ViewSourceType::CameraComponent:
    {
        AZ::EBusAggregateResults<AZ::EntityId> results;
        Camera::CameraBus::BroadcastResult(results, &Camera::CameraRequests::GetCameras);
        AZStd::sort_heap(results.values.begin(), results.values.end());
        auto&& currentCameraIterator = AZStd::find(results.values.begin(), results.values.end(), m_viewEntityId);
        if (currentCameraIterator != results.values.end())
        {
            ++currentCameraIterator;
            if (currentCameraIterator != results.values.end())
            {
                SetComponentCamera(*currentCameraIterator);
                break;
            }
        }
        SetDefaultCamera();
        break;
    }
    case EditorViewportWidget::ViewSourceType::AZ_Entity:
    {
        // we may decide to have this iterate over just selected entities
        SetDefaultCamera();
        break;
    }
    default:
    {
        SetDefaultCamera();
        break;
    }
    }
}

void EditorViewportWidget::SetViewFromEntityPerspective(const AZ::EntityId& entityId)
{
    SetViewAndMovementLockFromEntityPerspective(entityId, false);
}

void EditorViewportWidget::SetViewAndMovementLockFromEntityPerspective(const AZ::EntityId& entityId, bool lockCameraMovement)
{
    if (!m_ignoreSetViewFromEntityPerspective)
    {
        SetEntityAsCamera(entityId, lockCameraMovement);
    }
}

bool EditorViewportWidget::GetActiveCameraPosition(AZ::Vector3& cameraPos)
{
    if (m_pPrimaryViewport == this)
    {
        if (GetIEditor()->IsInGameMode())
        {
            cameraPos = m_renderViewport->GetViewportContext()->GetCameraTransform().GetTranslation();
        }
        else
        {
            // Use viewTM, which is synced with the camera and guaranteed to be up-to-date
            cameraPos = LYVec3ToAZVec3(m_viewTM.GetTranslation());
        }

        return true;
    }

    return false;
}

bool EditorViewportWidget::GetActiveCameraState(AzFramework::CameraState& cameraState)
{
    if (m_pPrimaryViewport == this)
    {
        cameraState = GetCameraState();

        return true;
    }

    return false;
}

void EditorViewportWidget::OnStartPlayInEditor()
{
    if (m_viewEntityId.IsValid())
    {
        m_viewEntityIdCachedForEditMode = m_viewEntityId;
        AZ::EntityId runtimeEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId,
            m_viewEntityId, runtimeEntityId);

        m_viewEntityId = runtimeEntityId;
    }
}

void EditorViewportWidget::OnStopPlayInEditor()
{
    if (m_viewEntityIdCachedForEditMode.IsValid())
    {
        m_viewEntityId = m_viewEntityIdCachedForEditMode;
        m_viewEntityIdCachedForEditMode.SetInvalid();
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnCameraFOVVariableChanged([[maybe_unused]] IVariable* var)
{
    if (m_viewPane)
    {
        m_viewPane->OnFOVChanged(GetFOV());
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::HideCursor()
{
    if (m_bCursorHidden || !gSettings.viewports.bHideMouseCursorWhenCaptured)
    {
        return;
    }

    qApp->setOverrideCursor(Qt::BlankCursor);
#if AZ_TRAIT_OS_PLATFORM_APPLE
    StartFixedCursorMode(this);
#endif
    m_bCursorHidden = true;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::ShowCursor()
{
    if (!m_bCursorHidden || !gSettings.viewports.bHideMouseCursorWhenCaptured)
    {
        return;
    }

#if AZ_TRAIT_OS_PLATFORM_APPLE
    StopFixedCursorMode();
#endif
    qApp->restoreOverrideCursor();
    m_bCursorHidden = false;
}

bool EditorViewportWidget::IsKeyDown(Qt::Key key) const
{
    return m_keyDown.contains(key);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::PushDisableRendering()
{
    assert(m_disableRenderingCount >= 0);
    ++m_disableRenderingCount;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::PopDisableRendering()
{
    assert(m_disableRenderingCount >= 1);
    --m_disableRenderingCount;
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::IsRenderingDisabled() const
{
    return m_disableRenderingCount > 0;
}

//////////////////////////////////////////////////////////////////////////
QPoint EditorViewportWidget::WidgetToViewport(const QPoint &point) const
{
    return point * WidgetToViewportFactor();
}

QPoint EditorViewportWidget::ViewportToWidget(const QPoint &point) const
{
    return point / WidgetToViewportFactor();
}

//////////////////////////////////////////////////////////////////////////
QSize EditorViewportWidget::WidgetToViewport(const QSize &size) const
{
    return size * WidgetToViewportFactor();
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::BeginUndoTransaction()
{
    PushDisableRendering();
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::EndUndoTransaction()
{
    PopDisableRendering();
    Update();
}

void EditorViewportWidget::UpdateCurrentMousePos(const QPoint& newPosition)
{
    m_prevMousePos = m_mousePos;
    m_mousePos = newPosition;
}

void* EditorViewportWidget::GetSystemCursorConstraintWindow() const
{
    AzFramework::SystemCursorState systemCursorState = AzFramework::SystemCursorState::Unknown;

    AzFramework::InputSystemCursorRequestBus::EventResult(
        systemCursorState, AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::GetSystemCursorState);

    const bool systemCursorConstrained =
        (systemCursorState == AzFramework::SystemCursorState::ConstrainedAndHidden ||
         systemCursorState == AzFramework::SystemCursorState::ConstrainedAndVisible);

    return systemCursorConstrained ? renderOverlayHWND() : nullptr;
}

void EditorViewportWidget::BuildDragDropContext(AzQtComponents::ViewportDragContext& context, const QPoint& pt)
{
    const auto scaledPoint = WidgetToViewport(pt);
    QtViewport::BuildDragDropContext(context, scaledPoint);
}

void EditorViewportWidget::RestoreViewportAfterGameMode()
{
    Matrix34 preGameModeViewTM = m_preGameModeViewTM;

    QString text = QString("You are exiting Game Mode. Would you like to restore the camera in the viewport to where it was before you entered Game Mode?<br/><br/><small>This option can always be changed in the General Preferences tab of the Editor Settings, by toggling the \"%1\" option.</small><br/><br/>").arg(EditorPreferencesGeneralRestoreViewportCameraSettingName);
    QString restoreOnExitGameModePopupDisabledRegKey("Editor/AutoHide/ViewportCameraRestoreOnExitGameMode");

    // Read the popup disabled registry value
    QSettings settings;
    QVariant restoreOnExitGameModePopupDisabledRegValue = settings.value(restoreOnExitGameModePopupDisabledRegKey);

    // Has the user previously disabled being asked about restoring the camera on exiting game mode?
    if (restoreOnExitGameModePopupDisabledRegValue.isNull())
    {
        // No, ask them now
        QMessageBox messageBox(QMessageBox::Question, "O3DE", text, QMessageBox::StandardButtons(QMessageBox::No | QMessageBox::Yes), this);
        messageBox.setDefaultButton(QMessageBox::Yes);

        QCheckBox* checkBox = new QCheckBox(QStringLiteral("Do not show this message again"));
        messageBox.setCheckBox(checkBox);

        // Unconstrain the system cursor and make it visible before we show the dialog box, otherwise the user can't see the cursor.
        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
            &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
            AzFramework::SystemCursorState::UnconstrainedAndVisible);

        int response = messageBox.exec();

        if (checkBox->isChecked())
        {
            settings.setValue(restoreOnExitGameModePopupDisabledRegKey, response);
        }

        // Update the value only if the popup hasn't previously been disabled and the value has changed
        bool newSetting = (response == QMessageBox::Yes);
        if (newSetting != GetIEditor()->GetEditorSettings()->restoreViewportCamera)
        {
            GetIEditor()->GetEditorSettings()->restoreViewportCamera = newSetting;
            GetIEditor()->GetEditorSettings()->Save();
        }
    }

    bool restoreViewportCamera = GetIEditor()->GetEditorSettings()->restoreViewportCamera;
    if (restoreViewportCamera)
    {
        SetViewTM(preGameModeViewTM);
    }
    else
    {
        SetViewTM(m_gameTM);
    }
}

void EditorViewportWidget::UpdateScene()
{
    auto sceneSystem = AzFramework::SceneSystemInterface::Get();
    if (sceneSystem)
    {
        AZStd::shared_ptr<AzFramework::Scene> mainScene = sceneSystem->GetScene(AzFramework::Scene::MainSceneName);
        if (mainScene)
        {
            AZ::RPI::SceneNotificationBus::Handler::BusDisconnect();
            m_renderViewport->SetScene(mainScene);
            AZ::RPI::SceneNotificationBus::Handler::BusConnect(m_renderViewport->GetViewportContext()->GetRenderScene()->GetId());
        }
    }
}

void EditorViewportWidget::UpdateCameraFromViewportContext()
{
   // Queue a sync for the next tick, to ensure the latest version of the viewport context transform is used
    m_updateCameraPositionNextTick = true;
}

void EditorViewportWidget::SetAsActiveViewport()
{
    auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();

    const AZ::Name defaultContextName = viewportContextManager->GetDefaultViewportContextName();

    // If another viewport was active before, restore its name to its per-ID one.
    if (m_pPrimaryViewport && m_pPrimaryViewport != this && m_pPrimaryViewport->m_renderViewport)
    {
        auto viewportContext = m_pPrimaryViewport->m_renderViewport->GetViewportContext();
        if (viewportContext)
        {
            // Remove the old viewport's camera from the stack, as it's no longer the owning viewport
            viewportContextManager->PopView(defaultContextName, viewportContext->GetDefaultView());
            viewportContextManager->RenameViewportContext(viewportContext, m_pPrimaryViewport->m_defaultViewportContextName);
        }
    }

    m_pPrimaryViewport = this;
    if (m_renderViewport)
    {
        auto viewportContext = m_renderViewport->GetViewportContext();
        if (viewportContext)
        {
            // Push our camera onto the default viewport's view stack to preserve camera state continuity
            // Other views can still be pushed on top of our view for e.g. game mode
            viewportContextManager->PushView(defaultContextName, viewportContext->GetDefaultView());
            viewportContextManager->RenameViewportContext(viewportContext, defaultContextName);
        }
    }
}

bool EditorViewportSettings::GridSnappingEnabled() const
{
    return SandboxEditor::GridSnappingEnabled();
}

float EditorViewportSettings::GridSize() const
{
    return SandboxEditor::GridSnappingSize();
}

bool EditorViewportSettings::ShowGrid() const
{
    return SandboxEditor::ShowingGrid();
}

bool EditorViewportSettings::AngleSnappingEnabled() const
{
    return SandboxEditor::AngleSnappingEnabled();
}

float EditorViewportSettings::AngleStep() const
{
    return SandboxEditor::AngleSnappingSize();
}

#include <moc_EditorViewportWidget.cpp>
