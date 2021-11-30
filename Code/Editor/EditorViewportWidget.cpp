/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <AzCore/Interface/Interface.h>
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
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

// AtomToolsFramework
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>

// CryCommon
#include <CryCommon/IRenderAuxGeom.h>

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
#include "EditorPreferencesPageGeneral.h"
#include "ViewportManipulatorController.h"
#include "EditorViewportSettings.h"

#include "ViewPane.h"
#include "CustomResolutionDlg.h"
#include "AnimationContext.h"
#include "Objects/SelectionGroup.h"
#include "Core/QtEditorApplication.h"
#include "MainWindow.h"
#include "LayoutWnd.h"

// ComponentEntityEditorPlugin
#include <Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h>

// LmbrCentral
#include <LmbrCentral/Rendering/EditorCameraCorrectionBus.h>

// Atom
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/ViewProviderBus.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MatrixUtils.h>

#include <QtGui/private/qhighdpiscaling_p.h>

AZ_CVAR(
    bool, ed_visibility_logTiming, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Output the timing of the new IVisibilitySystem query");

EditorViewportWidget* EditorViewportWidget::m_pPrimaryViewport = nullptr;

#if AZ_TRAIT_OS_PLATFORM_APPLE
void StopFixedCursorMode();
void StartFixedCursorMode(QObject *viewport);
#endif

#define RENDER_MESH_TEST_DISTANCE (0.2f)
#define CURSOR_FONT_HEIGHT  8.0f
namespace AZ::ViewportHelpers
{
    static const char TextCantCreateCameraNoLevel[] = "Cannot create camera when no level is loaded.";

    class EditorEntityNotifications : public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
    public:
        EditorEntityNotifications(EditorViewportWidget& editorViewportWidget)
            : m_editorViewportWidget(editorViewportWidget)
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        ~EditorEntityNotifications() override
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        }

        // AzToolsFramework::EditorEntityContextNotificationBus overrides ...
        void OnStartPlayInEditor() override
        {
            m_editorViewportWidget.OnStartPlayInEditor();
        }

        void OnStopPlayInEditor() override
        {
            m_editorViewportWidget.OnStopPlayInEditor();
        }

        void OnStartPlayInEditorBegin() override
        {
            m_editorViewportWidget.OnStartPlayInEditorBegin();
        }

    private:
        EditorViewportWidget& m_editorViewportWidget;
    };
} // namespace AZ::ViewportHelpers

//////////////////////////////////////////////////////////////////////////
// EditorViewportWidget
//////////////////////////////////////////////////////////////////////////

EditorViewportWidget::EditorViewportWidget(const QString& name, QWidget* parent)
    : QtViewport(parent)
    , m_defaultViewName(name)
    , m_renderViewport(nullptr) //m_renderViewport is initialized later, in SetViewportId
{
    // need this to be set in order to allow for language switching on Windows
    setAttribute(Qt::WA_InputMethodEnabled);

    m_defaultViewTM.SetIdentity();

    if (GetIEditor()->GetViewManager()->GetSelectedViewport() == nullptr)
    {
        GetIEditor()->GetViewManager()->SelectViewport(this);
    }

    GetIEditor()->RegisterNotifyListener(this);

    m_displayContext.pIconManager = GetIEditor()->GetIconManager();
    GetIEditor()->GetUndoManager()->AddListener(this);

    // The renderer requires something, so don't allow us to shrink to absolutely nothing
    // This won't in fact stop the viewport from being shrunk, when it's the centralWidget for
    // the MainWindow, but it will stop the viewport from getting resize events
    // once it's smaller than that, which from the renderer's perspective works out
    // to be the same thing.
    setMinimumSize(50, 50);

    setMouseTracking(true);

    Camera::EditorCameraRequestBus::Handler::BusConnect();
    Camera::CameraNotificationBus::Handler::BusConnect();

    m_editorEntityNotifications = AZStd::make_unique<AZ::ViewportHelpers::EditorEntityNotifications>(*this);
    AzFramework::AssetCatalogEventBus::Handler::BusConnect();

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

    m_editorViewportSettings.Disconnect();
    DisconnectViewportInteractionRequestBus();
    m_editorEntityNotifications.reset();
    Camera::EditorCameraRequestBus::Handler::BusDisconnect();
    Camera::CameraNotificationBus::Handler::BusDisconnect();
    GetIEditor()->GetUndoManager()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::resizeEvent(QResizeEvent* event)
{
    // Call base class resize event while not rendering
    PushDisableRendering();
    QtViewport::resizeEvent(event);
    PopDisableRendering();

    // Emit Legacy system events about the viewport size change
    const QRect rcWindow = rect().translated(mapToGlobal(QPoint()));

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE, rcWindow.left(), rcWindow.top());

    m_rcClient = rect();
    m_rcClient.setBottomRight(WidgetToViewport(m_rcClient.bottomRight()));

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE, width(), height());

    // In the case of the default viewport camera, we must re-set the FOV, which also updates the aspect ratio
    // Component cameras hand this themselves
    if (m_viewSourceType == ViewSourceType::None)
    {
        SetFOV(GetFOV());
    }
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
            const QFont font(kFontName, static_cast<int>(kFontSize / 10.0f));
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

