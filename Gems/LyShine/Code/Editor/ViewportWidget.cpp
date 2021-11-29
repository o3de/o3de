/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "UiCanvasComponent.h"

#include "EditorDefs.h"
#include "Settings.h"
#include <AzCore/std/containers/map.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

#include <LyShine/Bus/UiEditorCanvasBus.h>
#include <LyShine/Draw2d.h>

#include "LyShine.h"
#include "UiRenderer.h"
#include "ViewportNudge.h"
#include "ViewportPivot.h"
#include "ViewportSnap.h"
#include "ViewportElement.h"
#include "RulerWidget.h"
#include "CanvasHelpers.h"
#include "AssetDropHelpers.h"
#include "QtHelpers.h"
#include <QtGui/private/qhighdpiscaling_p.h>

#include <QGridLayout>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#define UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_ELEMENT_BORDERS_KEY         "ViewportWidget::m_drawElementBordersFlags"
#define UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_ELEMENT_BORDERS_DEFAULT     ( ViewportWidget::DrawElementBorders_Unselected )

#define UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_RULERS_KEY                  "ViewportWidget::m_rulersVisible"
#define UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_RULERS_DEFAULT              ( false )

#define UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_GUIDES_KEY                  "ViewportWidget::m_guidesVisible"
#define UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_GUIDES_DEFAULT              ( false )

namespace
{
    uint32 GetDrawElementBordersFlags()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        uint32 result = settings.value(UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_ELEMENT_BORDERS_KEY,
                UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_ELEMENT_BORDERS_DEFAULT).toInt();

        settings.endGroup();

        return result;
    }

    // Persistence.
    void SetDrawElementBordersFlags(uint32 flags)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        settings.setValue(UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_ELEMENT_BORDERS_KEY,
            flags);

        settings.endGroup();
    }

    bool GetPersistentRulerVisibility()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        bool result = settings.value(UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_RULERS_KEY,
                UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_RULERS_DEFAULT).toBool();

        settings.endGroup();

        return result;
    }

    // Persistence.
    void SetPersistentRulerVisibility(bool rulersVisible)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        settings.setValue(UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_RULERS_KEY,
            rulersVisible);

        settings.endGroup();
    }

    bool GetPersistentGuideVisibility()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        bool result = settings.value(UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_GUIDES_KEY,
                UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_GUIDES_DEFAULT).toBool();

        settings.endGroup();

        return result;
    }

    // Persistence.
    void SetPersistentGuideVisibility(bool guidesVisible)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        settings.setValue(UICANVASEDITOR_SETTINGS_VIEWPORTWIDGET_DRAW_GUIDES_KEY,
            guidesVisible);

        settings.endGroup();
    }

    // Map Qt event key codes to the game input system keyboard codes
    const AzFramework::InputChannelId* MapQtKeyToAzInputChannelId(int qtKey)
    {
        // The UI runtime only cares about a few special keys
        switch (qtKey)
        {
            case Qt::Key_Tab: return &AzFramework::InputDeviceKeyboard::Key::EditTab;
            case Qt::Key_Backspace: return &AzFramework::InputDeviceKeyboard::Key::EditBackspace;
            case Qt::Key_Return: return &AzFramework::InputDeviceKeyboard::Key::EditEnter;
            case Qt::Key_Enter: return &AzFramework::InputDeviceKeyboard::Key::EditEnter;
            case Qt::Key_Delete: return &AzFramework::InputDeviceKeyboard::Key::NavigationDelete;
            case Qt::Key_Left: return &AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft;
            case Qt::Key_Up: return &AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp;
            case Qt::Key_Right: return &AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight;
            case Qt::Key_Down: return &AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown;
            case Qt::Key_Home: return &AzFramework::InputDeviceKeyboard::Key::NavigationHome;
            case Qt::Key_End: return &AzFramework::InputDeviceKeyboard::Key::NavigationEnd;
            default: return nullptr;
        }
    }

    // Map Qt event modifiers to the AzFramework input system modifiers
    AzFramework::ModifierKeyMask MapQtModifiersToAzInputModifierKeys(Qt::KeyboardModifiers qtMods)
    {
        int modifiers = static_cast<int>(AzFramework::ModifierKeyMask::None);

        if (qtMods & Qt::ShiftModifier)
        {
            modifiers |= static_cast<int>(AzFramework::ModifierKeyMask::ShiftAny);
        }

        if (qtMods & Qt::ControlModifier)
        {
            modifiers |= static_cast<int>(AzFramework::ModifierKeyMask::CtrlAny);
        }

        if (qtMods & Qt::AltModifier)
        {
            modifiers |= static_cast<int>(AzFramework::ModifierKeyMask::AltAny);
        }

        return static_cast<AzFramework::ModifierKeyMask>(modifiers);
    }

    bool HandleCanvasInputEvent(AZ::EntityId canvasEntityId,
                                const AzFramework::InputChannel::Snapshot& inputSnapshot,
                                const AZ::Vector2* viewportPos = nullptr,
                                const AzFramework::ModifierKeyMask activeModifierKeys = AzFramework::ModifierKeyMask::None)
    {
        bool handled = false;
        EBUS_EVENT_ID_RESULT(handled, canvasEntityId, UiCanvasBus, HandleInputEvent, inputSnapshot, viewportPos, activeModifierKeys);

        // Execute events that have been queued during the input event handler
        gEnv->pLyShine->ExecuteQueuedEvents();

        return handled;
    }

    bool HandleCanvasTextEvent(AZ::EntityId canvasEntityId, const AZStd::string& textUTF8)
    {
        bool handled = false;
        EBUS_EVENT_ID_RESULT(handled, canvasEntityId, UiCanvasBus, HandleTextEvent, textUTF8);

        // Execute events that have been queued during the input event handler
        gEnv->pLyShine->ExecuteQueuedEvents();

        return handled;
    }

} // anonymous namespace.

