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
#include <QBoxLayout>
#include <QCheckBox>
#include <QMessageBox>
#include <QPainter>
#include <QScopedValueRollback>
#include <QTimer>

// AzCore
#include <AzCore/Component/EntityId.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/VectorConversions.h>

// AzFramework
#include <AzFramework/Components/CameraBus.h>
#if defined(AZ_PLATFORM_WINDOWS)
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>
#endif // defined(AZ_PLATFORM_WINDOWS)
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h> // for AzFramework::InputDeviceMouse
#include <AzFramework/Viewport/ViewportControllerList.h>

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Viewport/ViewBookmarkLoaderInterface.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

// AtomToolsFramework
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>

// AzFramework
#include <AzFramework/Render/IntersectorInterface.h>

// Editor
#include "AnimationContext.h"
#include "Core/QtEditorApplication.h"
#include "CryCommon/MathConversion.h"
#include "CryEditDoc.h"
#include "CustomResolutionDlg.h"
#include "DisplaySettings.h"
#include "EditorPreferencesPageGeneral.h"
#include "EditorViewportCamera.h"
#include "EditorViewportSettings.h"
#include "GameEngine.h"
#include "Include/IDisplayViewport.h"
#include "LayoutWnd.h"
#include "MainWindow.h"
#include "ProcessInfo.h"
#include "Util/fastlib.h"
#include "ViewManager.h"
#include "ViewPane.h"
#include "ViewportManipulatorController.h"

// Atom
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/ViewportContextManager.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MatrixUtils.h>

#include <QtGui/private/qhighdpiscaling_p.h>

AZ_CVAR(
    bool, ed_visibility_logTiming, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Output the timing of the new IVisibilitySystem query");

EditorViewportWidget* EditorViewportWidget::m_pPrimaryViewport = nullptr;

#if AZ_TRAIT_OS_PLATFORM_APPLE
void StopFixedCursorMode();
void StartFixedCursorMode(QObject* viewport);
#endif

#define RENDER_MESH_TEST_DISTANCE (0.2f)
#define CURSOR_FONT_HEIGHT 8.0f

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

// helper to mark the view entity dirty that was moved using the 'Be this camera' functionality
static void MarkCameraEntityDirty(const AZ::EntityId entityId)
{
    using AzToolsFramework::ToolsApplicationRequests;

    AzToolsFramework::UndoSystem::URSequencePoint* undoBatch = nullptr;
    ToolsApplicationRequests::Bus::BroadcastResult(
        undoBatch, &ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "EditorCameraComponentEntityChange");
    ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
    ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::EndUndoBatch);
}

static void PopViewGroupForDefaultContext()
{
    auto* atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    if (!atomViewportRequests)
    {
        return;
    }

    auto viewSystem = AZ::RPI::ViewportContextRequests::Get();
    if (!viewSystem)
    {
        return;
    }

    if (auto viewGroup = viewSystem->GetCurrentViewGroup(viewSystem->GetDefaultViewportContextName()))
    {
        const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
        atomViewportRequests->PopViewGroup(contextName, viewGroup);
    }
}

static void PushViewGroupForDefaultContext()
{
    auto* atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    if (!atomViewportRequests)
    {
        return;
    }

    auto viewSystem = AZ::RPI::ViewportContextRequests::Get();
    if (!viewSystem)
    {
        return;
    }

    if (auto viewGroup = viewSystem->GetCurrentViewGroup(viewSystem->GetDefaultViewportContextName()))
    {
        const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
        atomViewportRequests->PushViewGroup(contextName, viewGroup);
    }
}