AzToolsFramework::ViewportInteraction::MousePick EditorViewportWidget::BuildMousePick(const QPoint& point) const
{
    AzToolsFramework::ViewportInteraction::MousePick mousePick;
    mousePick.m_screenCoordinates = AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(point);
    const auto[origin, direction] = m_renderViewport->ViewportScreenToWorldRay(mousePick.m_screenCoordinates);
    mousePick.m_rayOrigin = origin;
    mousePick.m_rayDirection = direction;
    return mousePick;
}

AzToolsFramework::ViewportInteraction::MouseInteraction EditorViewportWidget::BuildMouseInteractionInternal(
    const AzToolsFramework::ViewportInteraction::MouseButtons buttons,
    const AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers,
    const AzToolsFramework::ViewportInteraction::MousePick& mousePick) const
{
    AzToolsFramework::ViewportInteraction::MouseInteraction mouse;
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
    namespace AztfVi = AzToolsFramework::ViewportInteraction;

    return BuildMouseInteractionInternal(
        AztfVi::BuildMouseButtons(buttons),
        AztfVi::BuildKeyboardModifiers(modifiers),
        BuildMousePick(WidgetToViewport(point)));
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::event(QEvent* event)
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

    // Render
    {
        // TODO: Move out this logic to a controller and refactor to work with Atom
        ProcessRenderLisneters(m_displayContext);

        m_displayContext.Flush2D();

        // Post Render Callback
        {
            PostRenderers::iterator itr = m_postRenderers.begin();
            PostRenderers::iterator end = m_postRenderers.end();
            for (; itr != end; ++itr)
            {
                (*itr)->OnPostRender();
            }
        }
    }

    {
        auto start = std::chrono::steady_clock::now();

        m_entityVisibilityQuery.UpdateVisibility(m_renderViewport->GetCameraState());

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
void EditorViewportWidget::PostCameraSet()
{
    if (m_viewPane)
    {
        m_viewPane->OnFOVChanged(GetFOV());
    }

    // CryLegacy notify
    GetIEditor()->Notify(eNotify_CameraChanged);

    // Special case in the editor; if the camera is the default editor camera,
    // notify that the active view changed. In game mode, it is a hard error to not have
    // any cameras on the view stack!
    if (m_viewSourceType == ViewSourceType::None)
    {
        m_sendingOnActiveChanged = true;
        Camera::CameraNotificationBus::Broadcast(
            &Camera::CameraNotificationBus::Events::OnActiveViewChanged, AZ::EntityId());
        m_sendingOnActiveChanged = false;
    }

    // Notify about editor camera change
    Camera::EditorCameraNotificationBus::Broadcast(
        &Camera::EditorCameraNotificationBus::Events::OnViewportViewEntityChanged, m_viewEntityId);

    // The editor view entity ID has changed, and the editor camera component "Be This Camera" text needs to be updated
    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
        AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* EditorViewportWidget::GetCameraObject() const
{
    CBaseObject* pCameraObject = nullptr;

    if (m_viewSourceType == ViewSourceType::CameraComponent)
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

            SetCurrentCursor(STD_CURSOR_GAME);

            if (ShouldPreviewFullscreen())
            {
                StartFullscreenPreview();
            }
        }

        if (m_renderViewport)
        {
            m_renderViewport->SetInputProcessingEnabled(false);
        }
    }
    break;

    case eNotify_OnEndGameMode:
        if (GetIEditor()->GetViewManager()->GetGameViewport() == this)
        {
            SetCurrentCursor(STD_CURSOR_DEFAULT);

            if (m_inFullscreenPreview)
            {
                StopFullscreenPreview();
            }

            RestoreViewportAfterGameMode();
        }

        if (m_renderViewport)
        {
            m_renderViewport->SetInputProcessingEnabled(true);
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
        {
            PopDisableRendering();

            Matrix34 viewTM;
            viewTM.SetIdentity();

            viewTM.SetTranslation(Vec3(m_editorViewportSettings.DefaultEditorCameraPosition()));
            SetViewTM(viewTM);

            UpdateScene();
        }
        break;

    case eNotify_OnBeginTerrainCreate:
        PushDisableRendering();
        break;

    case eNotify_OnEndTerrainCreate:
        {
            PopDisableRendering();

            Matrix34 viewTM;
            viewTM.SetIdentity();

            viewTM.SetTranslation(Vec3(m_editorViewportSettings.DefaultEditorCameraPosition()));
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
    }
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


    if (GetIEditor()->IsInGameMode())
    {
        return;
    }

    RenderAll();

    // Draw 2D helpers.
#ifdef LYSHINE_ATOM_TODO
    TransformationMatrices backupSceneMatrices;
#endif
    m_debugDisplay->DepthTestOff();
#ifdef LYSHINE_ATOM_TODO
    m_renderer->Set2DMode(m_rcClient.right(), m_rcClient.bottom(), backupSceneMatrices);
#endif
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
        namespace AztfVi = AzToolsFramework::ViewportInteraction;

        AztfVi::KeyboardModifiers keyboardModifiers;
        AztfVi::EditorModifierKeyRequestBus::BroadcastResult(
            keyboardModifiers, &AztfVi::EditorModifierKeyRequestBus::Events::QueryKeyboardModifiers);

        m_debugDisplay->DepthTestOff();
        m_manipulatorManager->DrawManipulators(
            *m_debugDisplay, m_renderViewport->GetCameraState(),
            BuildMouseInteractionInternal(
                AztfVi::MouseButtons(AztfVi::TranslateMouseButtons(QGuiApplication::mouseButtons())), keyboardModifiers,
                BuildMousePick(WidgetToViewport(mapFromGlobal(QCursor::pos())))));
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

        m_safeFrame.setLeft(static_cast<int>(m_safeFrame.left() + widthDifference * 0.5f));
        m_safeFrame.setRight(static_cast<int>(m_safeFrame.right() - widthDifference * 0.5f));
    }
    else
    {
        float maxSafeFrameHeight = m_safeFrame.width() / targetAspectRatio;
        float heightDifference = m_safeFrame.height() - maxSafeFrameHeight;

        m_safeFrame.setTop(static_cast<int>(m_safeFrame.top() + heightDifference * 0.5f));
        m_safeFrame.setBottom(static_cast<int>(m_safeFrame.bottom() - heightDifference * 0.5f));
    }

    m_safeFrame.adjust(0, 0, -1, -1); // <-- aesthetic improvement.

    const float SAFE_ACTION_SCALE_FACTOR = 0.05f;
    m_safeAction = m_safeFrame;
    m_safeAction.adjust(
        static_cast<int>(m_safeFrame.width() * SAFE_ACTION_SCALE_FACTOR),
        static_cast<int>(m_safeFrame.height() * SAFE_ACTION_SCALE_FACTOR),
        static_cast<int>(-m_safeFrame.width() * SAFE_ACTION_SCALE_FACTOR),
        static_cast<int>(-m_safeFrame.height() * SAFE_ACTION_SCALE_FACTOR));

    const float SAFE_TITLE_SCALE_FACTOR = 0.1f;
    m_safeTitle = m_safeFrame;
    m_safeTitle.adjust(
        static_cast<int>(m_safeFrame.width() * SAFE_TITLE_SCALE_FACTOR),
        static_cast<int>(m_safeFrame.height() * SAFE_TITLE_SCALE_FACTOR),
        static_cast<int>(-m_safeFrame.width() * SAFE_TITLE_SCALE_FACTOR),
        static_cast<int>(-m_safeFrame.height() * SAFE_TITLE_SCALE_FACTOR));
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
        AZ::Vector3 topLeft(static_cast<float>(frame.left() + i), static_cast<float>(frame.top() + i), 0.0f);
        AZ::Vector3 bottomRight(static_cast<float>(frame.right() - i), static_cast<float>(frame.bottom() - i), 0.0f);
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

void EditorViewportWidget::FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntitiesOut)
{
    visibleEntitiesOut.assign(m_entityVisibilityQuery.Begin(), m_entityVisibilityQuery.End());
}

QWidget* EditorViewportWidget::GetWidgetForViewportContextMenu()
{
    return this;
}

bool EditorViewportWidget::ShowingWorldSpace()
{
    namespace AztfVi = AzToolsFramework::ViewportInteraction;

    AztfVi::KeyboardModifiers keyboardModifiers;
    AztfVi::EditorModifierKeyRequestBus::BroadcastResult(
        keyboardModifiers, &AztfVi::EditorModifierKeyRequestBus::Events::QueryKeyboardModifiers);

    return keyboardModifiers.Shift();
}

void EditorViewportWidget::SetViewportId(int id)
{
    CViewport::SetViewportId(id);

    // Clear the cached DebugDisplay pointer. we're about to delete that render viewport, and deleting the render
    // viewport invalidates the DebugDisplay.
    m_debugDisplay = nullptr;

    // First delete any existing layout
    // This also deletes any existing render viewport widget (since it will be added to the layout)
    // Below is the typical method of clearing a QLayout, see e.g. https://doc.qt.io/qt-5/qlayout.html#takeAt
    if (QLayout* thisLayout = layout())
    {
        QLayoutItem* item;
        while ((item = thisLayout->takeAt(0)) != nullptr)
        {
            if (QWidget* widget = item->widget())
            {
                delete widget;
            }
            thisLayout->removeItem(item);
            delete item;
        }
        delete thisLayout;
    }

    // Now that we have an ID, we can initialize our viewport.
    m_renderViewport = new AtomToolsFramework::RenderViewportWidget(this, false);
    if (!m_renderViewport->InitializeViewportContext(id))
    {
        AZ_Warning("EditorViewportWidget", false, "Failed to initialize RenderViewportWidget's ViewportContext");
        delete m_renderViewport;
        m_renderViewport = nullptr;
        return;
    }
    auto viewportContext = m_renderViewport->GetViewportContext();
    m_defaultViewportContextName = viewportContext->GetName();
    m_defaultView = viewportContext->GetDefaultView();
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom, this);
    layout->setContentsMargins(QMargins());
    layout->addWidget(m_renderViewport);

    m_renderViewport->GetControllerList()->Add(AZStd::make_shared<SandboxEditor::ViewportManipulatorController>());

    m_editorModularViewportCameraComposer = AZStd::make_unique<SandboxEditor::EditorModularViewportCameraComposer>(AzFramework::ViewportId(id));
    m_renderViewport->GetControllerList()->Add(m_editorModularViewportCameraComposer->CreateModularViewportCameraController());

    m_editorViewportSettings.Connect(AzFramework::ViewportId(id));

    UpdateScene();

    if (m_pPrimaryViewport == this)
    {
        SetAsActiveViewport();
    }

    m_editorViewportSettingsCallbacks = SandboxEditor::CreateEditorViewportSettingsCallbacks();

    m_gridSnappingHandler = SandboxEditor::GridSnappingChangedEvent::Handler(
        [id](const bool snapping)
        {
            AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Event(
                id, &AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Events::OnGridSnappingChanged, snapping);
        });

    m_editorViewportSettingsCallbacks->SetGridSnappingChangedEvent(m_gridSnappingHandler);
}

