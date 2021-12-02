/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "ViewportNudge.h"
#include "ViewportElement.h"
#include "QtHelpers.h"
#include "GuideHelpers.h"
#include "AlignToolbarSection.h"
#include "ViewportMoveInteraction.h"
#include "ViewportMoveGuideInteraction.h"
#include <LyShine/UiComponentTypes.h>
#include <LyShine/Bus/UiEditorCanvasBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QToolButton>
#include <QTextDocumentFragment>
#include <QSettings>

#include <Editor/Resource.h>
#include <Editor/Util/EditorUtils.h>

static const float g_elementEdgeForgiveness = 10.0f;

// The square of the minimum corner-to-corner distance for an area selection
static const float g_minAreaSelectionDistance2 = 100.0f;

#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_KEY         "ViewportWidget::m_interactionMode"
#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_DEFAULT     ( ViewportInteraction::InteractionMode::SELECTION )

#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_KEY         "ViewportWidget::m_coordinateSystem"
#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_DEFAULT     ( ViewportInteraction::CoordinateSystem::LOCAL )

namespace
{
    const float defaultCanvasToViewportScaleIncrement = 0.20f;

    ViewportInteraction::InteractionMode PersistentGetInteractionMode()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        int defaultMode = static_cast<int>(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_DEFAULT);
        int result = settings.value(
                UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_KEY,
                defaultMode).toInt();

        settings.endGroup();

        return static_cast<ViewportInteraction::InteractionMode>(result);
    }

    void PersistentSetInteractionMode(ViewportInteraction::InteractionMode mode)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        settings.setValue(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_KEY,
            static_cast<int>(mode));

        settings.endGroup();
    }

    ViewportInteraction::CoordinateSystem PersistentGetCoordinateSystem()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        int defaultSystem = static_cast<int>(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_DEFAULT);
        int result = settings.value(
                UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_KEY,
                defaultSystem).toInt();

        settings.endGroup();

        return static_cast<ViewportInteraction::CoordinateSystem>(result);
    }

    void PersistentSetCoordinateSystem(ViewportInteraction::CoordinateSystem coordinateSystem)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        settings.setValue(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_KEY,
            static_cast<int>(coordinateSystem));

        settings.endGroup();
    }
} // anonymous namespace.

class ViewportInteractionExpanderWatcher
    : public QObject
{
public:
    ViewportInteractionExpanderWatcher(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    bool eventFilter(QObject* obj, QEvent* event) override
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            {
                if (qobject_cast<QToolButton*>(obj))
                {
                    auto mouseEvent = static_cast<QMouseEvent*>(event);
                    auto expansion = qobject_cast<QToolButton*>(obj);

                    expansion->setPopupMode(QToolButton::InstantPopup);
                    auto menu = new QMenu(expansion);

                    auto toolbar = qobject_cast<QToolBar*>(expansion->parentWidget());

                    for (auto toolbarAction : toolbar->actions())
                    {
                        auto actionWidget = toolbar->widgetForAction(toolbarAction);
                        if (actionWidget && !actionWidget->isVisible() && !toolbarAction->text().isEmpty())
                        {
                            QString plainText = QTextDocumentFragment::fromHtml(actionWidget->toolTip()).toPlainText();
                            toolbarAction->setText(plainText);
                            menu->addAction(toolbarAction);
                        }
                    }

                    if (menu->actions().count() == 0)
                    {
                        QAction* noAction = new QAction(this);
                        noAction->setEnabled(false);
                        noAction->setText(tr("Please resize the toolbar to see all the controls."));

                        menu->addAction(noAction);
                    }

                    menu->exec(mouseEvent->globalPos());
                    return true;
                }

                break;
            }
        }

        return QObject::eventFilter(obj, event);
    }
};

ViewportInteraction::ViewportInteraction(EditorWindow* editorWindow)
    : QObject()
    , m_editorWindow(editorWindow)
    , m_activeElementId()
    , m_anchorWhole(new ViewportIcon("Editor/Icons/Viewport/Anchor_Whole.tif"))
    , m_pivotIcon(new ViewportIcon("Editor/Icons/Viewport/Pivot.tif"))
    , m_interactionMode(PersistentGetInteractionMode())
    , m_interactionType(InteractionType::NONE)
    , m_coordinateSystem(PersistentGetCoordinateSystem())
    , m_spaceBarIsActive(false)
    , m_leftButtonIsActive(false)
    , m_middleButtonIsActive(false)
    , m_reversibleActionStarted(false)
    , m_startMouseDragPos(0.0f, 0.0f)
    , m_lastMouseDragPos(0.0f, 0.0f)
    , m_selectedElementsAtSelectionStart()
    , m_canvasViewportMatrixProps()
    , m_shouldScaleToFitOnViewportResize(true)
    , m_transformComponentType(AZ::Uuid::CreateNull())
    , m_grabbedEdges(ViewportHelpers::ElementEdges())
    , m_startAnchors(UiTransform2dInterface::Anchors())
    , m_grabbedAnchors(ViewportHelpers::SelectedAnchors())
    , m_grabbedGizmoParts(ViewportHelpers::GizmoParts())
    , m_lineTriangleX(new ViewportIcon("Editor/Icons/Viewport/Transform_Gizmo_Line_Triangle_X.tif"))
    , m_lineTriangleY(new ViewportIcon("Editor/Icons/Viewport/Transform_Gizmo_Line_Triangle_Y.tif"))
    , m_circle(new ViewportIcon("Editor/Icons/Viewport/Transform_Gizmo_Circle.tif"))
    , m_lineSquareX(new ViewportIcon("Editor/Icons/Viewport/Transform_Gizmo_Line_Square_X.tif"))
    , m_lineSquareY(new ViewportIcon("Editor/Icons/Viewport/Transform_Gizmo_Line_Square_Y.tif"))
    , m_centerSquare(new ViewportIcon("Editor/Icons/Viewport/Transform_Gizmo_Center_Square.tif"))
    , m_dottedLine(new ViewportIcon("Editor/Icons/Viewport/DottedLine.tif"))
    , m_dragInteraction(nullptr)
    , m_expanderWatcher(new ViewportInteractionExpanderWatcher(this))
{
    m_cursorRotate = CMFCUtils::LoadCursor(IDC_POINTER_OBJECT_ROTATE);
}

ViewportInteraction::~ViewportInteraction()
{
}

void ViewportInteraction::ClearInteraction(bool clearSpaceBarIsActive)
{
    m_activeElementId.SetInvalid();
    m_interactionType = InteractionType::NONE;
    m_spaceBarIsActive = clearSpaceBarIsActive ? false : m_spaceBarIsActive;
    m_leftButtonIsActive = false;
    m_middleButtonIsActive = false;
    m_startMouseDragPos = AZ::Vector2::CreateZero();
    m_lastMouseDragPos = AZ::Vector2::CreateZero();
    m_grabbedEdges = ViewportHelpers::ElementEdges();
    m_startAnchors = UiTransform2dInterface::Anchors();
    m_grabbedAnchors = ViewportHelpers::SelectedAnchors();
    m_grabbedGizmoParts = ViewportHelpers::GizmoParts();
    m_selectedElementsAtSelectionStart.clear();
    m_isAreaSelectionActive = false;
    m_reversibleActionStarted = false;

    SAFE_DELETE(m_dragInteraction);
}