ViewportWidget::ViewportWidget(EditorWindow* parent)
    : AtomToolsFramework::RenderViewportWidget(parent)
    , m_editorWindow(parent)
    , m_viewportInteraction(new ViewportInteraction(m_editorWindow))
    , m_viewportAnchor(new ViewportAnchor())
    , m_viewportHighlight(new ViewportHighlight())
    , m_viewportBackground(new ViewportCanvasBackground())
    , m_viewportPivot(new ViewportPivot())
    , m_drawElementBordersFlags(GetDrawElementBordersFlags())
    , m_refreshRequested(true)
    , m_canvasRenderIsEnabled(true)
    , m_updateTimer(this)
    , m_previewCanvasScale(1.0f)
    , m_rulersVisible(GetPersistentRulerVisibility())
    , m_guidesVisible(GetPersistentGuideVisibility())
{
    setAcceptDrops(true);

    InitUiRenderer();

    SetupShortcuts();
    installEventFilter(m_editorWindow);

    // Setup a timer for the maximum refresh rate we want.
    // Refresh is actually triggered by interaction events and by the IdleUpdate. This avoids the UI
    // Editor slowing down the main editor when no UI interaction is occurring.
    QObject::connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(RefreshTick()));
    const int kUpdateIntervalInMillseconds = 1000 / 60;   // 60 Hz
    m_updateTimer.start(kUpdateIntervalInMillseconds);

    // listen to the editor window for changes in mode. When in preview mode hide the rulers.
    QObject::connect(parent,
        &EditorWindow::EditorModeChanged,
        [this](UiEditorMode mode)
        {
            if (mode == UiEditorMode::Preview)
            {
                m_rulersVisible = false;
            }
            else
            {
                m_rulersVisible = GetPersistentRulerVisibility();
            }

            ApplyRulerVisibility();
        });

    FontNotificationBus::Handler::BusConnect();
    AZ::TickBus::Handler::BusConnect();
    AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(GetCurrentContextName());
}

ViewportWidget::~ViewportWidget()
{
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    FontNotificationBus::Handler::BusDisconnect();
    AZ::TickBus::Handler::BusDisconnect();
    LyShinePassDataRequestBus::Handler::BusDisconnect();
    AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();

    removeEventFilter(m_editorWindow);

    m_uiRenderer.reset();

    // Notify LyShine that this is no longer a valid UiRenderer.
    // Only one viewport/renderer is currently supported in the UI Editor
    CLyShine* lyShine = static_cast<CLyShine*>(gEnv->pLyShine);
    lyShine->SetUiRendererForEditor(nullptr);
}

void ViewportWidget::InitUiRenderer()
{
    m_uiRenderer = AZStd::make_shared<UiRenderer>(GetViewportContext());

    // Notify LyShine that this is the UiRenderer to be used for rendering
    // UI canvases that are loaded in the UI Editor.
    // Only one viewport/renderer is currently supported in the UI Editor
    CLyShine* lyShine = static_cast<CLyShine*>(gEnv->pLyShine);
    lyShine->SetUiRendererForEditor(m_uiRenderer);

    m_draw2d = AZStd::make_shared<CDraw2d>(GetViewportContext());

    LyShinePassDataRequestBus::Handler::BusConnect(GetViewportContext()->GetRenderScene()->GetId());
}

ViewportInteraction* ViewportWidget::GetViewportInteraction()
{
    return m_viewportInteraction.get();
}

bool ViewportWidget::IsDrawingElementBorders(uint32 flags) const
{
    return (m_drawElementBordersFlags & flags) ? true : false;
}

void ViewportWidget::ToggleDrawElementBorders(uint32 flags)
{
    m_drawElementBordersFlags ^= flags;

    // Persistence.
    SetDrawElementBordersFlags(m_drawElementBordersFlags);
}

void ViewportWidget::ActiveCanvasChanged()
{
    bool canvasLoaded = m_editorWindow->GetCanvas().IsValid();
    if (canvasLoaded)
    {
        m_viewportInteraction->CenterCanvasInViewport();
    }

    m_viewportInteraction->InitializeToolbars();

    EntityContextChanged();
}

void ViewportWidget::EntityContextChanged()
{
    if (m_inObjectPickMode)
    {
        OnEntityPickModeStopped();
    }

    // Disconnect from the PickModeRequests bus and reconnect with the new entity context
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    UiEditorEntityContext* context = m_editorWindow->GetEntityContext();
    if (context)
    {
        AzToolsFramework::EditorPickModeNotificationBus::Handler::BusConnect(context->GetContextId());
    }
}

void ViewportWidget::Refresh()
{
    m_refreshRequested = true;
}