void EditorViewportWidget::ConnectViewportInteractionRequestBus()
{
    AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequestBus::Handler::BusConnect(GetViewportId());
    AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Handler::BusConnect(GetViewportId());
    m_viewportUi.ConnectViewportUiBus(GetViewportId());
    AzFramework::ViewportBorderRequestBus::Handler::BusConnect(GetViewportId());

    AzFramework::InputSystemCursorConstraintRequestBus::Handler::BusConnect();
}

void EditorViewportWidget::DisconnectViewportInteractionRequestBus()
{
    AzFramework::InputSystemCursorConstraintRequestBus::Handler::BusDisconnect();

    AzFramework::ViewportBorderRequestBus::Handler::BusDisconnect();
    m_viewportUi.DisconnectViewportUiBus();
    AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Handler::BusDisconnect();
    AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequestBus::Handler::BusDisconnect();
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
} // namespace AZ::ViewportHelpers

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

        const auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        if (!gameEngine || !gameEngine->IsLevelLoaded() ||
            (prefabEditorEntityOwnershipInterface && !prefabEditorEntityOwnershipInterface->IsRootPrefabAssigned()))
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

    menu->addSeparator();

    // Camera Sub menu
    QMenu* customCameraMenu = menu->addMenu(tr("Camera"));

    QAction* action = customCameraMenu->addAction("Editor Camera");
    action->setCheckable(true);
    action->setChecked(m_viewSourceType == ViewSourceType::None);
    connect(action, &QAction::triggered, this, &EditorViewportWidget::SetDefaultCamera);

    AZ::EBusAggregateResults<AZ::EntityId> getCameraResults;
    Camera::CameraBus::BroadcastResult(getCameraResults, &Camera::CameraRequests::GetCameras);

    QVector<QAction*> additionalCameras;
    additionalCameras.reserve(static_cast<int>(getCameraResults.values.size()));

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