void ViewportInteraction::Nudge(NudgeDirection direction, NudgeSpeed speed)
{
    const AZ::Uuid& transformComponentType = InitAndGetTransformComponentType();

        ViewportNudge::Nudge(m_editorWindow,
            m_interactionMode,
            m_editorWindow->GetViewport(),
            direction,
            speed,
            m_editorWindow->GetHierarchy()->selectedItems(),
            m_coordinateSystem,
            transformComponentType);
}

void ViewportInteraction::StartObjectPickMode()
{
    // Temporarily set the viewport interaction mode to "Selection" and disable the toolbar
    m_interactionModeBeforePickMode = GetMode();
    SetMode(InteractionMode::SELECTION);
    m_editorWindow->GetModeToolbar()->setEnabled(false);

    InvalidateHoverElement();

    UpdateCursor();
}

void ViewportInteraction::StopObjectPickMode()
{
    bool mousePressed = GetLeftButtonIsActive();

    m_editorWindow->GetModeToolbar()->setEnabled(true);
    SetMode(m_interactionModeBeforePickMode);

    SetCursorStr("");

    m_hoverElement.SetInvalid();

    // Update interaction type and cursor right away if the mouse is already released (user pressed ESC to cancel pick mode)
    // instead of waiting for a mouse move/release event
    if (!mousePressed)
    {
        QPoint viewportCursorPos = m_editorWindow->GetViewport()->mapFromGlobal(QCursor::pos());
        QTreeWidgetItemRawPtrQList selectedItems = m_editorWindow->GetHierarchy()->selectedItems();
        UpdateInteractionType(AZ::Vector2(aznumeric_cast<float>(viewportCursorPos.x()), aznumeric_cast<float>(viewportCursorPos.y())), selectedItems);
    }

    UpdateCursor();
}

bool ViewportInteraction::GetLeftButtonIsActive()
{
    return m_leftButtonIsActive;
}

bool ViewportInteraction::GetSpaceBarIsActive()
{
    return m_spaceBarIsActive;
}

void ViewportInteraction::ActivateSpaceBar()
{
    m_spaceBarIsActive = true;
    UpdateCursor();

    if (m_editorWindow->GetViewport()->IsInObjectPickMode())
    {
        // Don't highlight the hover element during a pan
        InvalidateHoverElement();
    }
}

void ViewportInteraction::Draw(Draw2dHelper& draw2d,
    const QTreeWidgetItemRawPtrQList& selectedItems)
{
    // Draw border around hover UI element
    if (m_hoverElement.IsValid())
    {
        m_editorWindow->GetViewport()->GetViewportHighlight()->DrawHover(draw2d, m_hoverElement);
    }

    // Draw the guide lines
    if (m_editorWindow->GetViewport()->AreGuidesShown())
    {
        GuideHelpers::DrawGuideLines(m_editorWindow->GetCanvas(), m_editorWindow->GetViewport(), draw2d);
    }

    // Draw the transform gizmo where appropriate
    if (m_interactionMode != InteractionMode::SELECTION)
    {
        LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        switch (m_interactionMode)
        {
        case InteractionMode::MOVE:
        case InteractionMode::ANCHOR:
            for (auto element : selectedElements)
            {
                if (!ViewportHelpers::IsControlledByLayout(element))
                {
                    DrawAxisGizmo(draw2d, element, m_coordinateSystem, m_lineTriangleX.get(), m_lineTriangleY.get());
                }
            }
            break;
        case InteractionMode::ROTATE:
            for (auto element : selectedElements)
            {
                DrawCircleGizmo(draw2d, element);
            }
            break;
        case InteractionMode::RESIZE:
            for (auto element : selectedElements)
            {
                if (!ViewportHelpers::IsControlledByLayout(element))
                {
                    DrawAxisGizmo(draw2d, element, CoordinateSystem::LOCAL, m_lineSquareX.get(), m_lineSquareY.get());
                }
            }
            break;
        }
    }

    // Draw the area selection, if there is one
    if (AreaSelectionIsActive())
    {
        m_dottedLine->DrawAxisAlignedBoundingBox(draw2d, m_startMouseDragPos, m_lastMouseDragPos);
    }

    // If there is an active drag interaction give it a chance to render its interaction display
    if (m_dragInteraction)
    {
        m_dragInteraction->Render(draw2d);
    }

    // Draw the cursor string
    if (!m_cursorStr.empty() && m_editorWindow->GetViewport()->underMouse())
    {
        ViewportHelpers::DrawCursorText(m_cursorStr, draw2d, m_editorWindow->GetViewport());
    }
}

bool ViewportInteraction::AreaSelectionIsActive()
{
    return m_isAreaSelectionActive;
}

void ViewportInteraction::BeginReversibleAction(const QTreeWidgetItemRawPtrQList& selectedItems)
{
    if ((m_reversibleActionStarted) ||
        (m_interactionType == InteractionType::NONE || m_interactionType == InteractionType::GUIDE) ||
        (m_interactionMode == InteractionMode::SELECTION))
    {
        // Nothing to do.
        return;
    }

    // we are about to change something and we have not started an undo action yet, start one
    m_reversibleActionStarted = true;

    // Tell the Properties panel that we're about to do a reversible action
    HierarchyClipboard::BeginUndoableEntitiesChange(m_editorWindow, m_selectedEntitiesUndoState);

    // Snapping.
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, m_editorWindow->GetCanvas(), UiEditorCanvasBus, GetIsSnapEnabled);
    if (isSnapping)
    {
        // Set all initial non-snapped values.

        HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(m_editorWindow->GetHierarchy(),
                selectedItems);
        for (auto i : items)
        {
            UiTransform2dInterface::Offsets offsets;
            EBUS_EVENT_ID_RESULT(offsets, i->GetEntityId(), UiTransform2dBus, GetOffsets);
            i->SetNonSnappedOffsets(offsets);

            float rotation;
            EBUS_EVENT_ID_RESULT(rotation, i->GetEntityId(), UiTransformBus, GetZRotation);
            i->SetNonSnappedZRotation(rotation);
        }
    }
}

void ViewportInteraction::EndReversibleAction()
{
    if (!m_reversibleActionStarted)
    {
        // Nothing to do.
        return;
    }

    m_reversibleActionStarted = false;

    if (AreaSelectionIsActive())
    {
        // Nothing to do.
        return;
    }

    // Note that EndReversibleAction is not used for interactions that handle undo in a m_dragInteraction
    // Ideally we will change them all to use m_dragInteraction and handle the undo there.
    HierarchyClipboard::EndUndoableEntitiesChange(m_editorWindow, "viewport interaction", m_selectedEntitiesUndoState);
}