void ViewportWidget::ClearUntilSafeToRedraw()
{
    // set flag so that Update will just clear the screen rather than rendering canvas
    m_canvasRenderIsEnabled = false;

#ifdef LYSHINE_ATOM_TODO // check if still needed
    // Force an update
    Update();
#endif

    // Schedule a timer to set the m_canvasRenderIsEnabled flag
    // using a time of zero just waits until there is nothing on the event queue
    QTimer::singleShot(0, this, SLOT(EnableCanvasRender()));
}

void ViewportWidget::SetRedrawEnabled(bool enabled)
{
    m_canvasRenderIsEnabled = enabled;
}

void ViewportWidget::PickItem(AZ::EntityId entityId)
{
    AzToolsFramework::EditorPickModeRequestBus::Broadcast(
        &AzToolsFramework::EditorPickModeRequests::PickModeSelectEntity, entityId);

    AzToolsFramework::EditorPickModeRequestBus::Broadcast(
        &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);
}

QWidget* ViewportWidget::CreateViewportWithRulersWidget(QWidget* parent)
{
    QWidget* viewportWithRulersWidget = new QWidget(parent);

    QGridLayout* viewportWithRulersLayout = new QGridLayout(viewportWithRulersWidget);
    viewportWithRulersLayout->setContentsMargins(0, 0, 0, 0);
    viewportWithRulersLayout->setSpacing(0);

    m_rulerHorizontal = new RulerWidget(RulerWidget::Orientation::Horizontal, viewportWithRulersWidget, m_editorWindow);
    m_rulerVertical = new RulerWidget(RulerWidget::Orientation::Vertical, viewportWithRulersWidget, m_editorWindow);

    m_rulerCorner = new QWidget();
    m_rulerCorner->setBackgroundRole(QPalette::Window);

    viewportWithRulersLayout->addWidget(m_rulerCorner,0,0);
    viewportWithRulersLayout->addWidget(m_rulerHorizontal,0,1);
    viewportWithRulersLayout->addWidget(m_rulerVertical,1,0);
    viewportWithRulersLayout->addWidget(this,1,1);

    ApplyRulerVisibility();

    return viewportWithRulersWidget;
}

void ViewportWidget::ShowRulers(bool show)
{
    if (show != m_rulersVisible)
    {
        m_rulersVisible = show;
        ApplyRulerVisibility();
        SetPersistentRulerVisibility(m_rulersVisible);
    }
}

void ViewportWidget::RefreshRulers()
{
    if (m_rulersVisible)
    {
        m_rulerHorizontal->update();
        m_rulerVertical->update();
    }
}

void ViewportWidget::SetRulerCursorPositions(const QPoint& globalPos)
{
    if (m_rulersVisible)
    {
        m_rulerHorizontal->SetCursorPos(globalPos);
        m_rulerVertical->SetCursorPos(globalPos);
    }
}

void ViewportWidget::ShowGuides(bool show)
{
    if (show != m_guidesVisible)
    {
        m_guidesVisible = show;
        SetPersistentGuideVisibility(m_guidesVisible);
    }
}

void ViewportWidget::contextMenuEvent(QContextMenuEvent* e)
{
    if (m_editorWindow->GetCanvas().IsValid())
    {
        UiEditorMode editorMode = m_editorWindow->GetEditorMode();
        if (editorMode == UiEditorMode::Edit)
        {
            // The context menu.
            const QPoint pos = e->pos();
            HierarchyMenu contextMenu(m_editorWindow->GetHierarchy(),
                HierarchyMenu::Show::kCutCopyPaste |
                HierarchyMenu::Show::kNew_EmptyElement |
                HierarchyMenu::Show::kDeleteElement |
                HierarchyMenu::Show::kNewSlice |
                HierarchyMenu::Show::kNew_InstantiateSlice |
                HierarchyMenu::Show::kPushToSlice |
                HierarchyMenu::Show::kEditorOnly |
                HierarchyMenu::Show::kFindElements,
                true,
                &pos);

            contextMenu.exec(e->globalPos());
        }
    }

    RenderViewportWidget::contextMenuEvent(e);
}

void ViewportWidget::UserSelectionChanged(HierarchyItemRawPtrList* items)
{
    Refresh();

    if (!items)
    {
        m_viewportInteraction->ClearInteraction();
    }
}

void ViewportWidget::EnableCanvasRender()
{
    m_canvasRenderIsEnabled = true;

    // force a redraw
    Refresh();
    RefreshTick();
}

