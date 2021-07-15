/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation filefov


#include "EditorDefs.h"

#include "RenderViewport.h"

// Qt
#include <QPainter>
#include <QScopedValueRollback>
#include <QCheckBox>
#include <QMessageBox>
#include <QTimer>

// AzCore
#include <AzCore/Console/Console.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/std/sort.h>
#include <AzCore/Component/ComponentApplicationBus.h>

// AzFramework
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Viewport/DisplayContextRequestBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#if defined(AZ_PLATFORM_WINDOWS)
#   include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>
#endif // defined(AZ_PLATFORM_WINDOWS)
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>                   // for AzFramework::InputDeviceMouse

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>


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

#include "ViewPane.h"
#include "CustomResolutionDlg.h"
#include "AnimationContext.h"
#include "Objects/SelectionGroup.h"
#include "Core/QtEditorApplication.h"

// ComponentEntityEditorPlugin
#include <Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h>

// LmbrCentral
#include <LmbrCentral/Rendering/EditorCameraCorrectionBus.h>

#include <QtGui/private/qhighdpiscaling_p.h>

#include <IEntityRenderState.h>
#include <IPhysics.h>
#include <IStatObj.h>

AZ_CVAR(
    bool, ed_visibility_use, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Enable/disable using the new IVisibilitySystem for Entity visibility determination");

CRenderViewport* CRenderViewport::m_pPrimaryViewport = nullptr;

#if AZ_TRAIT_OS_PLATFORM_APPLE
void StopFixedCursorMode();
void StartFixedCursorMode(QObject *viewport);
#endif

#define MAX_ORBIT_DISTANCE (2000.0f)
#define RENDER_MESH_TEST_DISTANCE (0.2f)
#define CURSOR_FONT_HEIGHT  8.0f
#define FORWARD_DIRECTION Vec3(0, 1, 0)

static const char TextCantCreateCameraNoLevel[] = "Cannot create camera when no level is loaded.";

class EditorEntityNotifications
    : public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , public AzToolsFramework::EditorContextMenuBus::Handler
{
public:
    EditorEntityNotifications(CRenderViewport& renderViewport)
        : m_renderViewport(renderViewport)
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();
    }

    ~EditorEntityNotifications() override
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusDisconnect();
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

    // AzToolsFramework::EditorContextMenu::Bus
    void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override
    {
        m_renderViewport.PopulateEditorGlobalContextMenu(menu, point, flags);
    }
private:
    CRenderViewport& m_renderViewport;
};

struct CRenderViewport::SScopedCurrentContext
{
    const CRenderViewport* m_viewport;
    CRenderViewport::SPreviousContext m_previousContext;

    explicit SScopedCurrentContext(const CRenderViewport* viewport)
        : m_viewport(viewport)
    {
        m_previousContext = viewport->SetCurrentContext();

        // During normal updates of RenderViewport the value of m_cameraSetForWidgetRenderingCount is expected to be 0.
        // This is to guarantee no loss in performance by tracking unnecessary calls to SetCurrentContext/RestorePreviousContext.
        // If some code makes additional calls to Pre/PostWidgetRendering then the assert will be triggered because
        // m_cameraSetForWidgetRenderingCount will be greater than 0.
        // There is a legitimate case where the counter can be greater than 0. This is when QtViewport is processing mouse callbacks.
        // QtViewport::MouseCallback() is surrounded by Pre/PostWidgetRendering and the m_processingMouseCallbacksCounter
        // tracks this specific case. If an update of a RenderViewport happens while processing the mouse callback,
        // for example when showing a QMessageBox, then both counters must match.
        AZ_Assert(viewport->m_cameraSetForWidgetRenderingCount == viewport->m_processingMouseCallbacksCounter,
            "SScopedCurrentContext constructor was called while viewport widget context is active "
            "- this is unnecessary");
    }

    ~SScopedCurrentContext()
    {
        m_viewport->RestorePreviousContext(m_previousContext);
    }
};

//////////////////////////////////////////////////////////////////////////
// CRenderViewport
//////////////////////////////////////////////////////////////////////////

CRenderViewport::CRenderViewport(const QString& name, QWidget* parent)
    : QtViewport(parent)
    , m_Camera(GetIEditor()->GetSystem()->GetViewCamera())
    , m_camFOV(gSettings.viewports.fDefaultFov)
    , m_defaultViewName(name)
{
    // need this to be set in order to allow for language switching on Windows
    setAttribute(Qt::WA_InputMethodEnabled);
    LockCameraMovement(true);

    CRenderViewport::SetViewTM(m_Camera.GetMatrix());
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

    setFocusPolicy(Qt::StrongFocus);

    Camera::EditorCameraRequestBus::Handler::BusConnect();
    m_editorEntityNotifications = AZStd::make_unique<EditorEntityNotifications>(*this);

    m_manipulatorManager = GetIEditor()->GetViewManager()->GetManipulatorManager();
    if (!m_pPrimaryViewport)
    {
        m_pPrimaryViewport = this;
    }

    m_hwnd = renderOverlayHWND();
}

//////////////////////////////////////////////////////////////////////////
CRenderViewport::~CRenderViewport()
{
    AzFramework::WindowNotificationBus::Event(m_hwnd, &AzFramework::WindowNotificationBus::Handler::OnWindowClosed);

    if (m_pPrimaryViewport == this)
    {
        m_pPrimaryViewport = nullptr;
    }

    AzFramework::WindowRequestBus::Handler::BusDisconnect();
    DisconnectViewportInteractionRequestBus();
    m_editorEntityNotifications.reset();
    Camera::EditorCameraRequestBus::Handler::BusDisconnect();
    OnDestroy();
    GetIEditor()->GetUndoManager()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
// CRenderViewport message handlers
//////////////////////////////////////////////////////////////////////////
int CRenderViewport::OnCreate()
{
    CreateRenderContext();

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::resizeEvent(QResizeEvent* event)
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
void CRenderViewport::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    // Do not call CViewport::OnPaint() for painting messages
    // FIXME: paintEvent() isn't the best place for such logic. Should listen to proper eNotify events and to the stuff there instead. (Repeats for other view port classes too).
    CGameEngine* ge = GetIEditor()->GetGameEngine();
    if ((ge && ge->IsLevelLoaded()) || (GetType() != ET_ViewportCamera))
    {
        setRenderOverlayVisible(true);
        m_isOnPaint = true;
        Update();
        m_isOnPaint = false;
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
void CRenderViewport::mousePressEvent(QMouseEvent* event)
{
    // There's a bug caused by having a mix of MFC and Qt where if the render viewport
    // had focus and then an MFC widget gets focus, Qt internally still thinks
    // that the widget that had focus before (the render viewport) has it now.
    // Because of this, Qt won't set the window that the viewport is in as the
    // focused widget, and the render viewport won't get keyboard input.
    // Forcing the window to activate should allow the window to take focus
    // and then the call to setFocus() will give it focus.
    // All so that the ::keyPressEvent() gets called.
    ActivateWindowAndSetFocus();

    GetIEditor()->GetViewManager()->SelectViewport(this);

    QtViewport::mousePressEvent(event);
}

AzToolsFramework::ViewportInteraction::MousePick CRenderViewport::BuildMousePickInternal(const QPoint& point) const
{
    using namespace AzToolsFramework::ViewportInteraction;

    MousePick mousePick;
    Vec3 from, dir;
    ViewToWorldRay(point, from, dir);
    mousePick.m_rayOrigin = LYVec3ToAZVec3(from);
    mousePick.m_rayDirection = LYVec3ToAZVec3(dir);
    mousePick.m_screenCoordinates = AzFramework::ScreenPoint(point.x(), point.y());
    return mousePick;
}

AzToolsFramework::ViewportInteraction::MousePick CRenderViewport::BuildMousePick(const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;

    PreWidgetRendering();
    const MousePick mousePick = BuildMousePickInternal(point);
    PostWidgetRendering();
    return mousePick;
}

AzToolsFramework::ViewportInteraction::MouseInteraction CRenderViewport::BuildMouseInteractionInternal(
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

AzToolsFramework::ViewportInteraction::MouseInteraction CRenderViewport::BuildMouseInteraction(
    const Qt::MouseButtons buttons, const Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;

    return BuildMouseInteractionInternal(
        BuildMouseButtons(buttons),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(WidgetToViewport(point)));
}

namespace RenderViewportUtil
{
    static bool JustAltHeld(const Qt::KeyboardModifiers modifiers)
    {
        return (modifiers & Qt::ShiftModifier) == 0
            && (modifiers & Qt::ControlModifier) == 0
            && (modifiers & Qt::AltModifier) != 0;
    }

    static bool NoModifiersHeld(const Qt::KeyboardModifiers modifiers)
    {
        return (modifiers & Qt::ShiftModifier) == 0
            && (modifiers & Qt::ControlModifier) == 0
            && (modifiers & Qt::AltModifier) == 0;
    }

    static bool AllowDolly(const Qt::KeyboardModifiers modifiers)
    {
        return JustAltHeld(modifiers);
    }

    static bool AllowOrbit(const Qt::KeyboardModifiers modifiers)
    {
        return JustAltHeld(modifiers);
    }

    static bool AllowPan(const Qt::KeyboardModifiers modifiers)
    {
        // begin pan with alt (inverted movement) or no modifiers
        return JustAltHeld(modifiers) || NoModifiersHeld(modifiers);
    }

    static bool InvertPan(const Qt::KeyboardModifiers modifiers)
    {
        return JustAltHeld(modifiers);
    }
} // namespace RenderViewportUtil


//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    if (!m_renderer)
    {
        return;
    }

    // Force the visible object cache to be updated - this is to ensure that
    // selection will work properly even if helpers are not being displayed,
    // in which case the cache is not updated every frame.
    if (m_displayContext.settings && !m_displayContext.settings->IsDisplayHelpers())
    {
        GetIEditor()->GetObjectManager()->ForceUpdateVisibleObjectCache(m_displayContext);
    }

    const auto scaledPoint = WidgetToViewport(point);
    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::Left),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    bool manipulatorInteraction = false;
    EditorInteractionSystemViewportSelectionRequestBus::EventResult(
        manipulatorInteraction, AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleMouseManipulatorInteraction,
        MouseInteractionEvent(mouseInteraction, MouseEvent::Down));

    if (!manipulatorInteraction)
    {
        if (RenderViewportUtil::AllowOrbit(modifiers))
        {
            m_bInOrbitMode = true;
            m_orbitTarget =
                GetViewTM().GetTranslation() + GetViewTM().TransformVector(FORWARD_DIRECTION) * m_orbitDistance;

            // mouse buttons are treated as keys as well
            if (m_pressedKeyState == KeyPressedState::AllUp)
            {
                m_pressedKeyState = KeyPressedState::PressedThisFrame;
            }

            m_mousePos = scaledPoint;
            m_prevMousePos = scaledPoint;

            HideCursor();
            CaptureMouse();

            // no further handling of left mouse button down
            return;
        }

        EditorInteractionSystemViewportSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleMouseViewportInteraction,
            MouseInteractionEvent(mouseInteraction, MouseEvent::Down));
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    // Convert point to position on terrain.
    if (!m_renderer)
    {
        return;
    }

    // Update viewports after done with actions.
    GetIEditor()->UpdateViews(eUpdateObjects);

    const auto scaledPoint = WidgetToViewport(point);

    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::Left),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    if (m_bInOrbitMode)
    {
        m_bInOrbitMode = false;

        ReleaseMouse();
        ShowCursor();
    }

    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
        AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, MouseEvent::Up));
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnLButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    const auto scaledPoint = WidgetToViewport(point);
    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::Left),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
        AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, MouseEvent::DoubleClick));
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    SetFocus();

    const auto scaledPoint = WidgetToViewport(point);
    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::Right),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
        AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, MouseEvent::Down));

    if (RenderViewportUtil::AllowDolly(modifiers))
    {
        m_bInZoomMode = true;
    }
    else
    {
        m_bInRotateMode = true;
    }

    // mouse buttons are treated as keys as well
    if (m_pressedKeyState == KeyPressedState::AllUp)
    {
        m_pressedKeyState = KeyPressedState::PressedThisFrame;
    }

    m_mousePos = scaledPoint;
    m_prevMousePos = m_mousePos;

    HideCursor();

    // we can't capture the mouse here, or it will stop the popup menu
    // when the mouse is released.
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnRButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    const auto scaledPoint = WidgetToViewport(point);
    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::Right),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
        AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, MouseEvent::Up));

    m_bInRotateMode = false;
    m_bInZoomMode = false;

    ReleaseMouse();

    if (!m_bInMoveMode)
    {
        ShowCursor();
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnMButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    const auto scaledPoint = WidgetToViewport(point);
    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::Middle),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    if (RenderViewportUtil::AllowPan(modifiers))
    {
        m_bInMoveMode = true;

        // mouse buttons are treated as keys as well
        if (m_pressedKeyState == KeyPressedState::AllUp)
        {
            m_pressedKeyState = KeyPressedState::PressedThisFrame;
        }

        m_mousePos = scaledPoint;
        m_prevMousePos = scaledPoint;

        HideCursor();
        CaptureMouse();
    }

    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
        AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, MouseEvent::Down));
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    const auto scaledPoint = WidgetToViewport(point);
    UpdateCurrentMousePos(scaledPoint);

    const auto tryRestoreMouse = [this]
    {
        // if we are currently looking (rotateMode) or dollying (zoomMode)
        // do not show the cursor on mouse up as rmb is still held
        if (!m_bInZoomMode && !m_bInRotateMode)
        {
            ReleaseMouse();
            ShowCursor();
        }
    };

    if (m_bInMoveMode)
    {
        m_bInMoveMode = false;
        tryRestoreMouse();
    }

    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::Middle),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
        AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, MouseEvent::Up));
}