void ViewportInteraction::MousePressEvent(QMouseEvent* ev)
{
    AZ::Vector2 mousePosition = QtHelpers::QPointFToVector2(ev->localPos());
    m_startMouseDragPos = m_lastMouseDragPos = mousePosition;
    const bool ctrlKeyPressed = ev->modifiers().testFlag(Qt::ControlModifier);

    // Detect whether an entity was picked on the mouse press so that
    // mouse move/release events can be handled appropriately
    m_entityPickedOnMousePress = false;

    // Prepare to handle panning
    if ((!m_leftButtonIsActive) &&
        (ev->button() == Qt::MiddleButton))
    {
        m_middleButtonIsActive = true;
    }
    else if ((!m_middleButtonIsActive) &&
             (ev->button() == Qt::LeftButton))
    {
        // Prepare for clicking and dragging

        m_leftButtonIsActive = true;

        if (m_activeElementId.IsValid())
        {
            if (m_grabbedAnchors.Any())
            {
                // Prepare to move anchors
                EBUS_EVENT_ID_RESULT(m_startAnchors, m_activeElementId, UiTransform2dBus, GetAnchors);
            }
            else if (m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR)
            {
                // Prepare for moving elements by offsets or anchors
                QTreeWidgetItemRawPtrQList selectedItems = m_editorWindow->GetHierarchy()->selectedItems();
                m_dragInteraction = new ViewportMoveInteraction(m_editorWindow->GetHierarchy(), selectedItems,
                    m_editorWindow->GetCanvas(), GetActiveElement(), m_coordinateSystem, m_grabbedGizmoParts, m_interactionMode, m_interactionType, mousePosition);
            }
        }
        else if (m_interactionType == InteractionType::GUIDE)
        {
            // We are hovering over a guide with the move guide icon displayed so start the move guide interaction
            m_dragInteraction = new ViewportMoveGuideInteraction(m_editorWindow, m_editorWindow->GetCanvas(),
                m_activeGuideIsVertical, m_activeGuideIndex, mousePosition);
        }
    }

    // If there isn't another interaction happening, try to select an element
    if ((!m_spaceBarIsActive && !m_middleButtonIsActive && m_interactionType == InteractionType::NONE) ||
        ((m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR) && m_interactionType == InteractionType::DIRECT && ctrlKeyPressed))
    {
        if (m_editorWindow->GetViewport()->IsInObjectPickMode())
        {
            AZ::Entity* element = nullptr;
            EBUS_EVENT_ID_RESULT(element, m_editorWindow->GetCanvas(), UiCanvasBus, PickElement, mousePosition);

            m_editorWindow->GetViewport()->PickItem(element ? element->GetId() : AZ::EntityId());
            m_entityPickedOnMousePress = true;
        }
        else
        {
            QTreeWidgetItemRawPtrQList selectedItems = m_editorWindow->GetHierarchy()->selectedItems();

            // Because we draw the Anchors (grayed out) in MOVE mode or when multiple items are selected in ANCHOR mode then it is confusing if you
            // click on them, thinking it might do something, and it changes the selection.
            // But if the click is inside the element that the anchor belongs to we do want to consider it a select or it would get in the way.
            // So a compromise is that, if you click on them, and the click is outside the element they belong to, then the click is ignored.
            bool ignoreClickForSelection = false;
            if ((m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR) && m_interactionType == InteractionType::NONE)
            {
                LyShine::EntityArray topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
                for (auto elementWithAnchors : topLevelSelectedElements)
                {
                    ViewportHelpers::SelectedAnchors grabbedAnchors;
                    if (!ViewportHelpers::IsControlledByLayout(elementWithAnchors) &&
                        ViewportElement::PickAnchors(elementWithAnchors, mousePosition, m_anchorWhole->GetTextureSize(), grabbedAnchors))
                    {
                        // Hovering over anchors, if the click is outside the element with the anchors then ignore
                        bool isElementUnderCursor = false;
                        EBUS_EVENT_ID_RESULT(isElementUnderCursor, elementWithAnchors->GetId(), UiTransformBus, IsPointInRect, mousePosition);
                        if (!isElementUnderCursor)
                        {
                            ignoreClickForSelection = true;
                            break;
                        }
                    }
                }
            }

            if (!ignoreClickForSelection)
            {
                AZ::Entity* element = nullptr;
                EBUS_EVENT_ID_RESULT(element, m_editorWindow->GetCanvas(), UiCanvasBus, PickElement, mousePosition);

                HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();
                QTreeWidgetItem* widgetItem = nullptr;
                bool itemDeselected = false;

                // store the selected items at the start of the selection
                m_selectedElementsAtSelectionStart = SelectionHelpers::GetSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);

                if (element)
                {
                    widgetItem = HierarchyHelpers::ElementToItem(hierarchyWidget, element, false);

                    // If user is selecting something with the control key pressed, the
                    // element may need to be de-selected (if it's already selected).
                    itemDeselected = HierarchyHelpers::HandleDeselect(widgetItem, ctrlKeyPressed);
                }

                // If the item didn't need to be de-selected, then we should select it
                if (!itemDeselected)
                {
                    // Note that widgetItem could still be null at this point, but
                    // SetSelectedItem will handle this situation for us.
                    HierarchyHelpers::SetSelectedItem(hierarchyWidget, element);
                }

                // ClearInteraction gets called if the selection changes to empty but we do not want to clear these since we can start a drag now
                m_leftButtonIsActive = true;
                m_startMouseDragPos = m_lastMouseDragPos = mousePosition;

                m_isAreaSelectionActive = true;
            }
        }
    }

    UpdateCursor();
}

void ViewportInteraction::PanOnMouseMoveEvent(const AZ::Vector2& mousePosition)
{
    AZ::Vector2 deltaPosition = mousePosition - m_lastMouseDragPos;
    AZ::Vector3 mousePosDelta(EntityHelpers::MakeVec3(deltaPosition));
    m_canvasViewportMatrixProps.translation += mousePosDelta;
    UpdateCanvasToViewportMatrix();
    UpdateShouldScaleToFitOnResize();
}

const AZ::Uuid& ViewportInteraction::InitAndGetTransformComponentType()
{
    if (m_transformComponentType.IsNull())
    {
        m_transformComponentType = LyShine::UiTransform2dComponentUuid;
    }

    return m_transformComponentType;
}

void ViewportInteraction::MouseMoveEvent(QMouseEvent* ev,
    const QTreeWidgetItemRawPtrQList& selectedItems)
{
    AZ::Vector2 mousePosition = QtHelpers::QPointFToVector2(ev->localPos());

    if (m_spaceBarIsActive)
    {
        if (m_leftButtonIsActive || m_middleButtonIsActive)
        {
            PanOnMouseMoveEvent(mousePosition);
        }
    }
    else if (m_leftButtonIsActive)
    {
        if (!m_entityPickedOnMousePress)
        {
            // Click and drag
            ProcessInteraction(mousePosition,
                ev->modifiers(),
                selectedItems);
        }
    }
    else if (m_middleButtonIsActive)
    {
        PanOnMouseMoveEvent(mousePosition);
    }
    else if (ev->buttons() == Qt::NoButton)
    {
        // Hover

        if (m_editorWindow->GetViewport()->IsInObjectPickMode())
        {
            // Update hover element. We only display the hover element in object pick mode
            UpdateHoverElement(mousePosition);
        }
        else
        {
            m_interactionType = InteractionType::NONE;
            m_grabbedEdges.SetAll(false);
            m_grabbedAnchors.SetAll(false);
            m_grabbedGizmoParts.SetBoth(false);

            UpdateInteractionType(mousePosition,
                selectedItems);
            UpdateCursor();
        }
    }

    m_lastMouseDragPos = mousePosition;
}