void ViewportWidget::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
{
    // Update
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();
    if (editorMode == UiEditorMode::Edit)
    {
        UpdateEditMode(deltaTime);
    }
    else // if (editorMode == UiEditorMode::Preview)
    {
        UpdatePreviewMode(deltaTime);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int ViewportWidget::GetTickOrder()
{
    return AZ::TICK_PRE_RENDER;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewportWidget::OnRenderTick()
{
    if (!m_uiRenderer->IsReady() || !m_canvasRenderIsEnabled)
    {
        return;
    }

    const float dpiScale = QtHelpers::GetHighDpiScaleFactor(*this);
    ViewportIcon::SetDpiScaleFactor(dpiScale);

    UiEditorMode editorMode = m_editorWindow->GetEditorMode();
    if (editorMode == UiEditorMode::Edit)
    {
        RenderEditMode();
    }
    else // if (editorMode == UiEditorMode::Preview)
    {
        RenderPreviewMode();
    }
}

void ViewportWidget::RefreshTick()
{
#ifdef LYSHINE_EDITOR_TODO // still need this?
    if (m_refreshRequested)
    {
        if (m_canvasRenderIsEnabled)
        {
            // Redraw the canvas
            Update();
        }
        m_refreshRequested = false;

        // in case we were called manually, reset the timer
        m_updateTimer.start();
    }
#endif
}

void ViewportWidget::mousePressEvent(QMouseEvent* ev)
{
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();

    QPointF scaledPosition = WidgetToViewport(ev->localPos());
    QMouseEvent scaledEvent(ev->type(), scaledPosition, ev->button(), ev->buttons(), ev->modifiers());
    if (editorMode == UiEditorMode::Edit)
    {
        // in Edit mode just send input to ViewportInteraction
        m_viewportInteraction->MousePressEvent(&scaledEvent);
    }
    else // if (editorMode == UiEditorMode::Preview)
    {
        // In Preview mode convert the event into a game input event and send to canvas
        AZ::EntityId canvasEntityId = m_editorWindow->GetPreviewModeCanvas();
        if (canvasEntityId.IsValid())
        {
            if (ev->button() == Qt::LeftButton)
            {
                // Send event to this canvas
                const AZ::Vector2 viewportPosition(aznumeric_cast<float>(scaledPosition.x()), aznumeric_cast<float>(scaledPosition.y()));
                const AzFramework::InputChannel::Snapshot inputSnapshot(AzFramework::InputDeviceMouse::Button::Left,
                                                                        AzFramework::InputDeviceMouse::Id,
                                                                        AzFramework::InputChannel::State::Began);
                HandleCanvasInputEvent(canvasEntityId, inputSnapshot, &viewportPosition);
            }
        }
    }

    // Note: do not propagate this event to parent QViewport, otherwise
    // it will manipulate the mouse position in unexpected ways.

    Refresh();
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* ev)
{
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();

    QPointF scaledPosition = WidgetToViewport(ev->localPos());
    QMouseEvent scaledEvent(ev->type(), scaledPosition, ev->button(), ev->buttons(), ev->modifiers());

    if (editorMode == UiEditorMode::Edit)
    {
        // in Edit mode just send input to ViewportInteraction
        m_viewportInteraction->MouseMoveEvent(&scaledEvent,
            m_editorWindow->GetHierarchy()->selectedItems());

        QPointF screenPosition = WidgetToViewport(ev->screenPos());
        SetRulerCursorPositions(screenPosition.toPoint());
    }
    else // if (editorMode == UiEditorMode::Preview)
    {
        // In Preview mode convert the event into a game input event and send to canvas
        AZ::EntityId canvasEntityId = m_editorWindow->GetPreviewModeCanvas();
        if (canvasEntityId.IsValid())
        {
            const AZ::Vector2 viewportPosition(aznumeric_cast<float>(scaledPosition.x()), aznumeric_cast<float>(scaledPosition.y()));
            const AzFramework::InputChannelId& channelId = (ev->buttons() & Qt::LeftButton) ?
                                                            AzFramework::InputDeviceMouse::Button::Left :
                                                            AzFramework::InputDeviceMouse::SystemCursorPosition;
            const AzFramework::InputChannel::Snapshot inputSnapshot(channelId,
                                                                    AzFramework::InputDeviceMouse::Id,
                                                                    AzFramework::InputChannel::State::Updated);
            HandleCanvasInputEvent(canvasEntityId, inputSnapshot, &viewportPosition);
        }
    }

    // Note: do not propagate this event to parent QViewport, otherwise
    // it will manipulate the mouse position in unexpected ways.

    Refresh();
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* ev)
{
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();

    QPointF scaledPosition = WidgetToViewport(ev->localPos());
    QMouseEvent scaledEvent(ev->type(), scaledPosition, ev->button(), ev->buttons(), ev->modifiers());
    if (editorMode == UiEditorMode::Edit)
    {
        // in Edit mode just send input to ViewportInteraction
        m_viewportInteraction->MouseReleaseEvent(&scaledEvent,
            m_editorWindow->GetHierarchy()->selectedItems());
    }
    else // if (editorMode == UiEditorMode::Preview)
    {
        // In Preview mode convert the event into a game input event and send to canvas
        AZ::EntityId canvasEntityId = m_editorWindow->GetPreviewModeCanvas();
        if (canvasEntityId.IsValid())
        {
            if (ev->button() == Qt::LeftButton)
            {
                // Send event to this canvas
                const AZ::Vector2 viewportPosition(aznumeric_cast<float>(scaledPosition.x()), aznumeric_cast<float>(scaledPosition.y()));
                const AzFramework::InputChannel::Snapshot inputSnapshot(AzFramework::InputDeviceMouse::Button::Left,
                                                                        AzFramework::InputDeviceMouse::Id,
                                                                        AzFramework::InputChannel::State::Ended);
                HandleCanvasInputEvent(canvasEntityId, inputSnapshot, &viewportPosition);
            }
        }
    }

    // Note: do not propagate this event to parent QViewport, otherwise
    // it will manipulate the mouse position in unexpected ways.

    Refresh();
}

void ViewportWidget::wheelEvent(QWheelEvent* ev)
{
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();
    QWheelEvent scaledEvent(
        WidgetToViewport(ev->position()),
        ev->globalPosition(),
        ev->pixelDelta(),
        ev->angleDelta(),
        ev->buttons(),
        ev->modifiers(),
        ev->phase(),
        ev->inverted()
    );

    if (editorMode == UiEditorMode::Edit)
    {
        // in Edit mode just send input to ViewportInteraction
        m_viewportInteraction->MouseWheelEvent(&scaledEvent);
    }

    RenderViewportWidget::wheelEvent(ev);

    Refresh();
}

bool ViewportWidget::eventFilter([[maybe_unused]] QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride)
    {
        // When a shortcut is matched, Qt's event processing sends out a shortcut override event
        // to allow other systems to override it. If it's not overridden, then the key events
        // get processed as a shortcut, even if the widget that's the target has a keyPress event
        // handler. In our case this causes a problem in preview mode for the Key_Delete event.
        // So, if we are preview mode avoid treating Key_Delete as a shortcut.

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        // Override the space bar shortcut so that the key gets handled by the viewport's KeyPress/KeyRelease
        // events when the viewport has the focus. The space bar is set up as a shortcut in order to give the
        // viewport the focus and activate the space bar when another widget has the focus. Once the shortcut
        // is pressed and focus is given to the viewport, the viewport takes over handling the space bar via
        // the KeyPress/KeyRelease events.
        // Also ignore nudge shortcuts in edit/preview mode so that the KeyPressEvent will be sent.
        switch (key)
        {
        case Qt::Key_Space:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        {
            event->accept();
            return true;
        }
        default:
        {
            break;
        }
        }

        UiEditorMode editorMode = m_editorWindow->GetEditorMode();
        if (editorMode == UiEditorMode::Preview)
        {
            if (key == Qt::Key_Delete)
            {
                event->accept();
                return true;
            }
        }
    }

    return false;
}

bool ViewportWidget::event(QEvent* ev)
{
    bool result = RenderViewportWidget::event(ev);
    return result;
}

void ViewportWidget::keyPressEvent(QKeyEvent* event)
{
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();
    if (editorMode == UiEditorMode::Edit)
    {
        // in Edit mode just send input to ViewportInteraction
        if (!m_viewportInteraction->KeyPressEvent(event))
        {
            RenderViewportWidget::keyPressEvent(event);
        }
    }
    else // if (editorMode == UiEditorMode::Preview)
    {
        // In Preview mode convert the event into a game input event and send to canvas
        AZ::EntityId canvasEntityId = m_editorWindow->GetPreviewModeCanvas();
        if (canvasEntityId.IsValid())
        {
            // Send event to this canvas
            const AzFramework::InputChannelId* inputChannelId = MapQtKeyToAzInputChannelId(event->key());
            const AzFramework::ModifierKeyMask activeModifierKeys = MapQtModifiersToAzInputModifierKeys(event->modifiers());
            if (inputChannelId)
            {
                const AzFramework::InputChannel::Snapshot inputSnapshot(*inputChannelId,
                                                                        AzFramework::InputDeviceKeyboard::Id,
                                                                        AzFramework::InputChannel::State::Began);
                HandleCanvasInputEvent(canvasEntityId, inputSnapshot, nullptr, activeModifierKeys);
            }
        }
    }
}

void ViewportWidget::focusOutEvent([[maybe_unused]] QFocusEvent* ev)
{
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();
    if (editorMode == UiEditorMode::Edit)
    {
        m_viewportInteraction->ClearInteraction();
    }
}

void ViewportWidget::keyReleaseEvent(QKeyEvent* event)
{
    UiEditorMode editorMode = m_editorWindow->GetEditorMode();
    if (editorMode == UiEditorMode::Edit)
    {
        // in Edit mode just send input to ViewportInteraction
        bool handled = m_viewportInteraction->KeyReleaseEvent(event);
        if (!handled)
        {
            RenderViewportWidget::keyReleaseEvent(event);
        }
    }
    else if (editorMode == UiEditorMode::Preview)
    {
        AZ::EntityId canvasEntityId = m_editorWindow->GetPreviewModeCanvas();
        if (canvasEntityId.IsValid())
        {
            bool handled = false;

            // Send event to this canvas
            const AzFramework::InputChannelId* inputChannelId = MapQtKeyToAzInputChannelId(event->key());
            const AzFramework::ModifierKeyMask activeModifierKeys = MapQtModifiersToAzInputModifierKeys(event->modifiers());
            if (inputChannelId)
            {
                const AzFramework::InputChannel::Snapshot inputSnapshot(*inputChannelId,
                                                                        AzFramework::InputDeviceKeyboard::Id,
                                                                        AzFramework::InputChannel::State::Ended);
                HandleCanvasInputEvent(canvasEntityId, inputSnapshot, nullptr, activeModifierKeys);
            }

            QString string = event->text();
            if (string.length() != 0 && !handled)
            {
                AZStd::string textUTF8 = string.toUtf8().data();
                HandleCanvasTextEvent(canvasEntityId, textUTF8);
            }
        }
    }
    else
    {
        AZ_Assert(0, "Invalid editorMode: %d", editorMode);
    }
}

void ViewportWidget::resizeEvent(QResizeEvent* ev)
{
    m_editorWindow->GetPreviewToolbar()->ViewportHasResized(ev);

    if (m_editorWindow->GetCanvas().IsValid())
    {
        UiEditorMode editorMode = m_editorWindow->GetEditorMode();
        if (editorMode == UiEditorMode::Edit)
        {
            if (m_viewportInteraction->ShouldScaleToFitOnViewportResize())
            {
                m_viewportInteraction->CenterCanvasInViewport();
            }
        }
    }

    RenderViewportWidget::resizeEvent(ev);
}

bool ViewportWidget::AcceptsMimeData(const QMimeData* mimeData)
{
    bool canvasLoaded = m_editorWindow->GetCanvas().IsValid();
    if (!canvasLoaded)
    {
        return false;
    }

    return AssetDropHelpers::DoesMimeDataContainSliceOrComponentAssets(mimeData);
}

void ViewportWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (AcceptsMimeData(event->mimeData()))
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void ViewportWidget::dropEvent(QDropEvent* event)
{
    if (AcceptsMimeData(event->mimeData()))
    {
        const AZ::EntityId targetEntityId;
        const bool onElement = false;
        const int childIndex = -1;
        const QPoint pos = event->pos();
        m_editorWindow->GetHierarchy()->DropMimeDataAssets(event->mimeData(), targetEntityId, onElement, childIndex, &pos);
        event->accept();

        // Put focus on the viewport widget
        activateWindow();
        setFocus();
    }
}

void ViewportWidget::OnEntityPickModeStarted()
{
    m_inObjectPickMode = true;
    m_viewportInteraction->StartObjectPickMode();
}

void ViewportWidget::OnEntityPickModeStopped()
{
    if (m_inObjectPickMode)
    {
        m_inObjectPickMode = false;
        m_viewportInteraction->StopObjectPickMode();
    }
}

void ViewportWidget::OnFontsReloaded()
{
    m_fontTextureHasChanged = true;
}

void ViewportWidget::OnFontTextureUpdated([[maybe_unused]] IFFont* font)
{
    m_fontTextureHasChanged = true;
}

LyShine::AttachmentImagesAndDependencies ViewportWidget::GetRenderTargets()
{
    LyShine::AttachmentImagesAndDependencies canvasTargets;

    AZ::EntityId canvasEntityId = m_editorWindow->GetCanvasForCurrentEditorMode();
    if (canvasEntityId.IsValid())
    {
        AZ::Entity* canvasEntity = nullptr;
        EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
        AZ_Assert(canvasEntity, "Canvas entity not found by ID");
        if (canvasEntity)
        {
            UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
            AZ_Assert(canvasComponent, "Canvas entity has no canvas component");
            if (canvasComponent)
            {
                canvasComponent->GetRenderTargets(canvasTargets);
            }
        }
    }

    return canvasTargets;
}

QPointF ViewportWidget::WidgetToViewport(const QPointF & point) const
{
    return point * WidgetToViewportFactor();
}

void ViewportWidget::UpdateEditMode(float deltaTime)
{
    if (m_fontTextureHasChanged)
    {
        // A font texture has changed since we last rendered. Force a render graph update for each loaded canvas
        m_editorWindow->FontTextureHasChanged();
        m_fontTextureHasChanged = false;
    }

    AZ::EntityId canvasEntityId = m_editorWindow->GetCanvas();
    if (!canvasEntityId.IsValid())
    {
        return; // this can happen if a render happens during a restart
    }

    AZ::Vector2 canvasSize;
    EBUS_EVENT_ID_RESULT(canvasSize, canvasEntityId, UiCanvasBus, GetCanvasSize);

    // Set the target size of the canvas
    EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, SetTargetCanvasSize, false, canvasSize);

    // Update this canvas (must be done after SetTargetCanvasSize)
    EBUS_EVENT_ID(canvasEntityId, UiEditorCanvasBus, UpdateCanvasInEditorViewport, deltaTime, false);
}

void ViewportWidget::RenderEditMode()
{
    // sort keys for different layers
    static const int64_t backgroundKey = -0x1000;
    static const int64_t topLayerKey = 0x1000000;

    AZ::EntityId canvasEntityId = m_editorWindow->GetCanvas();
    if (!canvasEntityId.IsValid())
    {
        return; // this can happen if a render happens during a restart
    }

    Draw2dHelper draw2d(m_draw2d.get());    // sets and resets 2D draw mode in constructor/destructor

    QTreeWidgetItemRawPtrQList selection = m_editorWindow->GetHierarchy()->selectedItems();

    AZ::Vector2 canvasSize;
    EBUS_EVENT_ID_RESULT(canvasSize, canvasEntityId, UiCanvasBus, GetCanvasSize);

    m_draw2d->SetSortKey(backgroundKey);

    // Render a rectangle covering the entire editor viewport area
    RenderViewportBackground();

    // Render a checkerboard background covering the canvas area which represents transparency  
    m_viewportBackground->Draw(draw2d,
        canvasSize,
        m_viewportInteraction->GetCanvasToViewportScale(),
        m_viewportInteraction->GetCanvasToViewportTranslation());

    // Set the target size of the canvas
    EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, SetTargetCanvasSize, false, canvasSize);

    // Render this canvas
    QSize scaledViewportSize = QtHelpers::GetDpiScaledViewportSize(*this);
    AZ::Vector2 viewportSize(static_cast<float>(scaledViewportSize.width()), static_cast<float>(scaledViewportSize.height()));
    EBUS_EVENT_ID(canvasEntityId, UiEditorCanvasBus, RenderCanvasInEditorViewport, false, viewportSize);

    m_draw2d->SetSortKey(topLayerKey);
    // Draw borders around selected and unselected UI elements in the viewport
    // depending on the flags in m_drawElementBordersFlags
    HierarchyItemRawPtrList selectedItems = SelectionHelpers::GetSelectedHierarchyItems(m_editorWindow->GetHierarchy(), selection);
    m_viewportHighlight->Draw(draw2d,
        m_editorWindow->GetHierarchy()->invisibleRootItem(),
        selectedItems,
        m_drawElementBordersFlags);

    // Draw primary gizmos and guide lines
    m_viewportInteraction->Draw(draw2d, selection);

    // Draw any interaction display for the rulers that is in the viewport
    m_rulerHorizontal->DrawForViewport(draw2d);
    m_rulerVertical->DrawForViewport(draw2d);

    // Draw secondary gizmos
    if (ViewportInteraction::InteractionMode::ROTATE == m_viewportInteraction->GetMode())
    {
        // Draw the pivots and degrees only in Rotate mode
        LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selection);
        for (auto element : selectedElements)
        {
            bool isHighlighted = (m_viewportInteraction->GetActiveElement() == element) &&
                (m_viewportInteraction->GetInteractionType() == ViewportInteraction::InteractionType::PIVOT);
            m_viewportPivot->Draw(draw2d, element, isHighlighted);

            ViewportHelpers::DrawRotationValue(element, m_viewportInteraction.get(), m_viewportPivot.get(), draw2d);
        }
    }
    else if (ViewportInteraction::InteractionMode::MOVE == m_viewportInteraction->GetMode() ||
              ViewportInteraction::InteractionMode::ANCHOR == m_viewportInteraction->GetMode())
    {
        // Draw the anchors only if we're in Anchor or Move mode

        // We draw extra anchor related data when we are in the middle of an interaction
        bool leftButtonIsActive = m_viewportInteraction->GetLeftButtonIsActive();
        bool spaceBarIsActive = m_viewportInteraction->GetSpaceBarIsActive();
        bool isInteracting = leftButtonIsActive && !spaceBarIsActive &&
            m_viewportInteraction->GetInteractionType() != ViewportInteraction::InteractionType::NONE &&
            m_viewportInteraction->GetInteractionType() != ViewportInteraction::InteractionType::GUIDE;

        ViewportHelpers::SelectedAnchors highlightedAnchors = m_viewportInteraction->GetGrabbedAnchors();

        // These flags affect what parts of the anchor display is drawn
        bool drawUnTransformedRect = false;
        bool drawAnchorLines = false;
        bool drawLinesToParent = false;

        bool anchorInteractionEnabled = m_viewportInteraction->GetMode() == ViewportInteraction::InteractionMode::ANCHOR && selectedItems.size() == 1;

        if (isInteracting)
        {
            if (m_viewportInteraction->GetMode() == ViewportInteraction::InteractionMode::MOVE)
            {
                // when interacting in move mode (changing offsets) we draw the anchor lines from the anchor to the element
                // and also draw a faint untransformed rect around the element
                drawUnTransformedRect = true;
                drawAnchorLines = true;
            }
            else
            {
                // when interacting in anchor mode we draw lines from the anchor to the parent rect
                drawLinesToParent = true;
            }
        }
        else
        {
            // not interacting but could be hovering over anchors
            if (highlightedAnchors.Any())
            {
                // if the anchors are highlighted (whether actually moving or not) we want to draw distance
                // lines from the anchor to the edges of it's parent rect. In this case we do NOT want to
                // draw the lines from the anchor to this element's rect or pivot
                drawLinesToParent = true;
            }
        }

        // for all the top level selected elements, draw the anchors
        LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selection);
        for (auto element : selectedElements)
        {
            m_viewportAnchor->Draw(draw2d,
                element,
                drawUnTransformedRect,
                drawAnchorLines,
                drawLinesToParent,
                anchorInteractionEnabled,
                highlightedAnchors);
        }
    }
}