EditorViewportWidget::EditorViewportWidget(const QString& name, QWidget* parent)
    : QtViewport(parent)
    , m_defaultViewName(name)
    , m_renderViewport(nullptr) // m_renderViewport is initialized later, in SetViewportId
{
    // need this to be set in order to allow for language switching on Windows
    setAttribute(Qt::WA_InputMethodEnabled);

    if (GetIEditor()->GetViewManager()->GetSelectedViewport() == nullptr)
    {
        GetIEditor()->GetViewManager()->SelectViewport(this);
    }

    GetIEditor()->RegisterNotifyListener(this);

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

    AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusConnect();

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

    AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();

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
    if (!m_viewEntityId.IsValid())
    {
        SetFOV(GetFOV());
    }
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    // Do not call CViewport::OnPaint() for painting messages
    // FIXME: paintEvent() isn't the best place for such logic. Should listen to proper eNotify events and to the stuff there instead.
    // (Repeats for other view port classes too).
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
    mousePick.m_screenCoordinates = AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(point * devicePixelRatioF());
    const auto [origin, direction] = m_renderViewport->ViewportScreenToWorldRay(mousePick.m_screenCoordinates);
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
        AztfVi::BuildMouseButtons(buttons), AztfVi::BuildKeyboardModifiers(modifiers), BuildMousePick(WidgetToViewport(point)));
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::event(QEvent* event)
{
    switch (event->type())
    {
    case QEvent::WindowActivate:
        GetIEditor()->GetViewManager()->SelectViewport(this);
        // also kill the keys; if we alt-tab back to the viewport, or come back from the debugger, it's done (and there's no guarantee we'll
        // get the keyrelease event anyways)
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

    if (m_rcClient.isEmpty())
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

            // get debug display interface for the viewport
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, GetViewportId());
            AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

            AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

            // draw debug visualizations
            if (debugDisplay)
            {
                const AZ::u32 prevState = debugDisplay->GetState();
                debugDisplay->SetState(AzFramework::e_Mode3D | AzFramework::e_AlphaBlended | AzFramework::e_FillModeSolid | AzFramework::e_CullModeBack | AzFramework::e_DepthWriteOn | AzFramework::e_DepthTestOn);

                AzFramework::EntityDebugDisplayEventBus::Broadcast(
                    &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport, AzFramework::ViewportInfo{ GetViewportId() },
                    *debugDisplay);

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

    if (m_playInEditorState == PlayInEditorState::Stopping)
    {
        m_playInEditorState = PlayInEditorState::Editor;

        // Note that:
        // - this is assuming that the Atom camera components will share the same view ptr in editor as in game mode.
        // - if `m_viewEntityIdCachedForEditMode' is invalid, the camera before game mode was the default editor camera
        // - we must set the camera again when exiting game mode, because when rendering with track view, the editor camera gets set again
        SetViewFromEntityPerspective(m_viewEntityIdCachedForEditMode);
        m_viewEntityIdCachedForEditMode.SetInvalid();
    }

    // Prevents rendering recursion due to recursive Paint messages.
    if (IsRenderingDisabled())
    {
        return;
    }

    PushDisableRendering();

    // Render
    {
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

    if(!m_hasUpdatedVisibility)
    {
        auto start = std::chrono::steady_clock::now();

        m_entityVisibilityQuery.UpdateVisibility(m_renderViewport->GetCameraState());

        if (ed_visibility_logTiming)
        {
            auto stop = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = stop - start;
            AZ_Printf("Visibility", "FindVisibleEntities (new) - Duration: %f", diff);
        }

        m_hasUpdatedVisibility = true;
    }

    QtViewport::Update();

    PopDisableRendering();
    m_bUpdateViewport = false;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::PostCameraSet()
{
    // CryLegacy notify
    GetIEditor()->Notify(eNotify_CameraChanged);

    // Special case in the editor; if the camera is the default editor camera,
    // notify that the active view changed. In game mode, it is a hard error to not have
    // any cameras on the view stack!
    if (!m_viewEntityId.IsValid())
    {
        m_sendingOnActiveChanged = true;
        Camera::CameraNotificationBus::Broadcast(&Camera::CameraNotificationBus::Events::OnActiveViewChanged, AZ::EntityId());
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
void EditorViewportWidget::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
        {
            if (GetIEditor()->GetViewManager()->GetGameViewport() == this)
            {
                if (m_viewEntityId.IsValid())
                {
                    MarkCameraEntityDirty(m_viewEntityId);
                }

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
        // we restore the default viewport camera when closing the level to ensure if there is a pushed view group for a particular
        // editor camera component (view entity) it is popped/cleared to return to a default state when opening the next level
        SetDefaultCamera();
        m_renderViewport->SetScene(nullptr);
        break;

    case eNotify_OnEndLoad:
    case eNotify_OnEndCreate:
        UpdateScene();
        break;

    case eNotify_OnBeginNewScene:
        PushDisableRendering();
        break;

    case eNotify_OnEndNewScene:
        PopDisableRendering();
        UpdateScene();
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
    AZ_PROFILE_FUNCTION(Editor);

    m_hasUpdatedVisibility = false;

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
    m_debugDisplay->DepthTestOff();
    auto prevState = m_debugDisplay->GetState();
    m_debugDisplay->SetState(AzFramework::e_Mode3D | AzFramework::e_AlphaBlended | AzFramework::e_FillModeSolid | AzFramework::e_CullModeBack | AzFramework::e_DepthWriteOn | AzFramework::e_DepthTestOn);

    AzFramework::ViewportDebugDisplayEventBus::Event(
        AzToolsFramework::GetEntityContextId(), &AzFramework::ViewportDebugDisplayEvents::DisplayViewport2d,
        AzFramework::ViewportInfo{ GetViewportId() }, *m_debugDisplay);

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
float EditorViewportWidget::GetAspectRatio() const
{
    return gSettings.viewports.fDefaultAspectRatio;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::OnMenuCreateCameraEntityFromCurrentView()
{
    Camera::EditorCameraSystemRequestBus::Broadcast(&Camera::EditorCameraSystemRequests::CreateCameraEntityFromViewport);
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

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom, this);
    layout->setContentsMargins(QMargins());
    layout->addWidget(m_renderViewport);

    UpdateScene();

    if (m_pPrimaryViewport == this)
    {
        SetAsActiveViewport();
    }

    m_renderViewport->GetControllerList()->Add(AZStd::make_shared<SandboxEditor::ViewportManipulatorController>());

    m_editorModularViewportCameraComposer =
        AZStd::make_unique<SandboxEditor::EditorModularViewportCameraComposer>(AzFramework::ViewportId(id));
    m_renderViewport->GetControllerList()->Add(m_editorModularViewportCameraComposer->CreateModularViewportCameraController());

    m_editorViewportSettings.Connect(AzFramework::ViewportId(id));

    m_editorViewportSettingsCallbacks = SandboxEditor::CreateEditorViewportSettingsCallbacks();

    m_gridShowingHandler = SandboxEditor::GridShowingChangedEvent::Handler(
        [id](const bool showing)
        {
            AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Event(
                id, &AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Events::OnGridShowingChanged, showing);
        });
    m_editorViewportSettingsCallbacks->SetGridShowingChangedEvent(m_gridShowingHandler);

    m_gridSnappingHandler = SandboxEditor::GridSnappingChangedEvent::Handler(
        [id](const bool snapping)
        {
            AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Event(
                id, &AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Events::OnGridSnappingChanged, snapping);
        });
    m_editorViewportSettingsCallbacks->SetGridSnappingChangedEvent(m_gridSnappingHandler);

    m_angleSnappingHandler = SandboxEditor::AngleSnappingChangedEvent::Handler(
        [id](const bool snapping)
        {
            AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Event(
                id, &AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Events::OnAngleSnappingChanged, snapping);
        });
    m_editorViewportSettingsCallbacks->SetAngleSnappingChangedEvent(m_angleSnappingHandler);

    m_cameraSpeedScaleHandler = SandboxEditor::CameraSpeedScaleChangedEvent::Handler(
        [id](const float scale)
        {
            AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Event(
                id, &AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Events::OnCameraSpeedScaleChanged, scale);
        });
    m_editorViewportSettingsCallbacks->SetCameraSpeedScaleChangedEvent(m_cameraSpeedScaleHandler);

    m_perspectiveChangeHandler = SandboxEditor::PerspectiveChangedEvent::Handler(
        [this](const float fovRadians)
        {
            if (!m_viewEntityId.IsValid())
            {
                if (m_viewPane)
                {
                    m_viewPane->OnFOVChanged(fovRadians);
                }
                SetFOV(fovRadians);
            }
        });
    m_editorViewportSettingsCallbacks->SetPerspectiveChangedEvent(m_perspectiveChangeHandler);

    m_nearPlaneDistanceHandler = SandboxEditor::NearFarPlaneChangedEvent::Handler(
        [this]([[maybe_unused]] float nearPlaneDistance)
        {
            OnDefaultCameraNearFarChanged();
        });
    m_editorViewportSettingsCallbacks->SetNearPlaneDistanceChangedEvent(m_nearPlaneDistanceHandler);

    m_farPlaneDistanceHandler = SandboxEditor::NearFarPlaneChangedEvent::Handler(
        [this]([[maybe_unused]] float farPlaneDistance)
        {
            OnDefaultCameraNearFarChanged();
        });
    m_editorViewportSettingsCallbacks->SetFarPlaneDistanceChangedEvent(m_farPlaneDistanceHandler);
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
        QObject::connect(
            action, &QAction::triggered, action,
            [variable, disableVariableIfOn]
            {
                ToggleBool(variable, disableVariableIfOn);
            });
        action->setCheckable(true);
        action->setChecked(*variable);
    }

    void AddCheckbox(QMenu* menu, const QString& text, int* variable)
    {
        QAction* action = menu->addAction(text);
        QObject::connect(
            action, &QAction::triggered, action,
            [variable]
            {
                ToggleInt(variable);
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
    connect(
        action, &QAction::triggered, this,
        [bDisplayLabels]
        {
            GetIEditor()->GetDisplaySettings()->DisplayLabels(!bDisplayLabels);
        });
    action->setCheckable(true);
    action->setChecked(bDisplayLabels);

    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Construction Plane"), &gSettings.snap.constructPlaneDisplay);
    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Trigger Bounds"), &gSettings.viewports.bShowTriggerBounds);
    AZ::ViewportHelpers::AddCheckbox(menu, tr("Show Helpers of Frozen Objects"), &gSettings.viewports.nShowFrozenHelpers);

    if (!m_predefinedAspectRatios.IsEmpty())
    {
        QMenu* aspectRatiosMenu = menu->addMenu(tr("Target Aspect Ratio"));

        for (size_t i = 0; i < m_predefinedAspectRatios.GetCount(); ++i)
        {
            const QString& aspectRatioString = m_predefinedAspectRatios.GetName(i);
            QAction* aspectRatioAction = aspectRatiosMenu->addAction(aspectRatioString);
            connect(
                aspectRatioAction, &QAction::triggered, this,
                [i, this]
                {
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
    action->setChecked(!m_viewEntityId.IsValid());
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
        action->setChecked(m_viewEntityId == entityId && m_viewEntityId.IsValid());
        connect(
            action, &QAction::triggered, this,
            [this, entityId](bool isChecked)
            {
                if (isChecked)
                {
                    SetEntityAsCamera(entityId);
                }
                else
                {
                    SetDefaultCamera();
                }
            });
    }

    std::sort(
        additionalCameras.begin(), additionalCameras.end(),
        [](QAction* a1, QAction* a2)
        {
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
            AzFramework::RawInputNotificationBusWindows::Broadcast(
                &AzFramework::RawInputNotificationsWindows::OnRawInputCodeUnitUTF16Event, codeUnitUTF16);
            ++codeUnitsUTF16;
        }
    }
#endif // defined(AZ_PLATFORM_WINDOWS)
}

void EditorViewportWidget::SetViewTM(const Matrix34& camMatrix)
{
    GetCurrentAtomView()->SetCameraTransform(LYTransformToAZMatrix3x4(camMatrix));

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
    if (m_viewEntityId.IsValid())
    {
        // Check that the current view is the same view as the view entity view
        AZ::RPI::ViewPtr viewEntityView;
        AZ::RPI::ViewProviderBus::EventResult(viewEntityView, m_viewEntityId, &AZ::RPI::ViewProviderBus::Events::GetView);

        [[maybe_unused]] const bool isViewEntityCorrect = viewEntityView == GetCurrentAtomView();
        AZ_Error(
            "EditorViewportWidget", isViewEntityCorrect,
            "GetCurrentViewEntityId called while the current view is being changed. "
            "You may get inconsistent results if you make use of the returned entity ID. "
            "This is an internal error, please report it as a bug.");
    }

    return m_viewEntityId;
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

    auto ray =
        m_renderViewport->ViewportScreenToWorldRay(AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(vp * devicePixelRatioF()));

    const float maxDistance = 10000.f;
    Vec3 v = AZVec3ToLYVec3(ray.m_direction) * maxDistance;

    if (!_finite(v.x) || !_finite(v.y) || !_finite(v.z))
    {
        return Vec3(0, 0, 0);
    }

    Vec3 colp = AZVec3ToLYVec3(ray.m_origin) + 0.002f * v;

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
bool EditorViewportWidget::RayRenderMeshIntersection(
    IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal) const
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
    const AZ::Vector3 wp = m_renderViewport->ViewportScreenToWorld(AzFramework::ScreenPoint{ (int)sx, m_rcClient.bottom() - ((int)sy) });
    *px = wp.GetX();
    *py = wp.GetY();
    *pz = wp.GetZ();
}

void EditorViewportWidget::ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy) const
{
    AzFramework::ScreenPoint screenPosition = m_renderViewport->ViewportWorldToScreen(AZ::Vector3{ ptx, pty, ptz });
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
    return QtViewport::HitTest(point, hitInfo);
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::IsBoundsVisible(const AABB&) const
{
    AZ_Assert(false, "Not supported");
    return false;
}

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetFOV(const float fov)
{
    if (m_viewEntityId.IsValid())
    {
        Camera::CameraRequestBus::Event(m_viewEntityId, &Camera::CameraComponentRequests::SetFovRadians, fov);
    }
    else
    {
        auto viewSystem = AZ::RPI::ViewportContextRequests::Get();
        if (!viewSystem)
        {
            return;
        }

        if (auto viewGroup = viewSystem->GetCurrentViewGroup(viewSystem->GetDefaultViewportContextName()))
        {
            auto viewToClip = viewGroup->GetView()->GetViewToClipMatrix();
            AZ::SetPerspectiveMatrixFOV(viewToClip, fov, aznumeric_cast<float>(width()) / aznumeric_cast<float>(height()));
            viewGroup->GetView()->SetViewToClipMatrix(viewToClip);
        }
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
        auto viewSystem = AZ::RPI::ViewportContextRequests::Get();
        if (!viewSystem)
        {
            return AZ::Constants::HalfPi; // 90 degrees (default)
        }

        if (auto viewGroup = viewSystem->GetCurrentViewGroup(viewSystem->GetDefaultViewportContextName()))
        {
            return AZ::GetPerspectiveMatrixFOV(viewGroup->GetView()->GetViewToClipMatrix());
        }
    }

    return AZ::Constants::HalfPi; // 90 degrees (default)
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
            "EditorViewportWidget", Camera::EditorCameraViewRequestBus::FindFirstHandler(viewEntityId) != nullptr,
            "Internal logic error - active view changed to an entity which is not an editor camera. "
            "Please report this as a bug.");

        m_viewEntityId = viewEntityId;
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
    if (m_viewEntityId.IsValid())
    {
        // remove pushed view group for view entity (editor camera component in 'Be this camera' mode
        PopViewGroupForDefaultContext();
    }

    m_viewEntityId.SetInvalid();
    SetName(m_defaultViewName);

    // synchronize the configured editor viewport FOV to the default camera
    if (m_viewPane)
    {
        const float fovRadians = SandboxEditor::CameraDefaultFovRadians();
        m_viewPane->OnFOVChanged(fovRadians);
        SetFOV(fovRadians);
    }

    // update camera matrix according to near / far values
    SetDefaultCameraNearFar();

    PushViewGroupForDefaultContext();

    PostCameraSet();
}

void EditorViewportWidget::SetDefaultCameraNearFar()
{
    auto viewSystem = AZ::RPI::ViewportContextRequests::Get();
    if (!viewSystem)
    {
        return;
    }

    if (auto viewGroup = viewSystem->GetCurrentViewGroup(viewSystem->GetDefaultViewportContextName()))
    {
        auto viewToClip = viewGroup->GetView()->GetViewToClipMatrix();
        AZ::SetPerspectiveMatrixNearFar(viewToClip, SandboxEditor::CameraDefaultNearPlaneDistance(), SandboxEditor::CameraDefaultFarPlaneDistance());
        viewGroup->GetView()->SetViewToClipMatrix(viewToClip);
    }
}

void EditorViewportWidget::OnDefaultCameraNearFarChanged()
{
    if (!m_viewEntityId.IsValid())
    {
        SetDefaultCameraNearFar();
    }
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
void EditorViewportWidget::SetEntityAsCamera(const AZ::EntityId& entityId)
{
    SetViewFromEntityPerspective(entityId);
}

void EditorViewportWidget::SetFirstComponentCamera()
{
    AZ::EBusAggregateResults<AZ::EntityId> results;
    Camera::CameraBus::BroadcastResult(results, &Camera::CameraRequests::GetCameras);
    AZStd::sort_heap(results.values.begin(), results.values.end());
    AZ::EntityId entityId;
    if (!results.values.empty())
    {
        entityId = results.values[0];
    }
    SetEntityAsCamera(entityId);
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::SetSelectedCamera()
{
    AZ::EBusAggregateResults<AZ::EntityId> cameraList;
    Camera::CameraBus::BroadcastResult(cameraList, &Camera::CameraRequests::GetCameras);
    if (cameraList.values.size() > 0)
    {
        AzToolsFramework::EntityIdList selectedEntityList;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        for (const AZ::EntityId& entityId : selectedEntityList)
        {
            if (AZStd::find(cameraList.values.begin(), cameraList.values.end(), entityId) != cameraList.values.end())
            {
                SetEntityAsCamera(entityId);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool EditorViewportWidget::IsSelectedCamera() const
{
    AzToolsFramework::EntityIdList selectedEntityList;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    if (m_viewEntityId.IsValid() && !selectedEntityList.empty() &&
        AZStd::find(selectedEntityList.begin(), selectedEntityList.end(), m_viewEntityId) != selectedEntityList.end())
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void EditorViewportWidget::CycleCamera()
{
    // None (default editor camera) -> 1st CameraComponent (added editor camera component) -> ... next CameraComponent -> ... -> None
    if (!m_viewEntityId.IsValid())
    {
        SetFirstComponentCamera(); // None (default editor camera) -> select a first CameraComponent, if any
        return;
    }

    // Find the CameraComponent with the valid m_viewEntityId stored, if it still exists.
    AZ::EBusAggregateResults<AZ::EntityId> results;
    Camera::CameraBus::BroadcastResult(results, &Camera::CameraRequests::GetCameras);
    AZStd::sort_heap(results.values.begin(), results.values.end());
    auto&& currentCameraIterator = AZStd::find(results.values.begin(), results.values.end(), m_viewEntityId);
    if (currentCameraIterator != results.values.end())
    {
        if (++currentCameraIterator != results.values.end()) // Found -> check that a next one exists ... 
        {
            SetEntityAsCamera(*currentCameraIterator); // ... and then select it.
            return;
        }
    }
    // Go back to None (default editor camera) when the CameraComponent with stored m_viewEntityId
    // is the last one in the list, or was destroyed.
    SetDefaultCamera();
}

void EditorViewportWidget::SetViewFromEntityPerspective(const AZ::EntityId& entityId)
{
    // This is an editor event, so is only serviced during edit mode, not play game mode
    if (m_playInEditorState != PlayInEditorState::Editor)
    {
        AZ_Warning(
            "EditorViewportWidget", false, "Tried to change the editor camera during play game in editor; this is currently unsupported");
        return;
    }

    // when changing view, if an editor camera component (view entity) was in use, ensure we attempt to record an undo operation
    // in case the transform of the entity changed (if no changes occurred then no undo operation will be stored)
    if (m_viewEntityId.IsValid())
    {
        MarkCameraEntityDirty(m_viewEntityId);
    }

    if (entityId.IsValid())
    {
        // if we are switching between editor camera components (view entities) in the scene, if one is currently
        // assigned, ensure we pop it from the stack before assigning a new one
        if (m_viewEntityId.IsValid())
        {
            PopViewGroupForDefaultContext();
        }

        Camera::CameraRequestBus::Event(entityId, &Camera::CameraRequestBus::Events::MakeActiveView);
    }
    else
    {
        // note: SetDefaultCamera internally handles popping the current view group if an editor camera component
        // (view entity) is assigned
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

AZStd::optional<AZ::Transform> EditorViewportWidget::GetActiveCameraTransform()
{
    if (m_pPrimaryViewport == this)
    {
        if (GetIEditor()->IsInGameMode())
        {
            return m_renderViewport->GetViewportContext()->GetCameraTransform();
        }
        else
        {
            // Use viewTM, which is synced with the camera and guaranteed to be up-to-date
            return GetCurrentAtomView()->GetCameraTransform();
        }
    }
    return AZStd::nullopt;
}

AZStd::optional<float> EditorViewportWidget::GetCameraFoV()
{
    if (m_pPrimaryViewport == this)
    {
        return GetFOV();
    }
    return AZStd::nullopt;
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

void EditorViewportWidget::OnRootPrefabInstanceLoaded()
{
    // set the default camera on level/prefab load
    SetDefaultCamera();

    // set the camera position once we know the entire scene (level) has finished loading
    Matrix34 defaultView = Matrix34::CreateIdentity();
    // check to see if we have an existing last known location for this level
    auto* viewBookmarkInterface = AZ::Interface<AzToolsFramework::ViewBookmarkInterface>::Get();
    if (const AZStd::optional<AzToolsFramework::ViewBookmark> lastKnownLocationBookmark = viewBookmarkInterface->LoadLastKnownLocation();
        lastKnownLocationBookmark.has_value())
    {
        defaultView.SetTranslation(Vec3(lastKnownLocationBookmark->m_position));
        defaultView.SetRotation33(AZMatrix3x3ToLYMatrix3x3(AZ::Matrix3x3::CreateFromQuaternion(SandboxEditor::CameraRotation(
            AZ::DegToRad(lastKnownLocationBookmark->m_rotation.GetX()), AZ::DegToRad(lastKnownLocationBookmark->m_rotation.GetZ())))));
    }
    else
    {
        // set the default editor camera position and orientation if there was no last known location
        const AZ::Vector2 pitchYawDegrees = m_editorViewportSettings.DefaultEditorCameraOrientation();
        defaultView.SetTranslation(Vec3(m_editorViewportSettings.DefaultEditorCameraPosition()));
        defaultView.SetRotation33(AZMatrix3x3ToLYMatrix3x3(AZ::Matrix3x3::CreateFromQuaternion(
            SandboxEditor::CameraRotation(AZ::DegToRad(pitchYawDegrees.GetX()), AZ::DegToRad(pitchYawDegrees.GetY())))));
    }

    SetViewTM(defaultView);
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
            &AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId, m_viewEntityId, runtimeEntityId);

        m_viewEntityId = runtimeEntityId;
    }
}

void EditorViewportWidget::OnStopPlayInEditor()
{
    m_playInEditorState = PlayInEditorState::Stopping;
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
    // Even when the mouse cursor is not in a constrained mode, we still return the viewport as the constraint window,
    // so that the engine's mouse coordinates will be normalized to the editor viewport rather than the entire application window.
    // This ensures that viewport mouse interactions are in the correct 2D coordinate space, for example when using ImGuiManager's
    // debug tools.
    return renderOverlayHWND();
}

void EditorViewportWidget::BuildDragDropContext(
    AzQtComponents::ViewportDragContext& context, const AzFramework::ViewportId viewportId, const QPoint& point)
{
    QtViewport::BuildDragDropContext(context, viewportId, point);
}

void EditorViewportWidget::RestoreViewportAfterGameMode()
{
    Matrix34 preGameModeViewTM = m_preGameModeViewTM;

    QString text = QString(
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
        AzFramework::InputSystemCursorRequestBus::Event(
            AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
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
            auto viewportContext = m_renderViewport->GetViewportContext();
            AZ::RPI::SceneNotificationBus::Handler::BusConnect(viewportContext->GetRenderScene()->GetId());

            // Don't enable the render pipeline until a level has been loaded
            // Also show/hide the RenderViewportWidget accordingly so that we get the
            // expected gradient background when no level is loaded
            auto renderPipeline = viewportContext->GetCurrentPipeline();
            if (renderPipeline)
            {
                if (GetIEditor()->IsLevelLoaded())
                {
                    m_renderViewport->show();
                    renderPipeline->AddToRenderTick();
                }
                else
                {
                    m_renderViewport->hide();
                    renderPipeline->RemoveFromRenderTick();
                }
            }
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
            viewportContextManager->PopViewGroup(defaultContextName, viewportContext->GetViewGroup());
            viewportContextManager->RenameViewportContext(viewportContext, defaultContextName);
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
            viewportContextManager->RenameViewportContext(viewportContext, defaultContextName);
            viewportContextManager->PushViewGroup(defaultContextName, viewportContext->GetViewGroup());
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

AZ::Vector2 EditorViewportSettings::DefaultEditorCameraOrientation() const
{
    return SandboxEditor::CameraDefaultEditorOrientation();
}

bool EditorViewportSettings::IconsVisible() const
{
    return AzToolsFramework::IconsVisible();
}

bool EditorViewportSettings::HelpersVisible() const
{
    return AzToolsFramework::HelpersVisible();
}

bool EditorViewportSettings::OnlyShowHelpersForSelectedEntities() const
{
    return AzToolsFramework::OnlyShowHelpersForSelectedEntities();
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