void ViewportInteraction::MouseReleaseEvent(QMouseEvent* ev,
    [[maybe_unused]] const QTreeWidgetItemRawPtrQList& selectedItems)
{
    if (!m_entityPickedOnMousePress)
    {
        // if the mouse press and release were in the same position and
        // no changes have been made then we can treat it as a mouse-click which can
        // do selection. This is useful in the case where we are in move mode but just
        // clicked on something that is either:
        // - one of multiple things selected and we want to just select this
        // - an element in front of something that is selected
        // In this case the mouse press will not have been treated as selection in
        // MousePressEvent so we need to handle this as a special case
        if (!m_reversibleActionStarted && m_lastMouseDragPos == m_startMouseDragPos &&
            ev->button() == Qt::LeftButton && !ev->modifiers().testFlag(Qt::ControlModifier) &&
            (m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR)
            && (m_interactionType == InteractionType::DIRECT || m_interactionType == InteractionType::TRANSFORM_GIZMO))
        {
            AZ::Vector2 mousePosition = QtHelpers::QPointFToVector2(ev->localPos());

            bool ignoreClick = false;
            if (m_interactionType == InteractionType::TRANSFORM_GIZMO)
            {
                // if we clicked on a gizmo but didn't move then we want to consider this a select click as long as the click
                // was inside the active element. (the square part of the gizmo can cover a large area of the element so ignoring
                // the click is confusing).
                if (m_activeElementId.IsValid())
                {
                    bool isActiveElementUnderCursor = false;
                    EBUS_EVENT_ID_RESULT(isActiveElementUnderCursor, m_activeElementId, UiTransformBus, IsPointInRect, mousePosition);
                    if (!isActiveElementUnderCursor)
                    {
                        ignoreClick = true;
                    }
                }
            }

            if (!ignoreClick)
            {
                AZ::Entity* element = nullptr;
                EBUS_EVENT_ID_RESULT(element, m_editorWindow->GetCanvas(), UiCanvasBus, PickElement, mousePosition);

                HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();
                if (element)
                {
                    HierarchyHelpers::SetSelectedItem(hierarchyWidget, element);
                }
            }
        }

        if (m_dragInteraction)
        {
            // test to see if the mouse position is inside the viewport on each axis
            const QPoint& pos = ev->pos();
            const QSize& size = m_editorWindow->GetViewport()->size();

            ViewportDragInteraction::EndState inWidget;
            if (pos.x() >= 0 && pos.x() < size.width())
                inWidget = pos.y() >= 0 && pos.y() < size.height() ? ViewportDragInteraction::EndState::Inside : ViewportDragInteraction::EndState::OutsideY;
            else
                inWidget = pos.y() >= 0 && pos.y() < size.height() ? ViewportDragInteraction::EndState::OutsideX : ViewportDragInteraction::EndState::OutsideXY;

            // Some interactions end differently depending on whether the mouse was released inside or outside the viewport
            m_dragInteraction->EndInteraction(inWidget);
        }

        // Tell the Properties panel to update.
        // Refresh attributes as well in case this change affects an attribute (ex. anchors affect warning text on scale to device mode)
        const AZ::Uuid& transformComponentType = InitAndGetTransformComponentType();
        m_editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues, &transformComponentType);

        // Tell the Properties panel that the reversible action is complete
        EndReversibleAction();
    }

    // Reset the interaction
    ClearInteraction(false);

    if (!m_spaceBarIsActive)
    {
        // Immediately update the interaction type and cursor (using the possibly new selection)
        AZ::Vector2 mousePosition = QtHelpers::QPointFToVector2(ev->localPos());
        UpdateInteractionType(mousePosition,
            m_editorWindow->GetHierarchy()->selectedItems());
    }

    UpdateCursor();
}

void ViewportInteraction::MouseWheelEvent(QWheelEvent* ev)
{
    if (m_leftButtonIsActive || m_middleButtonIsActive)
    {
        // Ignore event.
        return;
    }

    const QPoint numDegrees(ev->angleDelta());

    if (!numDegrees.isNull())
    {
        // Angle delta returns distance rotated by mouse wheel in eigths of a
        // degree.
        static const int numStepsPerDegree = 8;
        const float numScrollDegress = aznumeric_cast<float>(numDegrees.y() / numStepsPerDegree);

        static const float zoomMultiplier = 1 / 100.0f;
        Vec2i pivotPoint(
            static_cast<int>(ev->position().x()),
            static_cast<int>(ev->position().y()));

        float newScale = m_canvasViewportMatrixProps.scale + numScrollDegress * zoomMultiplier;

        SetCanvasToViewportScale(QuantizeZoomScale(newScale), &pivotPoint);
    }
}

bool ViewportInteraction::KeyPressEvent(QKeyEvent* ev)
{
    switch (ev->key())
    {
    case Qt::Key_Space:
        if (!ev->isAutoRepeat())
        {
            ActivateSpaceBar();
        }
        return true;
    case Qt::Key_Up:
        Nudge(ViewportInteraction::NudgeDirection::Up,
            (ev->modifiers() & Qt::ShiftModifier) ? ViewportInteraction::NudgeSpeed::Fast : ViewportInteraction::NudgeSpeed::Slow);
        return true;
    case Qt::Key_Down:
        Nudge(ViewportInteraction::NudgeDirection::Down,
            (ev->modifiers() & Qt::ShiftModifier) ? ViewportInteraction::NudgeSpeed::Fast : ViewportInteraction::NudgeSpeed::Slow);
        return true;
    case Qt::Key_Left:
        Nudge(ViewportInteraction::NudgeDirection::Left,
            (ev->modifiers() & Qt::ShiftModifier) ? ViewportInteraction::NudgeSpeed::Fast : ViewportInteraction::NudgeSpeed::Slow);
        return true;
    case Qt::Key_Right:
        Nudge(ViewportInteraction::NudgeDirection::Right,
            (ev->modifiers() & Qt::ShiftModifier) ? ViewportInteraction::NudgeSpeed::Fast : ViewportInteraction::NudgeSpeed::Slow);
        return true;
    default:
        break;
    }

    return false;
}

bool ViewportInteraction::KeyReleaseEvent(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Space)
    {
        if (!ev->isAutoRepeat())
        {
            ClearInteraction();
            UpdateCursor();

            if (m_editorWindow->GetViewport()->IsInObjectPickMode())
            {
                // Update hover element right away in case mouse is over an element
                QPoint viewportCursorPos = m_editorWindow->GetViewport()->mapFromGlobal(QCursor::pos());
                UpdateHoverElement(AZ::Vector2(aznumeric_cast<float>(viewportCursorPos.x()), aznumeric_cast<float>(viewportCursorPos.y())));
            }
        }

        return true;
    }

    return false;
}

void ViewportInteraction::SetMode(InteractionMode m)
{
    ClearInteraction();
    m_interactionMode = m;
    PersistentSetInteractionMode(m_interactionMode);
    m_editorWindow->GetModeToolbar()->SetCheckedItem(static_cast<int>(m_interactionMode));
    m_editorWindow->GetModeToolbar()->GetAlignToolbarSection()->SetIsVisible(
        m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR);
    UpdateCoordinateSystemToolbarSection();
    m_editorWindow->GetViewport()->Refresh();
}

ViewportInteraction::InteractionMode ViewportInteraction::GetMode() const
{
    return m_interactionMode;
}

ViewportInteraction::InteractionType ViewportInteraction::GetInteractionType() const
{
    return m_interactionType;
}

void ViewportInteraction::SetCoordinateSystem(CoordinateSystem s)
{
    m_coordinateSystem = s;
    PersistentSetCoordinateSystem(s);
    m_editorWindow->GetCoordinateSystemToolbarSection()->SetCurrentIndex(static_cast<int>(m_coordinateSystem));
    m_editorWindow->GetViewport()->Refresh();
}