void ViewportWidget::UpdatePreviewMode(float deltaTime)
{
    AZ::EntityId canvasEntityId = m_editorWindow->GetPreviewModeCanvas();

    if (m_fontTextureHasChanged)
    {
        // A font texture has changed since we last rendered. Force a render graph update for each loaded canvas
        m_editorWindow->FontTextureHasChanged();
        m_fontTextureHasChanged = false;
    }

    if (canvasEntityId.IsValid())
    {
        QSize scaledViewportSize = QtHelpers::GetDpiScaledViewportSize(*this);
        AZ::Vector2 viewportSize(static_cast<float>(scaledViewportSize.width()), static_cast<float>(scaledViewportSize.height()));

        // Get the canvas size
        AZ::Vector2 canvasSize = m_editorWindow->GetPreviewCanvasSize();
        if (canvasSize.GetX() == 0.0f && canvasSize.GetY() == 0.0f)
        {
            // special value of (0,0) means use the viewport size
            canvasSize = viewportSize;
        }

        // Set the target size of the canvas
        EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, SetTargetCanvasSize, true, canvasSize);

        // Update this canvas (must be done after SetTargetCanvasSize)
        EBUS_EVENT_ID(canvasEntityId, UiEditorCanvasBus, UpdateCanvasInEditorViewport, deltaTime, true);

        // Execute events that have been queued during the canvas update
        gEnv->pLyShine->ExecuteQueuedEvents();
    }
}