void EditorViewportWidget::SetViewTM(const Matrix34& tm)
{
    SetViewTM(tm, false);
}

void EditorViewportWidget::SetViewTM(const Matrix34& camMatrix, bool bMoveOnly)
{
    AZ_Warning("EditorViewportWidget", !bMoveOnly, "'Move Only' mode is deprecated");
    CBaseObject* cameraObject = GetCameraObject();

    // Check if the active view entity is the same as the entity having the current view
    // Sometimes this isn't the case because the active view is in the process of changing
    // If it isn't, then we're doing the wrong thing below: we end up copying data from one (seemingly random)
    // camera to another (seemingly random) camera
    enum class ShouldUpdateObject
    {
        Yes, No, YesButViewsOutOfSync
    };

    const ShouldUpdateObject shouldUpdateObject = [&]() {
        if (!cameraObject)
        {
            return ShouldUpdateObject::No;
        }

        if (m_viewSourceType == ViewSourceType::CameraComponent)
        {
            if (!m_viewEntityId.IsValid())
            {
                // Should be impossible anyways
                AZ_Assert(false, "Internal logic error - view entity Id and view source type out of sync. Please report this as a bug");
                return ShouldUpdateObject::No;
            }

            // Check that the current view is the same view as the view entity view
            AZ::RPI::ViewPtr viewEntityView;
            AZ::RPI::ViewProviderBus::EventResult(
                viewEntityView, m_viewEntityId,
                &AZ::RPI::ViewProviderBus::Events::GetView
            );

            return viewEntityView == GetCurrentAtomView() ? ShouldUpdateObject::Yes : ShouldUpdateObject::YesButViewsOutOfSync;
        }
        else
        {
            AZ_Assert(false, "Internal logic error - view source type is the default camera, but there is somehow a camera object. Please report this as a bug.");

            // For non-component cameras, can't do any complicated view-based checks
            return ShouldUpdateObject::No;
        }
    }();

    if (shouldUpdateObject == ShouldUpdateObject::Yes)
    {
        AZ::Matrix3x3 lookThroughEntityCorrection = AZ::Matrix3x3::CreateIdentity();
        if (m_viewEntityId.IsValid())
        {
            LmbrCentral::EditorCameraCorrectionRequestBus::EventResult(
                lookThroughEntityCorrection, m_viewEntityId,
                &LmbrCentral::EditorCameraCorrectionRequests::GetInverseTransformCorrection);
        }

        int flags = 0;
        {
            // It isn't clear what this logic is supposed to do (it's legacy code)...
            // For now, instead of removing it, just assert if the m_pressedKeyState isn't as expected
            // Do not touch unless you really know what you're doing!
            AZ_Assert(m_pressedKeyState == KeyPressedState::AllUp, "Internal logic error - key pressed state got changed. Please report this as a bug");

            AZStd::optional<CUndo> undo;
            if (m_pressedKeyState != KeyPressedState::PressedInPreviousFrame)
            {
                flags = eObjectUpdateFlags_UserInput;
                undo.emplace("Move Camera");
            }

            if (bMoveOnly)
            {
                cameraObject->SetWorldPos(camMatrix.GetTranslation(), flags);
            }
            else
            {
                cameraObject->SetWorldTM(camMatrix * AZMatrix3x3ToLYMatrix3x3(lookThroughEntityCorrection), flags);
            }
        }
    }
    else if (shouldUpdateObject == ShouldUpdateObject::YesButViewsOutOfSync)
    {
        // Technically this should not cause anything to go wrong, but may indicate some underlying bug by a caller
        // of SetViewTm, for example, trying to set the view TM in the middle of a camera change.
        // If this is an important case, it can potentially be supported by caching the requested view TM
        // until the entity and view ptr become synchronized.
        AZ_Error("EditorViewportWidget",
            m_playInEditorState == PlayInEditorState::Editor,
            "Viewport camera entity ID and view out of sync; request view transform will be ignored. "
            "Please report this as a bug."
        );
    }
    else if (shouldUpdateObject == ShouldUpdateObject::No)
    {
        GetCurrentAtomView()->SetCameraTransform(LYTransformToAZMatrix3x4(camMatrix));
    }

    if (m_pressedKeyState == KeyPressedState::PressedThisFrame)
    {
        m_pressedKeyState = KeyPressedState::PressedInPreviousFrame;
    }
}