ViewportInteraction::CoordinateSystem ViewportInteraction::GetCoordinateSystem() const
{
    return m_coordinateSystem;
}

void ViewportInteraction::InitializeToolbars()
{
    m_editorWindow->GetModeToolbar()->SetCheckedItem(static_cast<int>(m_interactionMode));
    m_editorWindow->GetModeToolbar()->GetAlignToolbarSection()->SetIsVisible(m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR);

    m_editorWindow->GetCoordinateSystemToolbarSection()->SetCurrentIndex(static_cast<int>(m_coordinateSystem));

    UpdateCoordinateSystemToolbarSection();

    bool canvasLoaded = m_editorWindow->GetCanvas().IsValid();
    m_editorWindow->GetMainToolbar()->setEnabled(canvasLoaded);
    m_editorWindow->GetModeToolbar()->setEnabled(canvasLoaded);
    if (!m_editorWindow->GetModeToolbar()->isEnabled())
    {
        m_editorWindow->GetCoordinateSystemToolbarSection()->SetIsEnabled(false);
    }
    m_editorWindow->GetEnterPreviewToolbar()->setEnabled(canvasLoaded);

    AZ::Vector2 canvasSize(1280.0f, 720.0f);
    EBUS_EVENT_ID_RESULT(canvasSize, m_editorWindow->GetCanvas(), UiCanvasBus, GetCanvasSize);
    m_editorWindow->GetCanvasSizeToolbarSection()->SetInitialResolution(canvasSize);

    if (!m_editorWindow->GetCanvas().IsValid())
    {
        SetCanvasToViewportScale(1.0f);
    }

    {
        bool isSnapping = false;
        EBUS_EVENT_ID_RESULT(isSnapping, m_editorWindow->GetCanvas(), UiEditorCanvasBus, GetIsSnapEnabled);

        m_editorWindow->GetCoordinateSystemToolbarSection()->SetSnapToGridIsChecked(isSnapping);
    }

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_editorWindow->GetMainToolbar()))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_editorWindow->GetModeToolbar()))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_editorWindow->GetPreviewToolbar()))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_editorWindow->GetEnterPreviewToolbar()))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

}

const ViewportInteraction::TranslationAndScale& ViewportInteraction::GetCanvasViewportMatrixProps()
{
    return m_canvasViewportMatrixProps;
}

void ViewportInteraction::SetCanvasViewportMatrixProps(const TranslationAndScale& canvasViewportMatrixProps)
{
    m_canvasViewportMatrixProps = canvasViewportMatrixProps;
    UpdateCanvasToViewportMatrix();
    UpdateShouldScaleToFitOnResize();
}

void ViewportInteraction::CenterCanvasInViewport(const AZ::Vector2* newCanvasSize)
{
    GetScaleToFitTransformProps(newCanvasSize, m_canvasViewportMatrixProps);

    // Apply scale and translation changes
    UpdateCanvasToViewportMatrix();
    m_shouldScaleToFitOnViewportResize = true;
}

void ViewportInteraction::GetScaleToFitTransformProps(const AZ::Vector2* newCanvasSize, TranslationAndScale& propsOut)
{
    AZ::Vector2 canvasSize;

    // Normally we can just get the canvas size from GetCanvasSize, but if the canvas
    // size was recently changed, the caller can choose to provide a new canvas size
    // so we don't have to wait for the canvas size to update.
    if (newCanvasSize)
    {
        canvasSize = *newCanvasSize;
    }
    else
    {
        EBUS_EVENT_ID_RESULT(canvasSize, m_editorWindow->GetCanvas(), UiCanvasBus, GetCanvasSize);
    }

    QSize viewportSize = QtHelpers::GetDpiScaledViewportSize(*m_editorWindow->GetViewport());
    const int viewportWidth = viewportSize.width();
    const int viewportHeight = viewportSize.height();

    // We pad the edges of the viewport to allow the user to easily see the borders of
    // the canvas edges, which is especially helpful if there are anchors sitting on
    // the edges of the canvas.
    static const int canvasBorderPaddingInPixels = 32;
    AZ::Vector2 viewportPaddedSize(
        aznumeric_cast<float>(viewportWidth - canvasBorderPaddingInPixels),
        aznumeric_cast<float>(viewportHeight - canvasBorderPaddingInPixels));

    // Guard against very small viewports
    if (viewportPaddedSize.GetX() <= 0.0f)
    {
        viewportPaddedSize.SetX(aznumeric_cast<float>(viewportWidth));
    }

    if (viewportPaddedSize.GetY() <= 0.0f)
    {
        viewportPaddedSize.SetY(aznumeric_cast<float>(viewportHeight));
    }

    // Use a "scale to fit" approach
    const float canvasToViewportScale = AZ::GetMin<float>(
            viewportPaddedSize.GetX() / canvasSize.GetX(),
            viewportPaddedSize.GetY() / canvasSize.GetY());

    const int scaledCanvasWidth = static_cast<int>(canvasSize.GetX() * canvasToViewportScale);
    const int scaledCanvasHeight = static_cast<int>(canvasSize.GetY() * canvasToViewportScale);

    // Centers the canvas within the viewport
    propsOut.translation = AZ::Vector3(
            0.5f * (viewportWidth - scaledCanvasWidth),
            0.5f * (viewportHeight - scaledCanvasHeight),
            0.0f);

    propsOut.scale = canvasToViewportScale;
}

void ViewportInteraction::DecreaseCanvasToViewportScale()
{
    SetCanvasToViewportScale(QuantizeZoomScale(m_canvasViewportMatrixProps.scale - defaultCanvasToViewportScaleIncrement));
}

void ViewportInteraction::IncreaseCanvasToViewportScale()
{
    SetCanvasToViewportScale(QuantizeZoomScale(m_canvasViewportMatrixProps.scale + defaultCanvasToViewportScaleIncrement));
}

void ViewportInteraction::ResetCanvasToViewportScale()
{
    SetCanvasToViewportScale(1.0f);
}

void ViewportInteraction::SetCanvasZoomPercent(float percent)
{
    SetCanvasToViewportScale(percent / 100.0f);
}