void CRenderViewport::OnMouseMove(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    const auto scaledPoint = WidgetToViewport(point);

    const auto mouseInteraction = BuildMouseInteractionInternal(
        BuildMouseButtons(buttons),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
        AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, MouseEvent::Move));
}

void CRenderViewport::InjectFakeMouseMove(int deltaX, int deltaY, Qt::MouseButtons buttons)
{
    // this is required, otherwise the user will see the context menu
    OnMouseMove(Qt::NoModifier, buttons, QCursor::pos() + QPoint(deltaX, deltaY));
    // we simply move the prev mouse position, so the change will be picked up
    // by the next ProcessMouse call
    m_prevMousePos -= QPoint(deltaX, deltaY);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::ProcessMouse()
{
    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    const auto point = WidgetToViewport(mapFromGlobal(QCursor::pos()));

    if (point == m_prevMousePos)
    {
        return;
    }

    // specifically for the right mouse button click, which triggers rotate or zoom,
    // we can't capture the mouse until the user has moved the mouse, otherwise the
    // right click context menu won't popup
    if (!m_mouseCaptured && (m_bInZoomMode || m_bInRotateMode))
    {
        if ((point - m_mousePos).manhattanLength() > QApplication::startDragDistance())
        {
            CaptureMouse();
        }
    }

    float speedScale = GetCameraMoveSpeed();

    if (CheckVirtualKey(Qt::Key_Control))
    {
        speedScale *= gSettings.cameraFastMoveSpeed;
    }

    if (m_PlayerControl)
    {
        if (m_bInRotateMode)
        {
            f32 MousedeltaX = (m_mousePos.x() - point.x());
            f32 MousedeltaY = (m_mousePos.y() - point.y());
            m_relCameraRotZ += MousedeltaX;

            if (GetCameraInvertYRotation())
            {
                MousedeltaY = -MousedeltaY;
            }
            m_relCameraRotZ += MousedeltaX;
            m_relCameraRotX += MousedeltaY;

            ResetCursor();
        }
    }
    else if ((m_bInRotateMode && m_bInMoveMode) || m_bInZoomMode)
    {
        // Zoom.
        Matrix34 m = GetViewTM();

        Vec3 ydir = m.GetColumn1().GetNormalized();
        Vec3 pos = m.GetTranslation();

        const float posDelta = 0.2f * (m_prevMousePos.y() - point.y()) * speedScale;
        pos = pos - ydir * posDelta;
        m_orbitDistance = m_orbitDistance + posDelta;
        m_orbitDistance = fabs(m_orbitDistance);

        m.SetTranslation(pos);
        SetViewTM(m);

        ResetCursor();
    }
    else if (m_bInRotateMode)
    {
        Ang3 angles(-point.y() + m_prevMousePos.y(), 0, -point.x() + m_prevMousePos.x());
        angles = angles * 0.002f * GetCameraRotateSpeed();
        if (GetCameraInvertYRotation())
        {
            angles.x = -angles.x;
        }
        Matrix34 camtm = GetViewTM();
        Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(camtm));
        ypr.x += angles.z;
        ypr.y += angles.x;

        ypr.y = CLAMP(ypr.y, -1.5f, 1.5f);        // to keep rotation in reasonable range
        // In the recording mode of a custom camera, the z rotation is allowed.
        if (GetCameraObject() == nullptr || (!GetIEditor()->GetAnimation()->IsRecordMode() && !IsCameraObjectMove()))
        {
            ypr.z = 0;      // to have camera always upward
        }

        camtm = Matrix34(CCamera::CreateOrientationYPR(ypr), camtm.GetTranslation());
        SetViewTM(camtm);

        ResetCursor();
    }
    else if (m_bInMoveMode)
    {
        // Slide.
        Matrix34 m = GetViewTM();
        Vec3 xdir = m.GetColumn0().GetNormalized();
        Vec3 zdir = m.GetColumn2().GetNormalized();

        const auto modifiers = QGuiApplication::queryKeyboardModifiers();
        if (RenderViewportUtil::InvertPan(modifiers))
        {
            xdir = -xdir;
            zdir = -zdir;
        }

        Vec3 pos = m.GetTranslation();
        pos += 0.1f * xdir * (point.x() - m_prevMousePos.x()) * speedScale + 0.1f * zdir * (m_prevMousePos.y() - point.y()) * speedScale;
        m.SetTranslation(pos);
        SetViewTM(m, true);

        ResetCursor();
    }
    else if (m_bInOrbitMode)
    {
        Ang3 angles(-point.y() + m_prevMousePos.y(), 0, -point.x() + m_prevMousePos.x());
        angles = angles * 0.002f * GetCameraRotateSpeed();

        if (GetCameraInvertPan())
        {
            angles.z = -angles.z;
        }

        Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(GetViewTM()));
        ypr.x += angles.z;
        ypr.y = CLAMP(ypr.y, -1.5f, 1.5f);        // to keep rotation in reasonable range
        ypr.y += angles.x;

        Matrix33 rotateTM =  CCamera::CreateOrientationYPR(ypr);

        Vec3 src = GetViewTM().GetTranslation();
        Vec3 trg = m_orbitTarget;
        float fCameraRadius = (trg - src).GetLength();

        // Calc new source.
        src = trg - rotateTM * Vec3(0, 1, 0) * fCameraRadius;
        Matrix34 camTM = rotateTM;
        camTM.SetTranslation(src);

        SetViewTM(camTM);

        ResetCursor();
    }
}

void CRenderViewport::ResetCursor()
{
#ifdef AZ_PLATFORM_WINDOWS
    if (!gSettings.stylusMode)
    {
        const QPoint point = mapToGlobal(ViewportToWidget(m_prevMousePos));
        AzQtComponents::SetCursorPos(point);
    }
#endif

    // Recalculate the prev mouse pos even if we just reset to it to avoid compounding floating point math issues with DPI scaling
    m_prevMousePos = WidgetToViewport(mapFromGlobal(QCursor::pos()));
}