void ViewportWidget::RenderPreviewMode()
{
    // sort keys for different layers
    static const int64_t backgroundKey = -0x1000;

    AZ::EntityId canvasEntityId = m_editorWindow->GetPreviewModeCanvas();

    // Rather than scaling to exactly fit we try to draw at one of these preset scale factors
    // to make it it bit more obvious that the canvas size is changing
    float zoomScales[] = {
        1.00f,
        0.75f,
        0.50f,
        0.25f,
        0.10f,
        0.05f
    };

    if (canvasEntityId.IsValid())
    {
        QSize scaledViewportSize = QtHelpers::GetDpiScaledViewportSize(*this);
        AZ::Vector2 viewportSize(static_cast<float>(scaledViewportSize.width()), static_cast<float>(scaledViewportSize.height()));

        // Get the canvas size
        AZ::Vector2 canvasSize = m_editorWindow->GetPreviewCanvasSize();
        if (canvasSize.GetX() == 0.0f && canvasSize.GetY() == 0.0f)
        {
            // special value of (0,0) means use the viewport size
            canvasSize = viewportSize;
        }

        // work out what scale to use for the canvasToViewport matrix
        float scale = 1.0f;
        if (canvasSize.GetX() > viewportSize.GetX())
        {
            if (canvasSize.GetX() >= 1.0f)   // avoid divide by zero
            {
                scale = viewportSize.GetX() / canvasSize.GetX();
            }
        }
        if (canvasSize.GetY() > viewportSize.GetY())
        {
            if (canvasSize.GetY() >= 1.0f)   // avoid divide by zero
            {
                float scaleY = viewportSize.GetY() / canvasSize.GetY();
                if (scaleY < scale)
                {
                    scale = scaleY;
                }
            }
        }

        // match scale to one of the predefined scales. If the scale is so small
        // that it is less than the smallest scale then leave it as it is
        for (int i = 0; i < AZ_ARRAY_SIZE(zoomScales); ++i)
        {
            if (scale >= zoomScales[i])
            {
                scale = zoomScales[i];
                break;
            }
        }

        // Update the toolbar to show the current scale
        if (scale != m_previewCanvasScale)
        {
            m_previewCanvasScale = scale;
            m_editorWindow->GetPreviewToolbar()->UpdatePreviewCanvasScale(scale);
        }

        // Set up the canvasToViewportMatrix
        AZ::Vector3 scale3(scale, scale, 1.0f);
        AZ::Vector3 translation((viewportSize.GetX() - (canvasSize.GetX() * scale)) * 0.5f,
            (viewportSize.GetY() - (canvasSize.GetY() * scale)) * 0.5f, 0.0f);
        AZ::Matrix4x4 canvasToViewportMatrix = AZ::Matrix4x4::CreateScale(scale3);
        canvasToViewportMatrix.SetTranslation(translation);
        EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, SetCanvasToViewportMatrix, canvasToViewportMatrix);

        m_draw2d->SetSortKey(backgroundKey);

        RenderViewportBackground();

        // Render a black rectangle covering the canvas area. This allows the canvas bounds to be visible when the canvas size is
        // not exactly the same as the viewport size
        AZ::Vector2 topLeftInViewportSpace = CanvasHelpers::GetViewportPoint(canvasEntityId, AZ::Vector2(0.0f, 0.0f));
        AZ::Vector2 bottomRightInViewportSpace = CanvasHelpers::GetViewportPoint(canvasEntityId, canvasSize);
        AZ::Vector2 sizeInViewportSpace = bottomRightInViewportSpace - topLeftInViewportSpace;
        Draw2dHelper draw2d(m_draw2d.get());
        auto image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Black);
        draw2d.DrawImage(image, topLeftInViewportSpace, sizeInViewportSpace);

        // Render this canvas
        // NOTE: the displayBounds param is always false. If we wanted a debug option to display the bounds
        // in preview mode we would need to render the deferred primitives after this call so that they
        // show up in the correct viewport
        EBUS_EVENT_ID(canvasEntityId, UiEditorCanvasBus, RenderCanvasInEditorViewport, true, viewportSize);
    }
}