void ViewportInteraction::SetCanvasToViewportScale(float newScale, Vec2i* optionalPivotPoint)
{
    static const float minZoom = 0.1f;
    static const float maxZoom = 10.0f;
    const float currentScale = m_canvasViewportMatrixProps.scale;
    m_canvasViewportMatrixProps.scale = AZ::GetClamp(newScale, minZoom, maxZoom);

    if (m_editorWindow->GetCanvas().IsValid())
    {
        // Pivot the zoom based off the center of the viewport's location in canvas space

        // Calculate diff between the number of viewport pixels occupied by the current
        // scaled canvas view and the new one
        AZ::Vector2 canvasSize;
        EBUS_EVENT_ID_RESULT(canvasSize, m_editorWindow->GetCanvas(), UiCanvasBus, GetCanvasSize);
        const AZ::Vector2 scaledCanvasSize(canvasSize * currentScale);
        const AZ::Vector2 newScaledCanvasSize(canvasSize * m_canvasViewportMatrixProps.scale);
        const AZ::Vector2 scaledCanvasSizeDiff(newScaledCanvasSize - scaledCanvasSize);

        // Use the center of our viewport as the pivot point
        Vec2i pivotPoint;
        if (optionalPivotPoint)
        {
            pivotPoint = *optionalPivotPoint;
        }
        else
        {
            pivotPoint = Vec2i(
                    static_cast<int>(m_editorWindow->GetViewport()->size().width() * 0.5f),
                    static_cast<int>(m_editorWindow->GetViewport()->size().height() * 0.5f));
        }

        // Get the distance between our pivot point and the upper-left corner of the
        // canvas (in viewport space)
        const Vec2i canvasUpperLeft(
            static_cast<int32_t>(m_canvasViewportMatrixProps.translation.GetX()),
            static_cast<int32_t>(m_canvasViewportMatrixProps.translation.GetY()));
        Vec2i delta = canvasUpperLeft - pivotPoint;
        const AZ::Vector2 pivotDiff(aznumeric_cast<float>(delta.x), aznumeric_cast<float>(delta.y));

        // Calculate the pivot position relative to the current scaled canvas size. For
        // example, if the pivot position is the upper-left corner of the canvas, this
        // will be (0, 0), whereas if the pivot position is the bottom-right corner of
        // the canvas, this will be (1, 1).
        AZ::Vector2 relativePivotPosition(
            pivotDiff.GetX() / scaledCanvasSize.GetX(),
            pivotDiff.GetY() / scaledCanvasSize.GetY());

        // Use the relative pivot position to essentially determine what percentage of
        // the difference between the two on-screen canvas sizes should be used to move
        // the canvas by to pivot the zoom. For example, if the pivot position is the
        // bottom-right corner of the canvas, then we will use 100% of the difference
        // in on-screen canvas sizes to move the canvas right and up (to maintain the
        // view of the bottom-right corner).
        AZ::Vector2 pivotTranslation(
            scaledCanvasSizeDiff.GetX() * relativePivotPosition.GetX(),
            scaledCanvasSizeDiff.GetY() * relativePivotPosition.GetY());

        m_canvasViewportMatrixProps.translation.SetX(m_canvasViewportMatrixProps.translation.GetX() + pivotTranslation.GetX());
        m_canvasViewportMatrixProps.translation.SetY(m_canvasViewportMatrixProps.translation.GetY() + pivotTranslation.GetY());
    }

    UpdateCanvasToViewportMatrix();
    UpdateShouldScaleToFitOnResize();
}

float ViewportInteraction::QuantizeZoomScale(float newScale)
{
    // Fit to canvas can result in odd zoom scales - when manually zooming we snap it to one of the prefered intervals
    // The prefered intervals are in steps of defaultCanvasToViewportScaleIncrement starting at 100% (or a scale of 1.0)
    float scaleRelativeto1 = newScale - 1.0f;
    float roundedRelative = round(scaleRelativeto1 / defaultCanvasToViewportScaleIncrement) * defaultCanvasToViewportScaleIncrement;
    float quantizedScale = roundedRelative + 1.0f;
    return quantizedScale;
}

void ViewportInteraction::UpdateZoomFactorLabel()
{
    float percentage = m_canvasViewportMatrixProps.scale * 100.0f;
    m_editorWindow->GetMainToolbar()->SetZoomPercent(percentage);
}

AZ::Entity* ViewportInteraction::GetActiveElement() const
{
    return EntityHelpers::GetEntity(m_activeElementId);
}

const AZ::EntityId& ViewportInteraction::GetActiveElementId() const
{
    return m_activeElementId;
}

ViewportHelpers::SelectedAnchors ViewportInteraction::GetGrabbedAnchors() const
{
    return m_grabbedAnchors;
}

void ViewportInteraction::UpdateInteractionType(const AZ::Vector2& mousePosition,
    const QTreeWidgetItemRawPtrQList& selectedItems)
{
    switch (m_interactionMode)
    {
    case InteractionMode::MOVE:
    case InteractionMode::ANCHOR:
    {
        auto selectedElements = SelectionHelpers::GetSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        if (selectedElements.size() == 1 && m_interactionMode == InteractionMode::ANCHOR)
        {
            auto selectedElement = selectedElements.front();
            if (!ViewportHelpers::IsControlledByLayout(selectedElement) &&
                ViewportElement::PickAnchors(selectedElement, mousePosition, m_anchorWhole->GetTextureSize(), m_grabbedAnchors))
            {
                // Hovering over anchors
                m_interactionType = InteractionType::ANCHORS;
                m_activeElementId = selectedElement->GetId();
                return;
            }
        }

        auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : topLevelSelectedElements)
        {
            if (!ViewportHelpers::IsControlledByLayout(element) &&
                ViewportElement::PickAxisGizmo(element, m_coordinateSystem, m_interactionMode, mousePosition, m_lineTriangleX->GetTextureSize(), m_grabbedGizmoParts))
            {
                // Hovering over move gizmo
                m_interactionType = InteractionType::TRANSFORM_GIZMO;
                m_activeElementId = element->GetId();
                return;
            }
        }

        // if hovering over a guide line, then allow moving it or deleting it by moving out of viewport
        if (m_editorWindow->GetViewport()->AreGuidesShown() && !GuideHelpers::AreGuidesLocked(m_editorWindow->GetCanvas()))
        {
            if (GuideHelpers::PickGuide(m_editorWindow->GetCanvas(), mousePosition, m_activeGuideIsVertical, m_activeGuideIndex))
            {
                m_interactionType = InteractionType::GUIDE;
                m_activeElementId.SetInvalid();
                return;
            }
        }

        for (auto element : selectedElements)
        {
            bool isElementUnderCursor = false;
            EBUS_EVENT_ID_RESULT(isElementUnderCursor, element->GetId(), UiTransformBus, IsPointInRect, mousePosition);

            if (isElementUnderCursor)
            {
                // Hovering over a selected element
                m_interactionType = InteractionType::DIRECT;
                m_activeElementId = element->GetId();
                return;
            }
        }
        break;
    }
    case InteractionMode::ROTATE:
    {
        auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : topLevelSelectedElements)
        {
            if (ViewportElement::PickPivot(element, mousePosition, m_pivotIcon->GetTextureSize()))
            {
                // Hovering over pivot
                m_interactionType = InteractionType::PIVOT;
                m_activeElementId = element->GetId();
                return;
            }
        }
        for (auto element : topLevelSelectedElements)
        {
            if (ViewportElement::PickCircleGizmo(element, mousePosition, m_circle->GetTextureSize(), m_grabbedGizmoParts))
            {
                // Hovering over rotate gizmo
                m_interactionType = InteractionType::TRANSFORM_GIZMO;
                m_activeElementId = element->GetId();
                return;
            }
        }
        break;
    }
    case InteractionMode::RESIZE:
    {
        auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : topLevelSelectedElements)
        {
            if (!ViewportHelpers::IsControlledByLayout(element) &&
                ViewportElement::PickAxisGizmo(element, CoordinateSystem::LOCAL, m_interactionMode, mousePosition, m_lineTriangleX->GetTextureSize(), m_grabbedGizmoParts))
            {
                // Hovering over resize gizmo
                m_interactionType = InteractionType::TRANSFORM_GIZMO;
                m_activeElementId = element->GetId();
                return;
            }
        }

        auto selectedElements = SelectionHelpers::GetSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : selectedElements)
        {
            if (!ViewportHelpers::IsControlledByLayout(element))
            {
                // Check for grabbing element edges
                ViewportElement::PickElementEdges(element, mousePosition, g_elementEdgeForgiveness, m_grabbedEdges);
                if (m_grabbedEdges.BothHorizontal() || m_grabbedEdges.BothVertical())
                {
                    // Don't grab both opposite edges
                    m_grabbedEdges.SetAll(false);
                }

                if (m_grabbedEdges.Any())
                {
                    m_interactionType = InteractionType::DIRECT;
                    m_activeElementId = element->GetId();
                    return;
                }
            }
        }
        break;
    }
    default:
    {
        // Do nothing
        break;
    }
    } // switch statement
}