//////////////////////////////////////////////////////////////////////////
bool  CRenderViewport::event(QEvent* event)
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

    case QEvent::ShortcutOverride:
    {
        // since we respond to the following things, let Qt know so that shortcuts don't override us
        bool respondsToEvent = false;

        auto keyEvent = static_cast<QKeyEvent*>(event);
        bool manipulatorInteracting = false;
        AzToolsFramework::ManipulatorManagerRequestBus::EventResult(
            manipulatorInteracting,
            AzToolsFramework::g_mainManipulatorManagerId,
            &AzToolsFramework::ManipulatorManagerRequestBus::Events::Interacting);

        // If a manipulator is active, stop all shortcuts from working, except for the escape key, which cancels in some cases
        if ((keyEvent->key() != Qt::Key_Escape) &&  manipulatorInteracting)
        {
            respondsToEvent = true;
        }
        else
        {
            // In game mode we never want to be overridden by shortcuts
            if (GetIEditor()->IsInGameMode() && GetType() == ET_ViewportCamera)
            {
                respondsToEvent = true;
            }
            else
            {
                if (!(keyEvent->modifiers() & Qt::ControlModifier))
                {
                    switch (keyEvent->key())
                    {
                    case Qt::Key_Up:
                    case Qt::Key_W:
                    case Qt::Key_Down:
                    case Qt::Key_S:
                    case Qt::Key_Left:
                    case Qt::Key_A:
                    case Qt::Key_Right:
                    case Qt::Key_D:
                        respondsToEvent = true;
                        break;

                    default:
                        break;
                    }
                }
            }
        }

        if (respondsToEvent)
        {
            event->accept();
            return true;
        }

        // because we're doing keyboard grabs, we need to detect
        // when a shortcut matched so that we can track the buttons involved
        // in the shortcut, since the key released event won't be generated in that case
        ProcessKeyRelease(keyEvent);
    }
    break;
    default:
        // do nothing
        break;
    }

    return QtViewport::event(event);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::ResetContent()
{
    QtViewport::ResetContent();
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::UpdateContent(int flags)
{
    QtViewport::UpdateContent(flags);
    if (flags & eUpdateObjects)
    {
        m_bUpdateViewport = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::Update()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    if (Editor::EditorQtApplication::instance()->isMovingOrResizing())
    {
        return;
    }

    if (!m_renderer || m_rcClient.isEmpty() || GetIEditor()->IsInMatEditMode())
    {
        return;
    }

    if (!isVisible())
    {
        return;
    }

    // Only send the resize event if the render overlay is visible. This is to make sure
    // the native window has resized.
    if (m_windowResizedEvent && isRenderOverlayVisible())
    {
        AzFramework::WindowNotificationBus::Event(renderOverlayHWND(), &AzFramework::WindowNotificationBus::Handler::OnWindowResized, m_rcClient.width(), m_rcClient.height());
        m_windowResizedEvent = false;
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
                m_pPrimaryViewport = this;
            }
            else if (!m_bUpdateViewport) // Skip this viewport.
            {
                return;
            }
        }
    }

    if (CheckRespondToInput())
    {
        ProcessMouse();
        ProcessKeys();
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

            // draw debug visualizations
            {
                const AzFramework::DisplayContextRequestGuard displayContextGuard(m_displayContext);

                const AZ::u32 prevState = m_displayContext.GetState();
                m_displayContext.SetState(
                    e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOn | e_DepthTestOn);

                AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
                AzFramework::DebugDisplayRequestBus::Bind(
                    debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
                AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

                AzFramework::DebugDisplayRequests* debugDisplay =
                    AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

                AzFramework::EntityDebugDisplayEventBus::Broadcast(
                    &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
                    AzFramework::ViewportInfo{ GetViewportId() }, *debugDisplay);

                m_displayContext.SetState(prevState);
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
    if (!m_bRenderContextCreated)
    {
        if (!CreateRenderContext())
        {
            return;
        }
    }

    if (ed_visibility_use)
    {
        auto start = std::chrono::steady_clock::now();

        m_entityVisibilityQuery.UpdateVisibility(GetCameraState());
    }

    {
        SScopedCurrentContext context(this);

        m_renderer->SetClearColor(Vec3(0.4f, 0.4f, 0.4f));

        InitDisplayContext();

        OnRender();

        ProcessRenderLisneters(m_displayContext);

        m_displayContext.Flush2D();

        m_renderer->SwitchToNativeResolutionBackbuffer();

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

    QtViewport::Update();

    PopDisableRendering();
    m_bUpdateViewport = false;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::SetViewEntity(const AZ::EntityId& viewEntityId, bool lockCameraMovement)
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
void CRenderViewport::ResetToViewSourceType(const ViewSourceType& viewSourceType)
{
    LockCameraMovement(true);
    m_pCameraFOVVariable = nullptr;
    m_viewEntityId.SetInvalid();
    m_cameraObjectId = GUID_NULL;
    m_viewSourceType = viewSourceType;
    SetViewTM(GetViewTM());
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::PostCameraSet()
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
CBaseObject* CRenderViewport::GetCameraObject() const
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
void CRenderViewport::OnEditorNotifyEvent(EEditorNotifyEvent event)
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
                    m_previousContext = SetCurrentContext(deviceInfo->renderWidth, deviceInfo->renderHeight);
                    if (m_renderer->GetIStereoRenderer())
                    {
                        m_renderer->GetIStereoRenderer()->OnResolutionChanged();
                    }
                    SetActiveWindow();
                    SetFocus();
                    SetSelected(true);
                }
            }
            else
            {
                m_previousContext = SetCurrentContext();
            }
            SetCurrentCursor(STD_CURSOR_GAME);
        }
    }
    break;

    case eNotify_OnEndGameMode:
        if (GetIEditor()->GetViewManager()->GetGameViewport() == this)
        {
            SetCurrentCursor(STD_CURSOR_DEFAULT);
            if (m_renderer->GetCurrentContextHWND() != renderOverlayHWND())
            {
                // if this warning triggers it means that someone else (ie, some other part of the code)
                // called SetCurrentContext(...) on the renderer, probably did some rendering, but then either
                // failed to set the context back when done, or set it back to the wrong one.
                CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "RenderViewport render context was not correctly restored by someone else.");
            }
            RestorePreviousContext(m_previousContext);
            m_bInRotateMode = false;
            m_bInMoveMode = false;
            m_bInOrbitMode = false;
            m_bInZoomMode = false;

            RestoreViewportAfterGameMode();
        }
        break;

    case eNotify_OnCloseScene:
        SetDefaultCamera();
        break;

    case eNotify_OnBeginNewScene:
        PushDisableRendering();
        break;

    case eNotify_OnEndNewScene:
        PopDisableRendering();

        {
            // Default this to the size of default terrain in case there is no listener on the buss
            AZ::Aabb terrainAabb = AZ::Aabb::CreateFromMinMaxValues(0, 0, 32, 1024, 1024, 32);
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

    case eNotify_OnBeginTerrainCreate:
        PushDisableRendering();
        break;

    case eNotify_OnEndTerrainCreate:
        PopDisableRendering();

        {
            // Default this to the size of default terrain in case there is no listener on the buss
            AZ::Aabb terrainAabb = AZ::Aabb::CreateFromMinMaxValues(0, 0, 32, 1024, 1024, 32);
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
namespace {
    inline Vec3 NegY(const Vec3& v, float y)
    {
        return Vec3(v.x, y - v.y, v.z);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnRender()
{
    if (m_rcClient.isEmpty() || m_renderer->GetRenderType() == eRT_Null) // Null is crashing in CryEngine on macOS
    {
        // Even in null rendering, update the view camera.
        // This is necessary so that automated editor tests using the null renderer to test systems like dynamic vegetation
        // are still able to manipulate the current logical camera position, even if nothing is rendered.
        GetIEditor()->GetSystem()->SetViewCamera(m_Camera);
        return;
    }

    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

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
                lookThroughEntityCorrection, m_viewEntityId,
                &LmbrCentral::EditorCameraCorrectionRequests::GetTransformCorrection);
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

    CGameEngine* ge = GetIEditor()->GetGameEngine();

    bool levelIsDisplayable = (ge && ge->IsLevelLoaded() && GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady());

    //Handle scene render tasks such as gizmos and handles but only when not in VR
    if (!m_renderer->IsStereoEnabled())
    {
        DisplayContext& displayContext = m_displayContext;

        PreWidgetRendering();

        RenderAll();

        // Draw 2D helpers.
        TransformationMatrices backupSceneMatrices;
        m_renderer->Set2DMode(m_rcClient.right(), m_rcClient.bottom(), backupSceneMatrices);
        displayContext.SetState(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOn | e_DepthTestOn);

        // Display cursor string.
        RenderCursorString();

        if (gSettings.viewports.bShowSafeFrame)
        {
            UpdateSafeFrame();
            RenderSafeFrame();
        }

        const AzFramework::DisplayContextRequestGuard displayContextGuard(displayContext);

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(
            debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
        AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

        AzFramework::DebugDisplayRequests* debugDisplay =
            AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        AzFramework::ViewportDebugDisplayEventBus::Event(
            AzToolsFramework::GetEntityContextId(), &AzFramework::ViewportDebugDisplayEvents::DisplayViewport2d,
            AzFramework::ViewportInfo{ GetViewportId() }, *debugDisplay);

        m_renderer->Unset2DMode(backupSceneMatrices);

        PostWidgetRendering();
    }

    if (levelIsDisplayable)
    {
        m_renderer->SetViewport(0, 0, m_renderer->GetWidth(), m_renderer->GetHeight(), m_nCurViewportID);
    }
    else
    {
        ColorF viewportBackgroundColor(pow(71.0f / 255.0f, 2.2f), pow(71.0f / 255.0f, 2.2f), pow(71.0f / 255.0f, 2.2f));
        m_renderer->ClearTargetsLater(FRT_CLEAR_COLOR, viewportBackgroundColor);
        DrawBackground();
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RenderSelectionRectangle()
{
    if (m_selectedRect.isEmpty())
    {
        return;
    }

    Vec3 topLeft(m_selectedRect.left(), m_selectedRect.top(), 1);
    Vec3 bottomRight(m_selectedRect.right() +1, m_selectedRect.bottom() + 1, 1);

    m_displayContext.DepthTestOff();
    m_displayContext.SetColor(1, 1, 1, 0.4f);
    m_displayContext.DrawWireBox(topLeft, bottomRight);
    m_displayContext.DepthTestOn();
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::InitDisplayContext()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    // Draw all objects.
    DisplayContext& displayContext = m_displayContext;
    displayContext.settings = GetIEditor()->GetDisplaySettings();
    displayContext.view = this;
    displayContext.renderer = m_renderer;
    displayContext.box.min = Vec3(-100000.0f, -100000.0f, -100000.0f);
    displayContext.box.max = Vec3(100000.0f, 100000.0f, 100000.0f);
    displayContext.camera = &m_Camera;
    displayContext.flags = 0;

    if (!displayContext.settings->IsDisplayLabels() || !displayContext.settings->IsDisplayHelpers())
    {
        displayContext.flags |= DISPLAY_HIDENAMES;
    }

    if (displayContext.settings->IsDisplayLinks() && displayContext.settings->IsDisplayHelpers())
    {
        displayContext.flags |= DISPLAY_LINKS;
    }

    if (m_bDegradateQuality)
    {
        displayContext.flags |= DISPLAY_DEGRADATED;
    }

    if (displayContext.settings->GetRenderFlags() & RENDER_FLAG_BBOX)
    {
        displayContext.flags |= DISPLAY_BBOX;
    }

    if (displayContext.settings->IsDisplayTracks() && displayContext.settings->IsDisplayHelpers())
    {
        displayContext.flags |= DISPLAY_TRACKS;
        displayContext.flags |= DISPLAY_TRACKTICKS;
    }

    if (GetIEditor()->GetReferenceCoordSys() == COORDS_WORLD)
    {
        displayContext.flags |= DISPLAY_WORLDSPACEAXIS;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::PopulateEditorGlobalContextMenu(QMenu* /*menu*/, const AZ::Vector2& /*point*/, int /*flags*/)
{
    m_bInMoveMode = false;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RenderAll()
{
    // Draw all objects.
    DisplayContext& displayContext = m_displayContext;

    m_renderer->ResetToDefault();

    displayContext.SetState(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOn | e_DepthTestOn);
    GetIEditor()->GetObjectManager()->Display(displayContext);

    RenderSelectedRegion();

    RenderSnapMarker();

    if (gSettings.viewports.bShowGridGuide
        && GetIEditor()->GetDisplaySettings()->IsDisplayHelpers())
    {
        RenderSnappingGrid();
    }

    if (displayContext.settings->GetDebugFlags() & DBG_MEMINFO)
    {
        ProcessMemInfo mi;
        CProcessInfo::QueryMemInfo(mi);
        int MB = 1024 * 1024;
        QString str = QStringLiteral("WorkingSet=%1Mb, PageFile=%2Mb, PageFaults=%3").arg(mi.WorkingSet / MB).arg(mi.PagefileUsage / MB).arg(mi.PageFaultCount);
        m_renderer->TextToScreenColor(1, 1, 1, 0, 0, 1, str.toUtf8().data());
    }

    {
        const AzFramework::DisplayContextRequestGuard displayContextGuard(displayContext);

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(
            debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
        AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

        AzFramework::DebugDisplayRequests* debugDisplay =
            AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        // allow the override of in-editor visualization
        AzFramework::ViewportDebugDisplayEventBus::Event(
            AzToolsFramework::GetEntityContextId(), &AzFramework::ViewportDebugDisplayEvents::DisplayViewport,
            AzFramework::ViewportInfo{ GetViewportId() }, *debugDisplay);

        m_entityVisibilityQuery.DisplayVisibility(*debugDisplay);

        if (m_manipulatorManager != nullptr)
        {
            using namespace AzToolsFramework::ViewportInteraction;

            debugDisplay->DepthTestOff();
            m_manipulatorManager->DrawManipulators(
                *debugDisplay, GetCameraState(),
                BuildMouseInteractionInternal(
                    MouseButtons(TranslateMouseButtons(QGuiApplication::mouseButtons())),
                    BuildKeyboardModifiers(QGuiApplication::queryKeyboardModifiers()),
                    BuildMousePickInternal(WidgetToViewport(mapFromGlobal(QCursor::pos())))));
            debugDisplay->DepthTestOn();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::DrawAxis()
{
    AZ_Assert(m_cameraSetForWidgetRenderingCount > 0,
        "DrawAxis was called but viewport widget rendering was not set. PreWidgetRendering must be called before.");

    DisplayContext& dc = m_displayContext;

    // show axis only if draw helpers is activated
    if (!dc.settings->IsDisplayHelpers())
    {
        return;
    }

    Vec3 colX(1, 0, 0), colY(0, 1, 0), colZ(0, 0, 1), colW(1, 1, 1);
    Vec3 pos(50, 50, 0.1f); // Bottom-left corner

    float wx, wy, wz;
    m_renderer->UnProjectFromScreen(pos.x, pos.y, pos.z, &wx, &wy, &wz);
    Vec3 posInWorld(wx, wy, wz);
    float screenScale = GetScreenScaleFactor(posInWorld);
    float length = 0.03f * screenScale;
    float arrowSize = 0.02f * screenScale;
    float textSize = 1.1f;

    Vec3 x(length, 0, 0);
    Vec3 y(0, length, 0);
    Vec3 z(0, 0, length);

    int prevRState = dc.GetState();
    dc.DepthWriteOff();
    dc.DepthTestOff();
    dc.CullOff();
    dc.SetLineWidth(1);

    dc.SetColor(colX);
    dc.DrawLine(posInWorld, posInWorld + x);
    dc.DrawArrow(posInWorld + x * 0.9f, posInWorld + x, arrowSize);
    dc.SetColor(colY);
    dc.DrawLine(posInWorld, posInWorld + y);
    dc.DrawArrow(posInWorld + y * 0.9f, posInWorld + y, arrowSize);
    dc.SetColor(colZ);
    dc.DrawLine(posInWorld, posInWorld + z);
    dc.DrawArrow(posInWorld + z * 0.9f, posInWorld + z, arrowSize);

    dc.SetColor(colW);
    dc.DrawTextLabel(posInWorld + x, textSize, "x");
    dc.DrawTextLabel(posInWorld + y, textSize, "y");
    dc.DrawTextLabel(posInWorld + z, textSize, "z");

    dc.DepthWriteOn();
    dc.DepthTestOn();
    dc.CullOn();
    dc.SetState(prevRState);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::DrawBackground()
{
    DisplayContext& dc = m_displayContext;

    if (!dc.settings->IsDisplayHelpers())            // show gradient bg only if draw helpers are activated
    {
        return;
    }

    int heightVP = m_renderer->GetHeight() - 1;
    int widthVP = m_renderer->GetWidth() - 1;
    Vec3 pos(0, 0, 0);

    Vec3 x(widthVP, 0, 0);
    Vec3 y(0, heightVP, 0);

    float height = m_rcClient.height();

    Vec3 src =  NegY(pos, height);
    Vec3 trgx = NegY(pos + x, height);
    Vec3 trgy = NegY(pos + y, height);

    QColor topColor = palette().color(QPalette::Window);
    QColor bottomColor = palette().color(QPalette::Disabled, QPalette::WindowText);

    ColorB firstC(topColor.red(), topColor.green(), topColor.blue(), 255.0f);
    ColorB secondC(bottomColor.red(), bottomColor.green(), bottomColor.blue(), 255.0f);

    TransformationMatrices backupSceneMatrices;

    m_renderer->Set2DMode(m_rcClient.right(), m_rcClient.bottom(), backupSceneMatrices);
    m_displayContext.SetState(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOn | e_DepthTestOn);
    dc.DrawQuadGradient(src, trgx, pos + x, pos, secondC, firstC);
    m_renderer->Unset2DMode(backupSceneMatrices);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RenderCursorString()
{
    if (m_cursorStr.isEmpty())
    {
        return;
    }

    const auto point = WidgetToViewport(mapFromGlobal(QCursor::pos()));

    // Display hit object name.
    float col[4] = { 1, 1, 1, 1 };
    m_renderer->Draw2dLabel(point.x() + 12, point.y() + 4, 1.2f, col, false, "%s", m_cursorStr.toUtf8().data());

    if (!m_cursorSupplementaryStr.isEmpty())
    {
        float col2[4] = { 1, 1, 0, 1 };
        m_renderer->Draw2dLabel(point.x() + 12, point.y() + 4 + CURSOR_FONT_HEIGHT * 1.2f, 1.2f, col2, false, "%s", m_cursorSupplementaryStr.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::UpdateSafeFrame()
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
void CRenderViewport::RenderSafeFrame()
{
    RenderSafeFrame(m_safeFrame, 0.75f, 0.75f, 0, 0.8f);
    RenderSafeFrame(m_safeAction, 0, 0.85f, 0.80f, 0.8f);
    RenderSafeFrame(m_safeTitle, 0.80f, 0.60f, 0, 0.8f);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RenderSafeFrame(const QRect& frame, float r, float g, float b, float a)
{
    m_displayContext.SetColor(r, g, b, a);

    const int LINE_WIDTH = 2;
    for (int i = 0; i < LINE_WIDTH; i++)
    {
        Vec3 topLeft(frame.left() + i, frame.top() + i, 0);
        Vec3 bottomRight(frame.right() - i, frame.bottom() - i, 0);
        m_displayContext.DrawWireBox(topLeft, bottomRight);
    }
}

//////////////////////////////////////////////////////////////////////////
float CRenderViewport::GetAspectRatio() const
{
    return gSettings.viewports.fDefaultAspectRatio;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RenderSnapMarker()
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
static void OnMenuDisplayWireframe()
{
    ICVar* piVar(gEnv->pConsole->GetCVar("r_wireframe"));
    int nRenderMode = piVar->GetIVal();
    if (nRenderMode != R_WIREFRAME_MODE)
    {
        piVar->Set(R_WIREFRAME_MODE);
    }
    else
    {
        piVar->Set(R_SOLID_MODE);
    }
}

//////////////////////////////////////////////////////////////////////////
static void OnMenuTargetAspectRatio(float aspect)
{
    gSettings.viewports.fDefaultAspectRatio = aspect;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnMenuResolutionCustom()
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
void CRenderViewport::OnMenuCreateCameraEntityFromCurrentView()
{
    Camera::EditorCameraSystemRequestBus::Broadcast(&Camera::EditorCameraSystemRequests::CreateCameraEntityFromViewport);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnMenuSelectCurrentCamera()
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

static AzFramework::CameraState CameraStateFromCCamera(
    const CCamera& camera, const float fov, const float width, const float height)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    AzFramework::CameraState state;
    state.m_forward = LYVec3ToAZVec3(camera.GetViewdir());
    state.m_up = LYVec3ToAZVec3(camera.GetUp());
    state.m_side = state.m_forward.Cross(state.m_up);
    state.m_position = LYVec3ToAZVec3(camera.GetPosition());
    state.m_fovOrZoom = fov;
    state.m_nearClip = camera.GetNearPlane();
    state.m_farClip = camera.GetFarPlane();
    state.m_orthographic = false;
    state.m_viewportSize = AZ::Vector2(width, height);

    return state;
}

AzFramework::CameraState CRenderViewport::GetCameraState()
{
    return CameraStateFromCCamera(GetCamera(), GetFOV(), m_rcClient.width(), m_rcClient.height());
}

bool CRenderViewport::GridSnappingEnabled()
{
    return false;
}

float CRenderViewport::GridSize()
{
    return 0.0f;
}

bool CRenderViewport::ShowGrid()
{
    return false;
}

bool CRenderViewport::AngleSnappingEnabled()
{
    return false;
}

float CRenderViewport::AngleStep()
{
    return 0.0f;
}

AZ::Vector3 CRenderViewport::PickTerrain(const AzFramework::ScreenPoint& point)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    return LYVec3ToAZVec3(ViewToWorld(AzToolsFramework::ViewportInteraction::QPointFromScreenPoint(point), nullptr, true));
}

AZ::EntityId CRenderViewport::PickEntity(const AzFramework::ScreenPoint& point)
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

float CRenderViewport::TerrainHeight(const AZ::Vector2& position)
{
    return GetIEditor()->GetTerrainElevation(position.GetX(), position.GetY());
}

void CRenderViewport::FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntitiesOut)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    if (ed_visibility_use)
    {
        visibleEntitiesOut.assign(m_entityVisibilityQuery.Begin(), m_entityVisibilityQuery.End());
    }
    else
    {
        if (m_displayContext.GetView() == nullptr)
        {
            return;
        }

        const AZStd::vector<AZ::EntityId>& entityIdCache =
            m_displayContext.GetView()->GetVisibleObjectsCache()->GetEntityIdCache();

        visibleEntitiesOut.assign(entityIdCache.begin(), entityIdCache.end());
    }
}

AzFramework::ScreenPoint CRenderViewport::ViewportWorldToScreen(const AZ::Vector3& worldPosition)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    PreWidgetRendering();
    const AzFramework::ScreenPoint screenPosition =
        AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(WorldToView(AZVec3ToLYVec3(worldPosition)));
    PostWidgetRendering();

    return screenPosition;
}

bool CRenderViewport::IsViewportInputFrozen()
{
    return m_freezeViewportInput;
}

void CRenderViewport::FreezeViewportInput(bool freeze)
{
    m_freezeViewportInput = freeze;
}

QWidget* CRenderViewport::GetWidgetForViewportContextMenu()
{
    return this;
}

void CRenderViewport::BeginWidgetContext()
{
    PreWidgetRendering();
}

void CRenderViewport::EndWidgetContext()
{
    PostWidgetRendering();
}

bool CRenderViewport::ShowingWorldSpace()
{
    using namespace AzToolsFramework::ViewportInteraction;
    return BuildKeyboardModifiers(QGuiApplication::queryKeyboardModifiers()).Shift();
}

void CRenderViewport::SetWindowTitle(const AZStd::string& title)
{
    // Do not support the WindowRequestBus changing the editor window title
    AZ_UNUSED(title);
}

AzFramework::WindowSize CRenderViewport::GetClientAreaSize() const
{
    return AzFramework::WindowSize(m_rcClient.width(), m_rcClient.height());
}


void CRenderViewport::ResizeClientArea(AzFramework::WindowSize clientAreaSize)
{
    QWidget* window = this->window();
    window->resize(aznumeric_cast<int>(clientAreaSize.m_width), aznumeric_cast<int>(clientAreaSize.m_height));
}

bool CRenderViewport::GetFullScreenState() const
{
    // CRenderViewport does not currently support full screen.
    return false;
}

void CRenderViewport::SetFullScreenState([[maybe_unused]]bool fullScreenState)
{
    // CRenderViewport does not currently support full screen.
}

bool CRenderViewport::CanToggleFullScreenState() const
{
    // CRenderViewport does not currently support full screen.
    return false;
}

void CRenderViewport::ToggleFullScreenState()
{
    // CRenderViewport does not currently support full screen.
}

void CRenderViewport::ConnectViewportInteractionRequestBus()
{
    AzToolsFramework::ViewportInteraction::ViewportFreezeRequestBus::Handler::BusConnect(GetViewportId());
    AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusConnect(GetViewportId());
    AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequestBus::Handler::BusConnect(GetViewportId());
    m_viewportUi.ConnectViewportUiBus(GetViewportId());

    AzFramework::InputSystemCursorConstraintRequestBus::Handler::BusConnect();
}

void CRenderViewport::DisconnectViewportInteractionRequestBus()
{
    AzFramework::InputSystemCursorConstraintRequestBus::Handler::BusDisconnect();

    m_viewportUi.DisconnectViewportUiBus();
    AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequestBus::Handler::BusDisconnect();
    AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusDisconnect();
    AzToolsFramework::ViewportInteraction::ViewportFreezeRequestBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
static void ToggleBool(bool* variable, bool* disableVariableIfOn)
{
    *variable = !*variable;
    if (*variable && disableVariableIfOn)
    {
        *disableVariableIfOn = false;
    }
}

//////////////////////////////////////////////////////////////////////////
static void ToggleInt(int* variable)
{
    *variable = !*variable;
}

//////////////////////////////////////////////////////////////////////////
static void AddCheckbox(QMenu* menu, const QString& text, bool* variable, bool* disableVariableIfOn = nullptr)
{
    QAction* action = menu->addAction(text);
    QObject::connect(action, &QAction::triggered, action, [variable, disableVariableIfOn] { ToggleBool(variable, disableVariableIfOn);
        });
    action->setCheckable(true);
    action->setChecked(*variable);
}

//////////////////////////////////////////////////////////////////////////
static void AddCheckbox(QMenu* menu, const QString& text, int* variable)
{
    QAction* action = menu->addAction(text);
    QObject::connect(action, &QAction::triggered, action, [variable] { ToggleInt(variable);
        });
    action->setCheckable(true);
    action->setChecked(*variable);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnTitleMenu(QMenu* menu)
{
    const int nWireframe = gEnv->pConsole->GetCVar("r_wireframe")->GetIVal();
    QAction* action = menu->addAction(tr("Wireframe"));
    connect(action, &QAction::triggered, action, OnMenuDisplayWireframe);
    action->setCheckable(true);
    action->setChecked(nWireframe == R_WIREFRAME_MODE);

    const bool bDisplayLabels = GetIEditor()->GetDisplaySettings()->IsDisplayLabels();
    action = menu->addAction(tr("Labels"));
    connect(action, &QAction::triggered, this, [bDisplayLabels] {GetIEditor()->GetDisplaySettings()->DisplayLabels(!bDisplayLabels);
        });
    action->setCheckable(true);
    action->setChecked(bDisplayLabels);

    AddCheckbox(menu, tr("Show Safe Frame"), &gSettings.viewports.bShowSafeFrame);
    AddCheckbox(menu, tr("Show Construction Plane"), &gSettings.snap.constructPlaneDisplay);
    AddCheckbox(menu, tr("Show Trigger Bounds"), &gSettings.viewports.bShowTriggerBounds);
    AddCheckbox(menu, tr("Show Icons"), &gSettings.viewports.bShowIcons, &gSettings.viewports.bShowSizeBasedIcons);
    AddCheckbox(menu, tr("Show Size-based Icons"), &gSettings.viewports.bShowSizeBasedIcons, &gSettings.viewports.bShowIcons);
    AddCheckbox(menu, tr("Show Helpers of Frozen Objects"), &gSettings.viewports.nShowFrozenHelpers);

    if (!m_predefinedAspectRatios.IsEmpty())
    {
        QMenu* aspectRatiosMenu = menu->addMenu(tr("Target Aspect Ratio"));

        for (size_t i = 0; i < m_predefinedAspectRatios.GetCount(); ++i)
        {
            const QString& aspectRatioString = m_predefinedAspectRatios.GetName(i);
            QAction* aspectRatioAction = aspectRatiosMenu->addAction(aspectRatioString);
            connect(aspectRatioAction, &QAction::triggered, this, [i, this] { OnMenuTargetAspectRatio(m_predefinedAspectRatios.GetValue(i));
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
        connect(action, &QAction::triggered, this, &CRenderViewport::OnMenuCreateCameraEntityFromCurrentView);

        if (!gameEngine || !gameEngine->IsLevelLoaded())
        {
            action->setEnabled(false);
            action->setToolTip(tr(TextCantCreateCameraNoLevel));
            menu->setToolTipsVisible(true);
        }
    }

    if (!gameEngine || !gameEngine->IsLevelLoaded())
    {
        action->setEnabled(false);
        action->setToolTip(tr(TextCantCreateCameraNoLevel));
        menu->setToolTipsVisible(true);
    }

    if (GetCameraObject())
    {
        action = menu->addAction(tr("Select Current Camera"));
        connect(action, &QAction::triggered, this, &CRenderViewport::OnMenuSelectCurrentCamera);
    }

    // Add Cameras.
    bool bHasCameras = AddCameraMenuItems(menu);
    CRenderViewport* pFloatingViewport = nullptr;

    if (GetIEditor()->GetViewManager()->GetViewCount() > 1)
    {
        for (int i = 0; i < GetIEditor()->GetViewManager()->GetViewCount(); ++i)
        {
            CViewport* vp = GetIEditor()->GetViewManager()->GetView(i);
            if (!vp)
            {
                continue;
            }

            if (viewport_cast<CRenderViewport*>(vp) == nullptr)
            {
                continue;
            }

            if (vp->GetViewportId() == MAX_NUM_VIEWPORTS - 1)
            {
                menu->addSeparator();

                QMenu* floatViewMenu = menu->addMenu(tr("Floating View"));

                pFloatingViewport = (CRenderViewport*)vp;
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
                connect(customResolutionAction, &QAction::triggered, this, &CRenderViewport::OnMenuResolutionCustom);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::AddCameraMenuItems(QMenu* menu)
{
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }

    AddCheckbox(menu, "Lock Camera Movement", &m_bLockCameraMovement);
    menu->addSeparator();

    // Camera Sub menu
    QMenu* customCameraMenu = menu->addMenu(tr("Camera"));

    QAction* action = customCameraMenu->addAction("Editor Camera");
    action->setCheckable(true);
    action->setChecked(m_viewSourceType == ViewSourceType::None);
    connect(action, &QAction::triggered, this, &CRenderViewport::SetDefaultCamera);

    AZ::EBusAggregateResults<AZ::EntityId> getCameraResults;
    Camera::CameraBus::BroadcastResult(getCameraResults, &Camera::CameraRequests::GetCameras);

    const int numCameras = getCameraResults.values.size();

    // only enable if we're editing a sequence in Track View and have cameras in the level
    bool enableSequenceCameraMenu = (GetIEditor()->GetAnimation()->GetSequence() && numCameras);

    action = customCameraMenu->addAction(tr("Sequence Camera"));
    action->setCheckable(true);
    action->setChecked(m_viewSourceType == ViewSourceType::SequenceCamera);
    action->setEnabled(enableSequenceCameraMenu);
    connect(action, &QAction::triggered, this, &CRenderViewport::SetSequenceCamera);

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
    AzToolsFramework::EntityIdList selectedEntityList;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
    action->setCheckable(selectedEntityList.size() > 0 || m_viewSourceType == ViewSourceType::AZ_Entity);
    action->setEnabled(selectedEntityList.size() > 0 || m_viewSourceType == ViewSourceType::AZ_Entity);
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
void CRenderViewport::ResizeView(int width, int height)
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
void CRenderViewport::ToggleCameraObject()
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

void CRenderViewport::OnMouseWheel(Qt::KeyboardModifiers modifiers, short zDelta, const QPoint& point)
{
    using namespace AzToolsFramework::ViewportInteraction;
    using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

    if (GetIEditor()->IsInGameMode() || m_freezeViewportInput)
    {
        return;
    }

    const auto scaledPoint = WidgetToViewport(point);
    const auto mouseInteraction = BuildMouseInteractionInternal(
        MouseButtonsFromButton(MouseButton::None),
        BuildKeyboardModifiers(modifiers),
        BuildMousePick(scaledPoint));

    bool handled = false;
    MouseInteractionResult result = MouseInteractionResult::None;
    AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::EventResult(
        result, AzToolsFramework::GetEntityContextId(),
        &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
        MouseInteractionEvent(mouseInteraction, zDelta));

    handled = result != MouseInteractionResult::None;

    if (!handled)
    {
        Matrix34 m = GetViewTM();
        const Vec3 ydir = m.GetColumn1().GetNormalized();

        Vec3 pos = m.GetTranslation();

        const float posDelta = 0.01f * zDelta * gSettings.wheelZoomSpeed;
        pos += ydir * posDelta;
        m_orbitDistance = m_orbitDistance - posDelta;
        m_orbitDistance = fabs(m_orbitDistance);

        m.SetTranslation(pos);
        SetViewTM(m, true);

        QtViewport::OnMouseWheel(modifiers, zDelta, scaledPoint);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::SetCamera(const CCamera& camera)
{
    m_Camera = camera;
    SetViewTM(m_Camera.GetMatrix());
}

//////////////////////////////////////////////////////////////////////////
float CRenderViewport::GetCameraMoveSpeed() const
{
    return gSettings.cameraMoveSpeed;
}

//////////////////////////////////////////////////////////////////////////
float CRenderViewport::GetCameraRotateSpeed() const
{
    return gSettings.cameraRotateSpeed;
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::GetCameraInvertYRotation() const
{
    return gSettings.invertYRotation;
}

//////////////////////////////////////////////////////////////////////////
float CRenderViewport::GetCameraInvertPan() const
{
    return gSettings.invertPan;
}

//////////////////////////////////////////////////////////////////////////
CRenderViewport* CRenderViewport::GetPrimaryViewport()
{
    return m_pPrimaryViewport;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::focusOutEvent([[maybe_unused]] QFocusEvent* event)
{
    // if we lose focus, the keyboard map needs to be cleared immediately
    if (!m_keyDown.isEmpty())
    {
        m_keyDown.clear();

        releaseKeyboard();
    }
}

void CRenderViewport::keyPressEvent(QKeyEvent* event)
{
    // Special case Escape key and bubble way up to the top level parent so that it can cancel us out of any active tool
    // or clear the current selection
    if (event->key() == Qt::Key_Escape)
    {
        QCoreApplication::sendEvent(GetIEditor()->GetEditorMainWindow(), event);
    }

    // NOTE: we keep track of keypresses and releases explicitly because the OS/Qt will insert a slight delay between sending
    // keyevents when the key is held down. This is standard, but makes responding to key events for game style input silly
    // because we want the movement to be butter smooth.
    if (!event->isAutoRepeat())
    {
        if (m_keyDown.isEmpty())
        {
            grabKeyboard();
        }

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

void CRenderViewport::ProcessKeyRelease(QKeyEvent* event)
{
    if (!event->isAutoRepeat())
    {
        if (m_keyDown.contains(event->key()))
        {
            m_keyDown.remove(event->key());

            if (m_keyDown.isEmpty())
            {
                releaseKeyboard();
            }
        }
    }
}

void CRenderViewport::keyReleaseEvent(QKeyEvent* event)
{
    ProcessKeyRelease(event);

    QtViewport::keyReleaseEvent(event);
}

void CRenderViewport::SetViewTM(const Matrix34& viewTM, bool bMoveOnly)
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

        using namespace AzToolsFramework;
        ComponentEntityObjectRequestBus::Event(cameraObject, &ComponentEntityObjectRequestBus::Events::UpdatePreemptiveUndoCache);
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
void CRenderViewport::RenderSelectedRegion()
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

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::ProcessKeys()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    if (m_PlayerControl || GetIEditor()->IsInGameMode() || !CheckRespondToInput() || m_freezeViewportInput)
    {
        return;
    }

    //m_Camera.UpdateFrustum();
    Matrix34 m = GetViewTM();
    Vec3 ydir = m.GetColumn1().GetNormalized();
    Vec3 xdir = m.GetColumn0().GetNormalized();
    Vec3 zdir = m.GetColumn2().GetNormalized();

    Vec3 pos = GetViewTM().GetTranslation();

    float speedScale = AZStd::GetMin(
        60.0f * GetIEditor()->GetSystem()->GetITimer()->GetFrameTime(), 20.0f);

    speedScale *= GetCameraMoveSpeed();

    // Use the global modifier keys instead of our keymap. It's more reliable.
    const bool shiftPressed = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    const bool controlPressed = QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier;

    if (shiftPressed)
    {
        speedScale *= gSettings.cameraFastMoveSpeed;
    }

    if (controlPressed)
    {
        return;
    }

    bool bIsPressedSome = false;

    if (IsKeyDown(Qt::Key_Up) || IsKeyDown(Qt::Key_W))
    {
        // move forward
        bIsPressedSome = true;
        pos = pos + (speedScale * m_moveSpeed * ydir);
    }

    if (IsKeyDown(Qt::Key_Down) || IsKeyDown(Qt::Key_S))
    {
        // move backward
        bIsPressedSome = true;
        pos = pos - (speedScale * m_moveSpeed * ydir);
    }

    if (IsKeyDown(Qt::Key_Left) || IsKeyDown(Qt::Key_A))
    {
        // move left
        bIsPressedSome = true;
        pos = pos - (speedScale * m_moveSpeed * xdir);
    }

    if (IsKeyDown(Qt::Key_Right) || IsKeyDown(Qt::Key_D))
    {
        // move right
        bIsPressedSome = true;
        pos = pos + (speedScale * m_moveSpeed * xdir);
    }

    if (IsKeyDown(Qt::Key_E))
    {
        // move Up
        bIsPressedSome = true;
        pos = pos + (speedScale * m_moveSpeed * zdir);
    }

    if (IsKeyDown(Qt::Key_Q))
    {
        // move down
        bIsPressedSome = true;
        pos = pos - (speedScale * m_moveSpeed * zdir);
    }

    if (bIsPressedSome)
    {
        // Only change the keystate to pressed if it wasn't already marked in
        // a previous frame. Otherwise, the undo/redo stack will be all off
        // from what SetViewTM() does.
        if (m_pressedKeyState == KeyPressedState::AllUp)
        {
            m_pressedKeyState = KeyPressedState::PressedThisFrame;
        }

        m.SetTranslation(pos);
        SetViewTM(m, true);
    }

    bool mouseModifierKeysDown = ((QGuiApplication::mouseButtons() & (Qt::RightButton | Qt::MiddleButton)) != 0);

    if (!bIsPressedSome && !mouseModifierKeysDown)
    {
        m_pressedKeyState = KeyPressedState::AllUp;
    }
}

Vec3 CRenderViewport::WorldToView3D(const Vec3& wp, [[maybe_unused]] int nFlags) const
{
    AZ_Assert(m_cameraSetForWidgetRenderingCount > 0,
        "WorldToView3D was called but viewport widget rendering was not set. PreWidgetRendering must be called before.");

    Vec3 out(0, 0, 0);
    float x, y, z;

    m_renderer->ProjectToScreen(wp.x, wp.y, wp.z, &x, &y, &z);
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
QPoint CRenderViewport::WorldToView(const Vec3& wp) const
{
    AZ_Assert(m_cameraSetForWidgetRenderingCount > 0,
        "WorldToView was called but viewport widget rendering was not set. PreWidgetRendering must be called before.");

    QPoint p;
    float x, y, z;

    m_renderer->ProjectToScreen(wp.x, wp.y, wp.z, &x, &y, &z);
    if (_finite(x) || _finite(y))
    {
        p.rx() = (x / 100) * m_rcClient.width();
        p.ry() = (y / 100) * m_rcClient.height();
    }
    else
    {
        QPoint(0, 0);
    }

    return p;
}
//////////////////////////////////////////////////////////////////////////
QPoint CRenderViewport::WorldToViewParticleEditor(const Vec3& wp, int width, int height) const
{
    QPoint p;
    float x, y, z;

    m_renderer->ProjectToScreen(wp.x, wp.y, wp.z, &x, &y, &z);
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
Vec3 CRenderViewport::ViewToWorld(const QPoint& vp, bool* collideWithTerrain, bool onlyTerrain, bool bSkipVegetation, bool bTestRenderMesh, bool* collideWithObject) const
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    // Make sure we initialize the value if a pointer has been passed in
    if (collideWithTerrain != nullptr)
    {
        *collideWithTerrain = false;
    }

    // Make sure we initialize the value if a pointer has been passed in
    if (collideWithObject != nullptr)
{
        *collideWithObject = false;
    }

    if (!m_renderer)
    {
        return Vec3(0, 0, 0);
    }

    QRect rc = m_rcClient;

    Vec3 pos0;
    if (!m_Camera.Unproject(Vec3(vp.x(), rc.bottom() - vp.y(), 0), pos0))
    {
        return Vec3(0, 0, 0);
    }
    if (!IsVectorInValidRange(pos0))
    {
        pos0.Set(0, 0, 0);
    }

    Vec3 pos1;
    if (!m_Camera.Unproject(Vec3(vp.x(), rc.bottom() - vp.y(), 1), pos1))
    {
        return Vec3(0, 0, 0);
    }
    if (!IsVectorInValidRange(pos1))
    {
        pos1.Set(1, 0, 0);
    }

    const float maxDistance = 10000.f;

    Vec3 v = (pos1 - pos0);
    v = v.GetNormalized();
    v = v * maxDistance;

    if (!_finite(v.x) || !_finite(v.y) || !_finite(v.z))
    {
        return Vec3(0, 0, 0);
    }

    Vec3 colp = pos0 + 0.002f * v;

    AZ_UNUSED(vp)
    AZ_UNUSED(bTestRenderMesh)
    AZ_UNUSED(bSkipVegetation)
    AZ_UNUSED(bSkipVegetation)
    AZStd::optional<AZStd::pair<float, AZ::Vector3>> hitDistancePosition;

    if (!onlyTerrain && !GetIEditor()->IsTerrainAxisIgnoreObjects())
    {
        AzFramework::EntityContextId editorContextId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);

        AzFramework::RenderGeometry::RayRequest ray;
        ray.m_startWorldPosition = LYVec3ToAZVec3(pos0);
        ray.m_endWorldPosition = LYVec3ToAZVec3(pos0 + v);
        ray.m_onlyVisible = true;

        AzFramework::RenderGeometry::RayResult result;
        AzFramework::RenderGeometry::IntersectorBus::EventResult(result, editorContextId,
            &AzFramework::RenderGeometry::IntersectorInterface::RayIntersect, ray);

        if (result)
        {
            if (!hitDistancePosition || result.m_distance < hitDistancePosition->first)
            {
                hitDistancePosition = {result.m_distance, result.m_worldPosition};
                if (collideWithObject)
                {
                    *collideWithObject = true;
                }
            }
        }
    }

    if (hitDistancePosition)
    {
        colp = AZVec3ToLYVec3(hitDistancePosition->second);
    }


    return colp;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CRenderViewport::ViewToWorldNormal(const QPoint& vp, bool onlyTerrain, bool bTestRenderMesh)
{
    AZ_UNUSED(vp)
    AZ_UNUSED(bTestRenderMesh)

    AZ_Assert(m_cameraSetForWidgetRenderingCount > 0,
        "ViewToWorldNormal was called but viewport widget rendering was not set. PreWidgetRendering must be called before.");

    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    if (!m_renderer)
    {
        return Vec3(0, 0, 1);
    }

    QRect rc = m_rcClient;

    Vec3 pos0, pos1;
    float wx, wy, wz;
    m_renderer->UnProjectFromScreen(vp.x(), rc.bottom() - vp.y(), 0, &wx, &wy, &wz);
    if (!_finite(wx) || !_finite(wy) || !_finite(wz))
    {
        return Vec3(0, 0, 1);
    }
    pos0(wx, wy, wz);
    if (!IsVectorInValidRange(pos0))
    {
        pos0.Set(0, 0, 0);
    }

    m_renderer->UnProjectFromScreen(vp.x(), rc.bottom() - vp.y(), 1, &wx, &wy, &wz);
    if (!_finite(wx) || !_finite(wy) || !_finite(wz))
    {
        return Vec3(0, 0, 1);
    }
    pos1(wx, wy, wz);

    Vec3 v = (pos1 - pos0);
    if (!IsVectorInValidRange(pos1))
    {
        pos1.Set(1, 0, 0);
    }

    const float maxDistance = 2000.f;
    v = v.GetNormalized();
    v = v * maxDistance;

    if (!_finite(v.x) || !_finite(v.y) || !_finite(v.z))
    {
        return Vec3(0, 0, 1);
    }

    Vec3 colp(0, 0, 0);


    AZStd::optional<AZStd::pair<float, AZ::Vector3>> hitDistanceNormal;

    if (!onlyTerrain && !GetIEditor()->IsTerrainAxisIgnoreObjects())
    {
        AzFramework::EntityContextId editorContextId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);

        AzFramework::RenderGeometry::RayRequest ray;
        ray.m_startWorldPosition = LYVec3ToAZVec3(pos0);
        ray.m_endWorldPosition = LYVec3ToAZVec3(pos0 + v);
        ray.m_onlyVisible = true;

        AzFramework::RenderGeometry::RayResult result;
        AzFramework::RenderGeometry::IntersectorBus::EventResult(result, editorContextId,
            &AzFramework::RenderGeometry::IntersectorInterface::RayIntersect, ray);

        if (result)
        {
            if (!hitDistanceNormal || result.m_distance < hitDistanceNormal->first)
            {
                hitDistanceNormal = { result.m_distance, result.m_worldNormal };
            }
        }
    }

    return hitDistanceNormal ? AZVec3ToLYVec3(hitDistanceNormal->second) : Vec3(0, 0, 1);
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::AdjustObjectPosition(const ray_hit& hit, Vec3& outNormal, Vec3& outPos) const
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
bool CRenderViewport::RayRenderMeshIntersection(IRenderMesh*, const Vec3&, const Vec3&, Vec3&, Vec3&) const
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const
{
    AZ_Assert(m_cameraSetForWidgetRenderingCount > 0,
        "ViewToWorldRay was called but SScopedCurrentContext was not set at a higher scope! This means the camera for this call is incorrect.");

    if (!m_renderer)
    {
        return;
    }

    QRect rc = m_rcClient;

    Vec3 pos0, pos1;
    float wx, wy, wz;
    m_renderer->UnProjectFromScreen(vp.x(), rc.bottom() - vp.y(), 0, &wx, &wy, &wz);
    if (!_finite(wx) || !_finite(wy) || !_finite(wz))
    {
        return;
    }
    if (fabs(wx) > 1000000 || fabs(wy) > 1000000 || fabs(wz) > 1000000)
    {
        return;
    }
    pos0(wx, wy, wz);
    m_renderer->UnProjectFromScreen(vp.x(), rc.bottom() - vp.y(), 1, &wx, &wy, &wz);
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
float CRenderViewport::GetScreenScaleFactor(const Vec3& worldPoint) const
{
    float dist = m_Camera.GetPosition().GetDistance(worldPoint);
    if (dist < m_Camera.GetNearPlane())
    {
        dist = m_Camera.GetNearPlane();
    }
    return dist;
}
//////////////////////////////////////////////////////////////////////////
float CRenderViewport::GetScreenScaleFactor(const CCamera& camera, const Vec3& object_position)
{
    Vec3 camPos = camera.GetPosition();
    float dist = camPos.GetDistance(object_position);
    return dist;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnDestroy()
{
    DestroyRenderContext();
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::CheckRespondToInput() const
{
    if (!Editor::EditorQtApplication::IsActive())
    {
        return false;
    }

    if (!hasFocus())
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::HitTest(const QPoint& point, HitContext& hitInfo)
{
    hitInfo.camera = &m_Camera;
    hitInfo.pExcludedObject = GetCameraObject();
    return QtViewport::HitTest(point, hitInfo);
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::IsBoundsVisible(const AABB& box) const
{
    // If at least part of bbox is visible then its visible.
    return m_Camera.IsAABBVisible_F(AABB(box.min, box.max));
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::CenterOnSelection()
{
    if (!GetIEditor()->GetSelection()->IsEmpty())
    {
        // Get selection bounds & center
        CSelectionGroup* sel = GetIEditor()->GetSelection();
        AABB selectionBounds = sel->GetBounds();
        CenterOnAABB(selectionBounds);
    }
}

void CRenderViewport::CenterOnAABB(const AABB& aabb)
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
}

void CRenderViewport::CenterOnSliceInstance()
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
void CRenderViewport::SetFOV(float fov)
{
    if (m_pCameraFOVVariable)
    {
        m_pCameraFOVVariable->Set(fov);
    }
    else
    {
        m_camFOV = fov;
    }

    if (m_viewPane)
    {
        m_viewPane->OnFOVChanged(fov);
    }
}

//////////////////////////////////////////////////////////////////////////
float CRenderViewport::GetFOV() const
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

    if (m_pCameraFOVVariable)
    {
        float fov;
        m_pCameraFOVVariable->Get(fov);
        return fov;
    }
    else if (m_viewEntityId.IsValid())
    {
        float fov = AZ::RadToDeg(m_camFOV);
        Camera::CameraRequestBus::EventResult(fov, m_viewEntityId, &Camera::CameraComponentRequests::GetFov);
        return AZ::DegToRad(fov);
    }

    return m_camFOV;
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::CreateRenderContext()
{
    // Create context.
    if (m_renderer && !m_bRenderContextCreated)
    {
        m_bRenderContextCreated = true;

        AzFramework::WindowRequestBus::Handler::BusConnect(renderOverlayHWND());
        AzFramework::WindowSystemNotificationBus::Broadcast(&AzFramework::WindowSystemNotificationBus::Handler::OnWindowCreated, renderOverlayHWND());

        WIN_HWND oldContext = m_renderer->GetCurrentContextHWND();
        m_renderer->CreateContext(renderOverlayHWND());
        m_renderer->SetCurrentContext(oldContext); // restore prior context
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::DestroyRenderContext()
{
    // Destroy render context.
    if (m_renderer && m_bRenderContextCreated)
    {
        // Do not delete primary context.
        if (m_hwnd != m_renderer->GetHWND())
        {
            m_renderer->DeleteContext(m_hwnd);
        }
        m_bRenderContextCreated = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::SetDefaultCamera()
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
bool CRenderViewport::IsDefaultCamera() const
{
    return m_viewSourceType == ViewSourceType::None;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::SetSequenceCamera()
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
void  CRenderViewport::SetComponentCamera(const AZ::EntityId& entityId)
{
    ResetToViewSourceType(ViewSourceType::CameraComponent);
    SetViewEntity(entityId);
}

//////////////////////////////////////////////////////////////////////////
void  CRenderViewport::SetEntityAsCamera(const AZ::EntityId& entityId, bool lockCameraMovement)
{
    ResetToViewSourceType(ViewSourceType::AZ_Entity);
    SetViewEntity(entityId, lockCameraMovement);
}

void CRenderViewport::SetFirstComponentCamera()
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
void CRenderViewport::SetSelectedCamera()
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
bool CRenderViewport::IsSelectedCamera() const
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
void CRenderViewport::CycleCamera()
{
    // None -> Sequence -> LegacyCamera -> ... LegacyCamera -> CameraComponent -> ... CameraComponent -> None
    // AZ_Entity has been intentionally left out of the cycle for now.
    switch (m_viewSourceType)
    {
    case CRenderViewport::ViewSourceType::None:
    {
        SetFirstComponentCamera();
        break;
    }
    case CRenderViewport::ViewSourceType::SequenceCamera:
    {
        AZ_Error("CRenderViewport", false, "Legacy cameras no longer exist, unable to set sequence camera.");
        break;
    }
    case CRenderViewport::ViewSourceType::LegacyCamera:
    {
        AZ_Warning("CRenderViewport", false, "Legacy cameras no longer exist, using first found component camera instead.");
        SetFirstComponentCamera();
        break;
    }
    case CRenderViewport::ViewSourceType::CameraComponent:
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
    case CRenderViewport::ViewSourceType::AZ_Entity:
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

void CRenderViewport::SetViewFromEntityPerspective(const AZ::EntityId& entityId)
{
    SetViewAndMovementLockFromEntityPerspective(entityId, false);
}

void CRenderViewport::SetViewAndMovementLockFromEntityPerspective(const AZ::EntityId& entityId, bool lockCameraMovement)
{
    if (!m_ignoreSetViewFromEntityPerspective)
    {
        SetEntityAsCamera(entityId, lockCameraMovement);
    }
}

bool CRenderViewport::GetActiveCameraPosition(AZ::Vector3& cameraPos)
{
    cameraPos = LYVec3ToAZVec3(m_viewTM.GetTranslation());
    return true;
}

bool CRenderViewport::GetActiveCameraState(AzFramework::CameraState& cameraState)
{
    if (m_pPrimaryViewport == this)
    {
        if (GetIEditor()->IsInGameMode())
        {
            return false;
        }
        else
        {
            const auto& camera = GetCamera();
            cameraState = CameraStateFromCCamera(camera, GetFOV(), m_rcClient.width(), m_rcClient.height());
        }

        return true;
    }

    return false;
}

void CRenderViewport::OnStartPlayInEditor()
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
    // Force focus the render viewport, otherwise we don't receive keyPressEvents until the user first clicks a
    // mouse button. See also CRenderViewport::mousePressEvent for a deatiled description of the underlying bug.
    // We need to queue this up because we don't actually lose focus until sometime after this function returns.
    QTimer::singleShot(0, this, &CRenderViewport::ActivateWindowAndSetFocus);
}

void CRenderViewport::OnStopPlayInEditor()
{
    if (m_viewEntityIdCachedForEditMode.IsValid())
    {
        m_viewEntityId = m_viewEntityIdCachedForEditMode;
        m_viewEntityIdCachedForEditMode.SetInvalid();
    }
}

void CRenderViewport::ActivateWindowAndSetFocus()
{
    window()->activateWindow();
    setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RenderConstructionPlane()
{
    // noop
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RenderSnappingGrid()
{
    // noop
}

//////////////////////////////////////////////////////////////////////////
CRenderViewport::SPreviousContext CRenderViewport::SetCurrentContext(int newWidth, int newHeight) const
{
    SPreviousContext x;
    x.window = reinterpret_cast<HWND>(m_renderer->GetCurrentContextHWND());
    x.mainViewport = m_renderer->IsCurrentContextMainVP();
    x.width = m_renderer->GetCurrentContextViewportWidth();
    x.height = m_renderer->GetCurrentContextViewportHeight();
    x.rendererCamera = m_renderer->GetCamera();

    const float scale = CLAMP(gEnv->pConsole->GetCVar("r_ResolutionScale")->GetFVal(), MIN_RESOLUTION_SCALE, MAX_RESOLUTION_SCALE);
    const QSize newSize = WidgetToViewport(QSize(newWidth, newHeight)) * scale;

    // No way to query the requested Qt scale here, so do it this way for now
    float widthScale = aznumeric_cast<float>(newSize.width()) / aznumeric_cast<float>(newWidth);
    float heightScale = aznumeric_cast<float>(newSize.height()) / aznumeric_cast<float>(newHeight);

    m_renderer->SetCurrentContext(renderOverlayHWND());
    m_renderer->ChangeViewport(0, 0, newWidth, newHeight, true, widthScale, heightScale);
    m_renderer->SetCamera(m_Camera);

    return x;
}

//////////////////////////////////////////////////////////////////////////
CRenderViewport::SPreviousContext CRenderViewport::SetCurrentContext() const
{
    const auto r = rect();
    return SetCurrentContext(r.width(), r.height());
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::RestorePreviousContext(const SPreviousContext& x) const
{
    if (x.window && x.window != m_renderer->GetCurrentContextHWND())
    {
        m_renderer->SetCurrentContext(x.window);
        m_renderer->ChangeViewport(0, 0, x.width, x.height, x.mainViewport);
        m_renderer->SetCamera(x.rendererCamera);
    }
}

void CRenderViewport::PreWidgetRendering()
{
    // if we have not already set the render context for the viewport, do it now
    // based on the current state of the renderer/viewport, record the previous
    // context to restore afterwards
    if (m_cameraSetForWidgetRenderingCount == 0)
    {
        m_preWidgetContext = SetCurrentContext();
    }

    // keep track of how many times we've attempted to update the context
    m_cameraSetForWidgetRenderingCount++;
}

void CRenderViewport::PostWidgetRendering()
{
    if (m_cameraSetForWidgetRenderingCount > 0)
    {
        m_cameraSetForWidgetRenderingCount--;

        // unwinding - when the viewport context is no longer required,
        // restore the previous context when widget rendering first began
        if (m_cameraSetForWidgetRenderingCount == 0)
        {
            RestorePreviousContext(m_preWidgetContext);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::OnCameraFOVVariableChanged([[maybe_unused]] IVariable* var)
{
    if (m_viewPane)
    {
        m_viewPane->OnFOVChanged(GetFOV());
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::HideCursor()
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
void CRenderViewport::ShowCursor()
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

bool CRenderViewport::IsKeyDown(Qt::Key key) const
{
    return m_keyDown.contains(key);
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::PushDisableRendering()
{
    assert(m_disableRenderingCount >= 0);
    ++m_disableRenderingCount;
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::PopDisableRendering()
{
    assert(m_disableRenderingCount >= 1);
    --m_disableRenderingCount;
}

//////////////////////////////////////////////////////////////////////////
bool CRenderViewport::IsRenderingDisabled() const
{
    return m_disableRenderingCount > 0;
}

//////////////////////////////////////////////////////////////////////////
QPoint CRenderViewport::WidgetToViewport(const QPoint &point) const
{
    return point * WidgetToViewportFactor();
}

QPoint CRenderViewport::ViewportToWidget(const QPoint &point) const
{
    return point / WidgetToViewportFactor();
}

//////////////////////////////////////////////////////////////////////////
QSize CRenderViewport::WidgetToViewport(const QSize &size) const
{
    return size * WidgetToViewportFactor();
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::BeginUndoTransaction()
{
    PushDisableRendering();
}

//////////////////////////////////////////////////////////////////////////
void CRenderViewport::EndUndoTransaction()
{
    PopDisableRendering();
    Update();
}

void CRenderViewport::UpdateCurrentMousePos(const QPoint& newPosition)
{
    m_prevMousePos = m_mousePos;
    m_mousePos = newPosition;
}

void CRenderViewport::BuildDragDropContext(AzQtComponents::ViewportDragContext& context, const QPoint& pt)
{
    const auto scaledPoint = WidgetToViewport(pt);
    QtViewport::BuildDragDropContext(context, scaledPoint);
}

void* CRenderViewport::GetSystemCursorConstraintWindow() const
{
    AzFramework::SystemCursorState systemCursorState = AzFramework::SystemCursorState::Unknown;

    AzFramework::InputSystemCursorRequestBus::EventResult(
        systemCursorState,
        AzFramework::InputDeviceMouse::Id,
        &AzFramework::InputSystemCursorRequests::GetSystemCursorState);

    const bool systemCursorConstrained =
        (systemCursorState == AzFramework::SystemCursorState::ConstrainedAndHidden ||
         systemCursorState == AzFramework::SystemCursorState::ConstrainedAndVisible);

    return systemCursorConstrained ? renderOverlayHWND() : nullptr;
}

void CRenderViewport::RestoreViewportAfterGameMode()
{
    Matrix34 preGameModeViewTM = m_preGameModeViewTM;

    QString text =
        QString(
            tr("When leaving \" Game Mode \" the engine will automatically restore your camera position to the default position before you "
            "had entered Game mode.<br/><br/><small>If you dislike this setting you can always change this anytime in the global "
            "preferences.</small><br/><br/>"))
            .arg(EditorPreferencesGeneralRestoreViewportCameraSettingName);
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
        SetViewTM(m_gameTM);
    }
}

#include <moc_RenderViewport.cpp>