const Matrix34& EditorViewportWidget::GetViewTM() const
{
    // `m_viewTmStorage' is only required because we must return a reference
    m_viewTmStorage = AZTransformToLYTransform(GetCurrentAtomView()->GetCameraTransform());
    return m_viewTmStorage;
};

AZ::EntityId EditorViewportWidget::GetCurrentViewEntityId()
{
    // Sanity check that this camera entity ID is actually the camera entity which owns the current active render view
    if (m_viewSourceType == ViewSourceType::CameraComponent)
    {
        // Check that the current view is the same view as the view entity view
        AZ::RPI::ViewPtr viewEntityView;
        AZ::RPI::ViewProviderBus::EventResult(
            viewEntityView, m_viewEntityId,
            &AZ::RPI::ViewProviderBus::Events::GetView
        );

        [[maybe_unused]] const bool isViewEntityCorrect = viewEntityView == GetCurrentAtomView();
        AZ_Error("EditorViewportWidget", isViewEntityCorrect,
            "GetCurrentViewEntityId called while the current view is being changed. "
            "You may get inconsistent results if you make use of the returned entity ID. "
            "This is an internal error, please report it as a bug."
        );
    }

    return m_viewEntityId;
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
        dc.pRenderAuxGeom->DrawTriangles(&verts[0], static_cast<uint32>(verts.size()), &inds[0], numInds, &colors[0]);
        dc.CullOn();
        dc.DepthWriteOn();
    }
}

Vec3 EditorViewportWidget::WorldToView3D(const Vec3& wp, [[maybe_unused]] int nFlags) const
{
    Vec3 out(0, 0, 0);
    float x, y;

    ProjectToScreen(wp.x, wp.y, wp.z, &x, &y);
    if (_finite(x) && _finite(y))
    {
        out.x = (x / 100) * m_rcClient.width();
        out.y = (y / 100) * m_rcClient.height();
        out.x /= static_cast<float>(QHighDpiScaling::factor(windowHandle()->screen()));
        out.y /= static_cast<float>(QHighDpiScaling::factor(windowHandle()->screen()));
    }
    return out;
}

//////////////////////////////////////////////////////////////////////////
QPoint EditorViewportWidget::WorldToView(const Vec3& wp) const
{
    return AzToolsFramework::ViewportInteraction::QPointFromScreenPoint(m_renderViewport->ViewportWorldToScreen(LYVec3ToAZVec3(wp)));
}

//////////////////////////////////////////////////////////////////////////
Vec3 EditorViewportWidget::ViewToWorld(
    const QPoint& vp, bool* collideWithTerrain, bool onlyTerrain, bool bSkipVegetation, bool bTestRenderMesh, bool* collideWithObject) const
{
    AZ_PROFILE_FUNCTION(Editor);

    AZ_UNUSED(collideWithTerrain);
    AZ_UNUSED(onlyTerrain);
    AZ_UNUSED(bTestRenderMesh);
    AZ_UNUSED(bSkipVegetation);
    AZ_UNUSED(bSkipVegetation);
    AZ_UNUSED(collideWithObject);

    auto ray = m_renderViewport->ViewportScreenToWorldRay(AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(vp));

    const float maxDistance = 10000.f;
    Vec3 v = AZVec3ToLYVec3(ray.direction) * maxDistance;

    if (!_finite(v.x) || !_finite(v.y) || !_finite(v.z))
    {
        return Vec3(0, 0, 0);
    }

    Vec3 colp = AZVec3ToLYVec3(ray.origin) + 0.002f * v;

    return colp;
}