void ViewportInteraction::UpdateCursor()
{
    QCursor cursor = Qt::ArrowCursor;

    if (m_spaceBarIsActive)
    {
        cursor = (m_leftButtonIsActive || m_middleButtonIsActive ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
    }
    else if (m_interactionType == InteractionType::GUIDE)
    {
        if (m_activeGuideIsVertical)
        {
            cursor = Qt::SplitHCursor; // vertical guide
        }
        else
        {
            cursor = Qt::SplitVCursor; // horizontal guide
        }
    }
    else if (m_activeElementId.IsValid())
    {
        if ((m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR) &&
            m_interactionType == InteractionType::DIRECT)
        {
            cursor = Qt::SizeAllCursor;
        }
        else if (m_interactionMode == InteractionMode::ROTATE &&
                 m_interactionType == InteractionType::TRANSFORM_GIZMO)
        {
            cursor = m_cursorRotate;
        }
        else if (m_interactionMode == InteractionMode::RESIZE &&
                 m_interactionType == InteractionType::DIRECT)
        {
            UiTransformInterface::RectPoints rect;
            EBUS_EVENT_ID(m_activeElementId, UiTransformBus, GetViewportSpacePoints, rect);

            float topAngle = RAD2DEG(atan2f(rect.TopRight().GetY() - rect.TopLeft().GetY(), rect.TopRight().GetX() - rect.TopLeft().GetX()));
            float leftAngle = RAD2DEG(atan2f(rect.TopLeft().GetY() - rect.BottomLeft().GetY(), rect.TopLeft().GetX() - rect.BottomLeft().GetX()));
            float topLeftAngle = 0.5f * (topAngle + leftAngle);
            float topRightAngle = ViewportHelpers::GetPerpendicularAngle(topLeftAngle);

            if (m_grabbedEdges.TopLeft() || m_grabbedEdges.BottomRight())
            {
                cursor = ViewportHelpers::GetSizingCursor(topLeftAngle);
            }
            else if (m_grabbedEdges.TopRight() || m_grabbedEdges.BottomLeft())
            {
                cursor = ViewportHelpers::GetSizingCursor(topRightAngle);
            }
            else if (m_grabbedEdges.m_left || m_grabbedEdges.m_right)
            {
                cursor = ViewportHelpers::GetSizingCursor(leftAngle);
            }
            else if (m_grabbedEdges.m_top || m_grabbedEdges.m_bottom)
            {
                cursor = ViewportHelpers::GetSizingCursor(topAngle);
            }
        }
    }
    else if (m_editorWindow->GetViewport()->IsInObjectPickMode())
    {
        cursor = m_editorWindow->GetEntityPickerCursor();
    }

    m_editorWindow->GetViewport()->setCursor(cursor);
}

void ViewportInteraction::UpdateHoverElement(const AZ::Vector2 mousePosition)
{
    m_hoverElement.SetInvalid();
    AZ::Entity* element = nullptr;
    EBUS_EVENT_ID_RESULT(element, m_editorWindow->GetCanvas(), UiCanvasBus, PickElement, mousePosition);
    if (element)
    {
        m_hoverElement = element->GetId();
        SetCursorStr(element->GetName());
    }
    else
    {
        SetCursorStr("");
    }
}

void ViewportInteraction::InvalidateHoverElement()
{
    m_hoverElement.SetInvalid();
    SetCursorStr("");
}

void ViewportInteraction::SetCursorStr(const AZStd::string& cursorStr)
{
    m_cursorStr = cursorStr;
}

void ViewportInteraction::UpdateCanvasToViewportMatrix()
{
    AZ::Vector3 scaleVec3(m_canvasViewportMatrixProps.scale, m_canvasViewportMatrixProps.scale, 1.0f);
    AZ::Matrix4x4 updatedMatrix = AZ::Matrix4x4::CreateScale(scaleVec3);
    updatedMatrix.SetTranslation(m_canvasViewportMatrixProps.translation);

    EBUS_EVENT_ID(m_editorWindow->GetCanvas(), UiCanvasBus, SetCanvasToViewportMatrix, updatedMatrix);

    UpdateZoomFactorLabel();

    // when the zoom or pan changes we need to redraw the rulers
    m_editorWindow->GetViewport()->RefreshRulers();
}

void ViewportInteraction::UpdateShouldScaleToFitOnResize()
{
    // If the current viewport matrix props match the "scale to fit" props,
    // the canvas will scale to fit when the viewport resizes.
    TranslationAndScale props;
    GetScaleToFitTransformProps(nullptr, props);
    m_shouldScaleToFitOnViewportResize = (props == m_canvasViewportMatrixProps);
}

void ViewportInteraction::ProcessInteraction(const AZ::Vector2& mousePosition,
    Qt::KeyboardModifiers modifiers,
    const QTreeWidgetItemRawPtrQList& selectedItems)
{
    // Get the mouse move delta, which is in viewport space.
    AZ::Vector2 delta = mousePosition - m_lastMouseDragPos;
    AZ::Vector3 mouseTranslation(delta.GetX(), delta.GetY(), 0.0f);

    BeginReversibleAction(selectedItems);

    bool ctrlIsPressed = modifiers.testFlag(Qt::ControlModifier);
    if (m_interactionType == InteractionType::NONE)
    {
        if (m_isAreaSelectionActive)
        {
            float mouseDragDistance2 = (mousePosition - m_startMouseDragPos).GetLengthSq();
            if (mouseDragDistance2 >= g_minAreaSelectionDistance2)
            {
                // Area selection
                AZ::Vector2 rectMin(min(m_startMouseDragPos.GetX(), mousePosition.GetX()), min(m_startMouseDragPos.GetY(), mousePosition.GetY()));
                AZ::Vector2 rectMax(max(m_startMouseDragPos.GetX(), mousePosition.GetX()), max(m_startMouseDragPos.GetY(), mousePosition.GetY()));

                LyShine::EntityArray elementsToSelect;
                EBUS_EVENT_ID_RESULT(elementsToSelect, m_editorWindow->GetCanvas(), UiCanvasBus, PickElements, rectMin, rectMax);

                if (ctrlIsPressed)
                {
                    // NOTE: We are fighting against SetSelectedItems a bit here. SetSelectedItems uses Qt
                    // to set the selection and the control and shift modifiers affect its behavior.
                    // When Ctrl is down, unless you pass null or an empty list it adds to the existing
                    // selected items. To get the behavior we want when ctrl is held down we have to clear
                    // the selection before setting it. NOTE: if you area select over a group and (during
                    // same drag) move the cursor so that they are not in the box then they should not
                    // be added to the selection.
                    HierarchyHelpers::SetSelectedItem(m_editorWindow->GetHierarchy(), nullptr);

                    // when control is pressed we add the selected elements in a drag select to the already selected elements
                    // NOTE: It would be nice to allow ctrl-area-select to deselect already selected items. However, the main
                    // level editor does not behave that way and we are trying to be consistent (see LMBR-10377)
                    for (auto element : m_selectedElementsAtSelectionStart)
                    {
                        // if not already in the selectedElements then add it
                        auto iter = AZStd::find(elementsToSelect.begin(), elementsToSelect.end(), element);
                        if (iter == elementsToSelect.end())
                        {
                            elementsToSelect.push_back(element);
                        }
                    }
                }

                HierarchyHelpers::SetSelectedItems(m_editorWindow->GetHierarchy(), &elementsToSelect);
            }
            else
            {
                // Selection area too small, ignore
            }
        }
    }
    else if (m_interactionType == InteractionType::PIVOT)
    {
        // Move the pivot that was grabbed
        ViewportElement::MovePivot(m_lastMouseDragPos, EntityHelpers::GetEntity(m_activeElementId), mousePosition);
    }
    else if (m_interactionType == InteractionType::ANCHORS)
    {
        // Move the anchors of the active element
        ViewportElement::MoveAnchors(m_grabbedAnchors, m_startAnchors, m_startMouseDragPos, EntityHelpers::GetEntity(m_activeElementId), mousePosition, ctrlIsPressed);
    }
    else if (m_interactionType == InteractionType::TRANSFORM_GIZMO)
    {
        // Transform all selected elements by interacting with one element's transform gizmo
        switch (m_interactionMode)
        {
        case InteractionMode::MOVE:
        case InteractionMode::ANCHOR:
            if (m_dragInteraction)
            {
                m_dragInteraction->Update(mousePosition);
            }
            break;
        case InteractionMode::ROTATE:
        {
            LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
            for (auto element : selectedElements)
            {
                ViewportElement::Rotate(m_editorWindow->GetHierarchy(), m_editorWindow->GetCanvas(), m_lastMouseDragPos, m_activeElementId, element, mousePosition);
            }
        }
        break;
        case InteractionMode::RESIZE:
        {
            if (!ViewportHelpers::IsControlledByLayout(EntityHelpers::GetEntity(m_activeElementId)))
            {
                LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
                for (auto element : selectedElements)
                {
                    ViewportElement::ResizeByGizmo(m_editorWindow->GetHierarchy(), m_editorWindow->GetCanvas(), m_grabbedGizmoParts, m_activeElementId, element, mouseTranslation);
                }
            }
        }
        break;
        default:
            AZ_Assert(0, "Unexpected combination of m_interactionMode and m_interactionType.");
            break;
        }
    }
    else if (m_interactionType == InteractionType::DIRECT)
    {
        // Transform all selected elements by interacting with one element directly
        switch (m_interactionMode)
        {
        case InteractionMode::MOVE:
        case InteractionMode::ANCHOR:
            if (m_dragInteraction)
            {
                m_dragInteraction->Update(mousePosition);
            }
            break;
        case InteractionMode::RESIZE:
            // Exception: Direct resizing (grabbing an edge) only affects the element you grabbed
            ViewportElement::ResizeDirectly(m_editorWindow->GetHierarchy(), m_editorWindow->GetCanvas(), m_grabbedEdges, EntityHelpers::GetEntity(m_activeElementId), mouseTranslation);
            break;
        default:
            AZ_Assert(0, "Unexpected combination of m_interactionMode and m_interactionType.");
            break;
        }
    }
    else if (m_interactionType == InteractionType::GUIDE)
    {
        if (m_dragInteraction)
        {
            m_dragInteraction->Update(mousePosition);
        }
    }
    else
    {
        AZ_Assert(0, "Unexpected value for m_interactionType.");
    }

    // Tell the Properties panel to update
    const AZ::Uuid& transformComponentType = InitAndGetTransformComponentType();
    m_editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);
}