void ViewportWidget::RenderViewportBackground()
{
    QSize viewportSize = QtHelpers::GetDpiScaledViewportSize(*this);
    AZ::Color backgroundColor = ViewportHelpers::backgroundColorDark;
    const AZ::Data::Instance<AZ::RPI::Image>& image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);

    Draw2dHelper draw2d(m_draw2d.get());
    draw2d.SetImageColor(backgroundColor.GetAsVector3());
    draw2d.DrawImage(image, AZ::Vector2(0.0f, 0.0f), AZ::Vector2(static_cast<float>(viewportSize.width()), static_cast<float>(viewportSize.height())));
}

void ViewportWidget::SetupShortcuts()
{
    // Actions with shortcuts are created instead of direct shortcuts because the shortcut dispatcher only looks for matching actions

    // Give the viewport focus and activate the space bar
    {
        QAction* action = new QAction("Viewport Focus", this);
        action->setShortcut(QKeySequence(Qt::Key_Space));
        QObject::connect(action,
            &QAction::triggered,
            [this]()
        {
            setFocus();
            m_viewportInteraction->ActivateSpaceBar();
        });
        addAction(action);
    }
}

void ViewportWidget::ApplyRulerVisibility()
{
    // Since we are using a grid layout, setting the width of the corner widget (the square at the top left of the grid)
    // determines whether the rulers are zero size or not.
    int rulerBreadth = (m_rulersVisible) ? RulerWidget::GetRulerBreadth() : 0;
    m_rulerCorner->setFixedSize(rulerBreadth, rulerBreadth);
}


#include <moc_ViewportWidget.cpp>