//////////////////////////////////////////////////////////////////////////
Vec3 EditorViewportWidget::ViewToWorldNormal(const QPoint& vp, bool onlyTerrain, bool bTestRenderMesh)
{
    AZ_UNUSED(vp);
    AZ_UNUSED(onlyTerrain);
    AZ_UNUSED(bTestRenderMesh);

    AZ_PROFILE_FUNCTION(Editor);

    return Vec3(0, 0, 1);
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

void EditorViewportWidget::UnProjectFromScreen(float sx, float sy, float* px, float* py, float* pz) const
{
    const AZ::Vector3 wp = m_renderViewport->ViewportScreenToWorld(AzFramework::ScreenPoint{(int)sx, m_rcClient.bottom() - ((int)sy)});
    *px = wp.GetX();
    *py = wp.GetY();
    *pz = wp.GetZ();
}

void EditorViewportWidget::ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy) const
{
    AzFramework::ScreenPoint screenPosition = m_renderViewport->ViewportWorldToScreen(AZ::Vector3{ptx, pty, ptz});
    *sx = static_cast<float>(screenPosition.m_x);
    *sy = static_cast<float>(screenPosition.m_y);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const
{
    QRect rc = m_rcClient;

    Vec3 pos0, pos1;
    float wx, wy, wz;
    UnProjectFromScreen(static_cast<float>(vp.x()), static_cast<float>(rc.bottom() - vp.y()), &wx, &wy, &wz);

    if (!_finite(wx) || !_finite(wy) || !_finite(wz))
    {
        return;
    }

    if (fabs(wx) > 1000000 || fabs(wy) > 1000000 || fabs(wz) > 1000000)
    {
        return;
    }

    pos0(wx, wy, wz);

    raySrc = pos0;
    rayDir = (pos0 - AZVec3ToLYVec3(m_renderViewport->GetCameraState().m_position)).GetNormalized();
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetScreenScaleFactor([[maybe_unused]] const Vec3& worldPoint) const
{
    AZ_Error("CryLegacy", false, "EditorViewportWidget::GetScreenScaleFactor not implemented");
    return 1.f;
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
    hitInfo.pExcludedObject = GetCameraObject();
    return QtViewport::HitTest(point, hitInfo);
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::IsBoundsVisible(const AABB&) const
{
    AZ_Assert(false, "Not supported");
    return false;
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
    float orbitDistance = distanceToTarget;
    orbitDistance = fabs(orbitDistance);

    SetViewTM(newTM);
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
        Camera::CameraRequestBus::Event(m_viewEntityId, &Camera::CameraComponentRequests::SetFovRadians, fov);
    }
    else
    {
        auto m = m_defaultView->GetViewToClipMatrix();
        AZ::SetPerspectiveMatrixFOV(m, fov, aznumeric_cast<float>(width()) / aznumeric_cast<float>(height()));
        m_defaultView->SetViewToClipMatrix(m);
    }
}

//////////////////////////////////////////////////////////////////////////
float EditorViewportWidget::GetFOV() const
{
    if (m_viewEntityId.IsValid())
    {
        float fov = 0.f;
        Camera::CameraRequestBus::EventResult(fov, m_viewEntityId, &Camera::CameraComponentRequests::GetFovRadians);
        return fov;
    }
    else
    {
        return AZ::GetPerspectiveMatrixFOV(m_defaultView->GetViewToClipMatrix());
    }
}

void EditorViewportWidget::OnActiveViewChanged(const AZ::EntityId& viewEntityId)
{
    // Avoid re-entry
    if (m_sendingOnActiveChanged)
    {
        return;
    }

    // Ignore any changes in simulation mode
    if (m_playInEditorState != PlayInEditorState::Editor)
    {
        return;
    }

    // if they've picked the same camera, then that means they want to toggle
    if (viewEntityId.IsValid())
    {
        // Any such events for game entities should be filtered out by the check above
        AZ_Error(
            "EditorViewportWidget",
            Camera::EditorCameraViewRequestBus::FindFirstHandler(viewEntityId) != nullptr,
            "Internal logic error - active view changed to an entity which is not an editor camera. "
            "Please report this as a bug."
        );

        m_viewEntityId = viewEntityId;
        m_viewSourceType = ViewSourceType::CameraComponent;
        AZStd::string entityName;
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, viewEntityId);
        SetName(QString("Camera entity: %1").arg(entityName.c_str()));

        PostCameraSet();
    }
    else
    {
        SetDefaultCamera();
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetDefaultCamera()
{
    m_viewEntityId.SetInvalid();
    m_viewSourceType = ViewSourceType::None;
    GetViewManager()->SetCameraObjectId(GUID_NULL);
    SetName(m_defaultViewName);

    // Synchronize the configured editor viewport FOV to the default camera
    if (m_viewPane)
    {
        const float fov = gSettings.viewports.fDefaultFov;
        m_viewPane->OnFOVChanged(fov);
        SetFOV(fov);
    }

    // Push the default view as the active view
    auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    if (atomViewportRequests)
    {
        const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
        atomViewportRequests->PushView(contextName, m_defaultView);
    }

    // Set the default Editor Camera position.
    m_defaultViewTM.SetTranslation(Vec3(m_editorViewportSettings.DefaultEditorCameraPosition()));
    SetViewTM(m_defaultViewTM);

    PostCameraSet();
}

//////////////////////////////////////////////////////////////////////////
AZ::RPI::ViewPtr EditorViewportWidget::GetCurrentAtomView() const
{
    if (m_renderViewport && m_renderViewport->GetViewportContext())
    {
        return m_renderViewport->GetViewportContext()->GetDefaultView();
    }
    else
    {
        return nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetComponentCamera(const AZ::EntityId& entityId)
{
    SetViewFromEntityPerspective(entityId);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetEntityAsCamera(const AZ::EntityId& entityId, bool lockCameraMovement)
{
    SetViewAndMovementLockFromEntityPerspective(entityId, lockCameraMovement);
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

    if ((m_viewSourceType == ViewSourceType::CameraComponent)
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

void EditorViewportWidget::SetViewAndMovementLockFromEntityPerspective(const AZ::EntityId& entityId, [[maybe_unused]] bool lockCameraMovement)
{
    // This is an editor event, so is only serviced during edit mode, not play game mode
    //
    if (m_playInEditorState != PlayInEditorState::Editor)
    {
        AZ_Warning("EditorViewportWidget", false,
            "Tried to change the editor camera during play game in editor; this is currently unsupported"
        );
        return;
    }

    AZ_Assert(lockCameraMovement == false, "SetViewAndMovementLockFromEntityPerspective with lockCameraMovement == true not supported");

    if (entityId.IsValid())
    {
        Camera::CameraRequestBus::Event(entityId, &Camera::CameraRequestBus::Events::MakeActiveView);
    }
    else
    {
        // The default camera
        SetDefaultCamera();
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
            cameraPos = LYVec3ToAZVec3(GetViewTM().GetTranslation());
        }

        return true;
    }

    return false;
}

bool EditorViewportWidget::GetActiveCameraState(AzFramework::CameraState& cameraState)
{
    if (m_pPrimaryViewport == this)
    {
        cameraState = m_renderViewport->GetCameraState();
        return true;
    }

    return false;
}

void EditorViewportWidget::OnStartPlayInEditorBegin()
{
    m_playInEditorState = PlayInEditorState::Starting;
}

void EditorViewportWidget::OnStartPlayInEditor()
{
    m_playInEditorState = PlayInEditorState::Started;

    if (m_viewEntityId.IsValid())
    {
        // Note that this is assuming that the Atom camera components will share the same view ptr
        // in editor as in game mode

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
    m_playInEditorState = PlayInEditorState::Editor;

    // Note that:
    // - this is assuming that the Atom camera components will share the same view ptr in editor as in game mode.
    // - if `m_viewEntityIdCachedForEditMode' is invalid, the camera before game mode was the default editor camera
    // - we MUST set the camera again when exiting game mode, because when rendering with trackview, the editor camera gets set somehow
    SetViewFromEntityPerspective(m_viewEntityIdCachedForEditMode);
    m_viewEntityIdCachedForEditMode.SetInvalid();
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

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::PushDisableRendering()
{
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
QPoint EditorViewportWidget::WidgetToViewport(const QPoint& point) const
{
    return point * WidgetToViewportFactor();
}

QPoint EditorViewportWidget::ViewportToWidget(const QPoint& point) const
{
    return point / WidgetToViewportFactor();
}

//////////////////////////////////////////////////////////////////////////
QSize EditorViewportWidget::WidgetToViewport(const QSize& size) const
{
    return size * WidgetToViewportFactor();
}

//////////////////////////////////////////////////////////////////////////
double EditorViewportWidget::WidgetToViewportFactor() const
{
#if defined(AZ_PLATFORM_WINDOWS)
    // Needed for high DPI mode on windows
    return devicePixelRatioF();
#else
    return 1.0;
#endif
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

void EditorViewportWidget::BuildDragDropContext(
    AzQtComponents::ViewportDragContext& context, const AzFramework::ViewportId viewportId, const QPoint& point)
{
    QtViewport::BuildDragDropContext(context, viewportId, point);
}

void EditorViewportWidget::RestoreViewportAfterGameMode()
{
    Matrix34 preGameModeViewTM = m_preGameModeViewTM;

    QString text =
        QString(
            tr("When leaving \" Game Mode \" the engine will automatically restore your camera position to the default position before you "
            "had entered Game mode.<br/><br/><small>If you dislike this setting you can always change this anytime in the global "
            "preferences.</small><br/><br/>"));
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
        checkBox->setChecked(true);
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
        AZ_Warning("CryLegacy", false, "Not restoring the editor viewport camera is currently unsupported");
        SetViewTM(preGameModeViewTM);
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

void EditorViewportSettings::Connect(const AzFramework::ViewportId viewportId)
{
    AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler::BusConnect(viewportId);
}

void EditorViewportSettings::Disconnect()
{
    AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler::BusDisconnect();
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

float EditorViewportSettings::ManipulatorLineBoundWidth() const
{
    return SandboxEditor::ManipulatorLineBoundWidth();
}

float EditorViewportSettings::ManipulatorCircleBoundWidth() const
{
    return SandboxEditor::ManipulatorCircleBoundWidth();
}

bool EditorViewportSettings::StickySelectEnabled() const
{
    return SandboxEditor::StickySelectEnabled();
}

AZ::Vector3 EditorViewportSettings::DefaultEditorCameraPosition() const
{
    return SandboxEditor::CameraDefaultEditorPosition();
}

AZ_CVAR_EXTERNED(bool, ed_previewGameInFullscreen_once);

bool EditorViewportWidget::ShouldPreviewFullscreen() const
{
    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (!layout)
    {
        AZ_Assert(false, "CRenderViewport: No View Manager layout");
        return false;
    }

    // Doesn't work with split layout
    if (layout->GetLayout() != EViewLayout::ET_Layout0)
    {
        return false;
    }

    // If level not loaded, don't preview in fullscreen (preview shouldn't work at all without a level, but it does)
    if (auto ge = GetIEditor()->GetGameEngine())
    {
        if (!ge->IsLevelLoaded())
        {
            return false;
        }
    }

    // Check 'ed_previewGameInFullscreen_once'
    if (ed_previewGameInFullscreen_once)
    {
        ed_previewGameInFullscreen_once = false;
        return true;
    }
    else
    {
        return false;
    }
}

void EditorViewportWidget::StartFullscreenPreview()
{
    AZ_Assert(!m_inFullscreenPreview, "EditorViewportWidget::StartFullscreenPreview called when already in full screen preview");
    m_inFullscreenPreview = true;

    // Pick the screen on which the main window lies to use as the screen for the full screen preview
    const QScreen* screen = MainWindow::instance()->screen();
    const QRect screenGeometry = screen->geometry();

    // Unparent this and show it, which turns it into a free floating window
    // Also set style to frameless and disable resizing by user
    setParent(nullptr);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);
    setFixedSize(screenGeometry.size());
    move(QPoint(screenGeometry.x(), screenGeometry.y()));
    showMaximized();

    // This must be done after unparenting this widget above
    MainWindow::instance()->hide();
}

void EditorViewportWidget::StopFullscreenPreview()
{
    AZ_Assert(m_inFullscreenPreview, "EditorViewportWidget::StartFullscreenPreview called when not in full screen preview");
    m_inFullscreenPreview = false;

    // Unset frameless window flags
    setWindowFlag(Qt::FramelessWindowHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, false);

    // Unset fixed size (note that 50x50 is the minimum set in the constructor)
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setMinimumSize(50, 50);

    // Attach this viewport to the primary view pane (whose index is 0).
    if (CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout())
    {
        if (CLayoutViewPane* viewPane = layout->GetViewPaneByIndex(0))
        {
            // Force-reattach this viewport to its view pane by first detaching
            viewPane->DetachViewport();
            viewPane->AttachViewport(this);

            // Set the main widget of the layout, which causes this widgets size to be bound to the layout
            // and the viewport title bar to be displayed
            layout->SetMainWidget(viewPane);
        }
        else
        {
            AZ_Assert(false, "CRenderViewport: No view pane with ID 0 (primary view pane)");
        }
    }
    else
    {
        AZ_Assert(false, "CRenderViewport: No View Manager layout");
    }

    // Set this as the selected viewport
    GetIEditor()->GetViewManager()->SelectViewport(this);

    // Show this widget (setting flags may hide it)
    showNormal();

    // Show the main window
    MainWindow::instance()->show();
}

AZStd::optional<AzFramework::ViewportBorderPadding> EditorViewportWidget::GetViewportBorderPadding() const
{
    if (auto viewportEditorModeTracker = AZ::Interface<AzToolsFramework::ViewportEditorModeTrackerInterface>::Get())
    {
        auto viewportEditorModes = viewportEditorModeTracker->GetViewportEditorModes({ AzToolsFramework::GetEntityContextId() });
        if (viewportEditorModes->IsModeActive(AzToolsFramework::ViewportEditorMode::Focus) ||
            viewportEditorModes->IsModeActive(AzToolsFramework::ViewportEditorMode::Component))
        {
            AzFramework::ViewportBorderPadding viewportBorderPadding = {};
            viewportBorderPadding.m_top = AzToolsFramework::ViewportUi::ViewportUiTopBorderSize;
            viewportBorderPadding.m_left = AzToolsFramework::ViewportUi::ViewportUiLeftRightBottomBorderSize;
            viewportBorderPadding.m_right = AzToolsFramework::ViewportUi::ViewportUiLeftRightBottomBorderSize;
            viewportBorderPadding.m_bottom = AzToolsFramework::ViewportUi::ViewportUiLeftRightBottomBorderSize;
            return viewportBorderPadding;
        }
    }

    return AZStd::nullopt;
}

#include <moc_EditorViewportWidget.cpp>