void ViewportInteraction::DrawAxisGizmo(Draw2dHelper& draw2d, const AZ::Entity* element, CoordinateSystem coordinateSystem, const ViewportIcon* lineTextureX, const ViewportIcon* lineTextureY)
{
    if (UiTransformBus::FindFirstHandler(element->GetId()))
    {
        AZ::Vector2 pivotPosition;
        AZ::Matrix4x4 transform;

        bool isMoveOrAnchorMode = m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR;

        if (coordinateSystem == CoordinateSystem::LOCAL)
        {
            EBUS_EVENT_ID_RESULT(pivotPosition, element->GetId(), UiTransformBus, GetCanvasSpacePivotNoScaleRotate);

            // LOCAL MOVE in the parent element's LOCAL space.
            AZ::EntityId elementId(isMoveOrAnchorMode ? EntityHelpers::GetParentElement(element)->GetId() : element->GetId());
            EBUS_EVENT_ID(elementId, UiTransformBus, GetTransformToViewport, transform);
        }
        else
        {
            // View coordinate system: do everything in viewport space
            EBUS_EVENT_ID_RESULT(pivotPosition, element->GetId(), UiTransformBus, GetViewportSpacePivot);
            transform = AZ::Matrix4x4::CreateIdentity();
        }

        // Draw up axis
        if (isMoveOrAnchorMode || !ViewportHelpers::IsVerticallyFit(element))
        {
            AZ::Color color = ((m_activeElementId == element->GetId()) && m_grabbedGizmoParts.m_top) ? ViewportHelpers::highlightColor : ViewportHelpers::yColor;
            lineTextureY->Draw(draw2d, pivotPosition, transform, 0.0f, color);
        }

        // Draw right axis
        if (isMoveOrAnchorMode || !ViewportHelpers::IsHorizontallyFit(element))
        {
            AZ::Color color = ((m_activeElementId == element->GetId()) && m_grabbedGizmoParts.m_right) ? ViewportHelpers::highlightColor : ViewportHelpers::xColor;
            lineTextureX->Draw(draw2d, pivotPosition, transform, 0.0f, color);
        }

        // Draw center square
        if (isMoveOrAnchorMode || !ViewportHelpers::IsHorizontallyFit(element) && !ViewportHelpers::IsVerticallyFit(element))
        {
            AZ::Color color = ((m_activeElementId == element->GetId()) && m_grabbedGizmoParts.Both()) ? ViewportHelpers::highlightColor : ViewportHelpers::zColor;
            m_centerSquare->Draw(draw2d, pivotPosition, transform, 0.0f, color);
        }
    }
}

void ViewportInteraction::DrawCircleGizmo(Draw2dHelper& draw2d, const AZ::Entity* element)
{
    if (UiTransformBus::FindFirstHandler(element->GetId()))
    {
        AZ::Vector2 pivotPosition;
        EBUS_EVENT_ID_RESULT(pivotPosition, element->GetId(), UiTransformBus, GetViewportSpacePivot);

        // Draw circle
        AZ::Color color = ((m_activeElementId == element->GetId()) && m_interactionType == InteractionType::TRANSFORM_GIZMO) ? ViewportHelpers::highlightColor : ViewportHelpers::zColor;
        m_circle->Draw(draw2d, pivotPosition, AZ::Matrix4x4::CreateIdentity(), 0.0f, color);
    }
}

void ViewportInteraction::UpdateCoordinateSystemToolbarSection()
{
    // the coordinate system toolbar should only be enabled in move or anchor mode
    bool isMoveOrAnchorMode = m_interactionMode == InteractionMode::MOVE || m_interactionMode == InteractionMode::ANCHOR;
    m_editorWindow->GetCoordinateSystemToolbarSection()->SetIsEnabled(isMoveOrAnchorMode);
}

#include <moc_ViewportInteraction.cpp>
