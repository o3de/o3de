/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityOutlinerTreeView.hxx"
#include "EntityOutlinerListModel.hxx"

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/algorithm.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

#include <AzQtComponents/Components/Style.h>

#include <QDrag>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>

namespace AzToolsFramework
{
    EntityOutlinerTreeView::EntityOutlinerTreeView(QWidget* pParent)
        : AzQtComponents::StyledTreeView(pParent)
        , m_queuedMouseEvent(nullptr)
        , m_draggingUnselectedItem(false)
    {
        setUniformRowHeights(true);
        setHeaderHidden(true);

        m_editorEntityFrameworkInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();
        AZ_Assert((m_editorEntityFrameworkInterface != nullptr),
            "EntityOutlinerTreeView requires a EditorEntityFrameworkInterface instance on Construction.");

        m_readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
        AZ_Assert(
            (m_readOnlyEntityPublicInterface != nullptr),
            "EntityOutlinerTreeView requires a ReadOnlyEntityPublicInterface instance on Construction.");

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

        FocusModeNotificationBus::Handler::BusConnect(editorEntityContextId);

        viewport()->setMouseTracking(true);
    }

    EntityOutlinerTreeView::~EntityOutlinerTreeView()
    {
        FocusModeNotificationBus::Handler::BusDisconnect();

        ClearQueuedMouseEvent();
    }

    void EntityOutlinerTreeView::setAutoExpandDelay(int delay)
    {
        m_expandOnlyDelay = delay;
    }

    void EntityOutlinerTreeView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        AzQtComponents::StyledTreeView::dataChanged(topLeft, bottomRight, roles);

        if (topLeft.isValid() && topLeft.parent() == bottomRight.parent() && topLeft.row() <= bottomRight.row() &&
            topLeft.column() <= bottomRight.column())
        {
            for (int i = topLeft.row(); i <= bottomRight.row(); i++)
            {
                auto modelRow = topLeft.sibling(i, EntityOutlinerListModel::ColumnName);
                if (modelRow.isValid())
                {
                    checkExpandedState(modelRow);
                }
            }
        }
    }

    void EntityOutlinerTreeView::rowsInserted(const QModelIndex& parent, int start, int end)
    {
        if (parent.isValid())
        {
            for (int i = start; i <= end; i++)
            {
                auto modelRow = model()->index(i, EntityOutlinerListModel::ColumnName, parent);
                if (modelRow.isValid())
                {
                    checkExpandedState(modelRow);
                    recursiveCheckExpandedStates(modelRow);
                }
            }
        }
        AzQtComponents::StyledTreeView::rowsInserted(parent, start, end);
    }

    void EntityOutlinerTreeView::recursiveCheckExpandedStates(const QModelIndex& current)
    {
        const int rowCount = model()->rowCount(current);
        for (int i = 0; i < rowCount; i++)
        {
            auto modelRow = model()->index(i, EntityOutlinerListModel::ColumnName, current);
            if (modelRow.isValid())
            {
                checkExpandedState(modelRow);
                recursiveCheckExpandedStates(modelRow);
            }
        }
    }

    void EntityOutlinerTreeView::checkExpandedState(const QModelIndex& current)
    {
        const bool expandState = current.data(EntityOutlinerListModel::ExpandedRole).template value<bool>();
        setExpanded(current, expandState);
    }

    void EntityOutlinerTreeView::mousePressEvent(QMouseEvent* event)
    {
        //postponing normal mouse pressed logic until mouse is released or dragged
        //this means selection occurs on mouse released now
        //this is to support drag/drop of non-selected items
        ClearQueuedMouseEvent();
        m_queuedMouseEvent = new QMouseEvent(*event);
    }

    void EntityOutlinerTreeView::mouseMoveEvent(QMouseEvent* event)
    {
        QModelIndex previousHoveredIndex = m_currentHoveredIndex;
        m_mousePosition = event->pos();

        if (QModelIndex hoveredIndex = indexAt(m_mousePosition); hoveredIndex.column() != EntityOutlinerListModel::ColumnSpacing &&  m_currentHoveredIndex != hoveredIndex)
        {
            m_currentHoveredIndex = hoveredIndex;
        }

        if (m_queuedMouseEvent)
        {
            switch (m_currentDragState)
            {
            case DragState::NO_DRAG:
                {
                    if (m_currentHoveredIndex.isValid() && m_currentHoveredIndex.column() != EntityOutlinerListModel::ColumnSpacing)
                    {
                        m_currentDragState = DragState::ENTITY;
                    }
                    else
                    {
                        m_currentDragState = DragState::SELECT;
                        update();
                    }

                    break;
                }
            case DragState::ENTITY:
                {
                    break;
                }
            case DragState::SELECT:
                {
                    SelectAllEntitiesInSelectionRect();
                    update();
                    break;
                }
            default:
                {
                    break;
                }
            }

            // disable selection for the pending click if the mouse moved so selection is maintained for dragging
            QAbstractItemView::SelectionMode selectionModeBefore = selectionMode();
            setSelectionMode(QAbstractItemView::NoSelection);

            // treat this as a mouse pressed event to process everything but selection, but use the position data from the mousePress
            // message
            processQueuedMousePressedEvent(m_queuedMouseEvent);

            // restore selection state
            setSelectionMode(selectionModeBefore);
        }

        if (previousHoveredIndex != m_currentHoveredIndex)
        {
            update();
        }

        // process mouse movement as normal, potentially triggering drag and drop
        // QTreeView::mouseMoveEvent(event);
    }

    void EntityOutlinerTreeView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_currentDragState == DragState::SELECT)
        {
            SelectAllEntitiesInSelectionRect();
            update();
        }
        else
        {
            if (m_queuedMouseEvent && !m_draggingUnselectedItem)
            {
                // mouseMoveEvent will set the state to be DraggingState, which will make Qt ignore
                // mousePressEvent in QTreeViewPrivate::expandOrCollapseItemAtPos. So we manually
                // and temporarily set it to EditingState.
                QAbstractItemView::State stateBefore = QAbstractItemView::state();
                QAbstractItemView::setState(QAbstractItemView::State::EditingState);

                // treat this as a mouse pressed event to process selection etc
                processQueuedMousePressedEvent(m_queuedMouseEvent);

                QAbstractItemView::setState(stateBefore);
            }
        }

        ClearQueuedMouseEvent();
        m_draggingUnselectedItem = false;
        m_currentDragState = DragState::NO_DRAG;

        QTreeView::mouseReleaseEvent(event);
    }

    void EntityOutlinerTreeView::SelectAllEntitiesInSelectionRect()
    {
        QPoint point1 = (m_queuedMouseEvent->pos());
        QPoint point2 = (m_mousePosition);

        float top(AZStd::min(point1.y(), point2.y()));
        float bottom(AZStd::max(point1.y(), point2.y()));
        float middle(viewport()->rect().center().x());

        QModelIndex topIndex = indexAt(QPoint(middle, top));
        QModelIndex bottomIndex = indexAt(QPoint(middle, bottom));

        QModelIndex firstIndex = indexAt(QPoint(middle, 10));

        if (!topIndex.isValid() && top < 10)
        {
            topIndex = firstIndex;
        }

        if (!topIndex.isValid())
        {
            return;
        }

        QItemSelection selection;

        QModelIndex iter = topIndex;
        selection.select(iter, iter);

        while (iter.isValid() && iter != bottomIndex)
        {
            iter = indexBelow(iter);
            selection.select(iter, iter);
        }

        selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    void EntityOutlinerTreeView::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //cancel pending mouse press
        ClearQueuedMouseEvent();
        QTreeView::mouseDoubleClickEvent(event);
    }

    void EntityOutlinerTreeView::focusInEvent(QFocusEvent* event)
    {
        //cancel pending mouse press
        ClearQueuedMouseEvent();
        QTreeView::focusInEvent(event);
    }

    void EntityOutlinerTreeView::focusOutEvent(QFocusEvent* event)
    {
        //cancel pending mouse press
        ClearQueuedMouseEvent();
        QTreeView::focusOutEvent(event);
    }

    void EntityOutlinerTreeView::startDrag([[maybe_unused]] Qt::DropActions supportedActions)
    {
        /*
        QModelIndex index = indexAt(m_queuedMouseEvent->pos());
        AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());

        AZ::EntityId parentEntityId;
        EditorEntityInfoRequestBus::EventResult(parentEntityId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

        // If the entity is parented to a read-only entity, cancel the drag operation.
        if (m_readOnlyEntityPublicInterface->IsReadOnly(parentEntityId))
        {
            return;
        }

        //if we are attempting to drag an unselected item then we must special case drag and drop logic
        //QAbstractItemView::startDrag only supports selected items
        if (m_queuedMouseEvent)
        {
            if (!index.isValid() || index.column() != 0)
            {
                return;
            }

            if (!selectionModel()->isSelected(index))
            {
                StartCustomDrag({ index }, supportedActions);
                return;
            }
        }

        StyledTreeView::startDrag(supportedActions);
        */
    }

    void EntityOutlinerTreeView::dragMoveEvent([[maybe_unused]] QDragMoveEvent* event)
    {
        /*
        if (m_expandOnlyDelay >= 0)
        {
            m_expandTimer.start(m_expandOnlyDelay, this);
        }

        QTreeView::dragMoveEvent(event);
        */
    }

    void EntityOutlinerTreeView::dropEvent([[maybe_unused]] QDropEvent* event)
    {
        /*
        emit ItemDropped();
        QTreeView::dropEvent(event);

        m_draggingUnselectedItem = false;
        */
    }

    void EntityOutlinerTreeView::ClearQueuedMouseEvent()
    {
        if (m_queuedMouseEvent)
        {
            delete m_queuedMouseEvent;
            m_queuedMouseEvent = nullptr;
        }
    }

    void EntityOutlinerTreeView::leaveEvent([[maybe_unused]] QEvent* event)
    {
        // Only clear the mouse position if the last mouse position registered is inside.
        // This allows drag to select to work correctly in all situations.
        if(this->viewport()->rect().contains(m_mousePosition))
        {
            m_mousePosition = QPoint(-1, -1);
        }
        m_currentHoveredIndex = QModelIndex();
        update();
    }

    void EntityOutlinerTreeView::paintEvent(QPaintEvent* event)
    {
        AzQtComponents::StyledTreeView::paintEvent(event);

        if (m_currentDragState == DragState::SELECT)
        {
            QPainter painter(viewport());

            QPoint point1 = (m_queuedMouseEvent->pos());
            QPoint point2 = (m_mousePosition);

            QPoint topLeft(AZStd::min(point1.x(), point2.x()), AZStd::min(point1.y(), point2.y()));
            QPoint bottomRight(AZStd::max(point1.x(), point2.x()), AZStd::max(point1.y(), point2.y()));

            painter.setBrush(Qt::cyan);
            painter.setPen(Qt::darkCyan);
            painter.drawRect(QRect(topLeft, bottomRight));
        }
    }

    void EntityOutlinerTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
    {
        const bool isEnabled = (this->model()->flags(index) & Qt::ItemIsEnabled);

        const bool isSelected = selectionModel()->isSelected(index);
        const bool isHovered = (index == m_currentHoveredIndex.siblingAtColumn(0)) && isEnabled;

        // Paint the branch Selection/Hover Rect
        PaintBranchSelectionHoverRect(painter, rect, isSelected, isHovered);
        
        // Paint the branch background as defined by the entity's handler, or its closes ancestor's.
        PaintBranchBackground(painter, rect, index);

        QTreeView::drawBranches(painter, rect, index);
    }

    void EntityOutlinerTreeView::PaintBranchSelectionHoverRect(
        QPainter* painter, const QRect& rect, bool isSelected, bool isHovered) const
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);

        if (isSelected || isHovered)
        {
            QPainterPath backgroundPath;
            QRect backgroundRect(rect);

            backgroundPath.addRect(backgroundRect);

            QColor backgroundColor = m_hoverColor;
            if (isSelected)
            {
                backgroundColor = m_selectedColor;
            }

            painter->fillPath(backgroundPath, backgroundColor);
        }

        painter->restore();
    }

    void EntityOutlinerTreeView::PaintBranchBackground(QPainter* painter, const QRect& rect, const QModelIndex& index) const
    {
        // Go through ancestors and add them to the stack
        AZStd::stack<QModelIndex> handlerStack;

        for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
        {
            handlerStack.push(ancestorIndex);
        }

        // Apply the ancestor overrides from top to bottom
        while (!handlerStack.empty())
        {
            QModelIndex ancestorIndex = handlerStack.top();
            handlerStack.pop();

            AZ::EntityId ancestorEntityId(ancestorIndex.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
            auto ancestorUiHandler = m_editorEntityFrameworkInterface->GetHandler(ancestorEntityId);

            if (ancestorUiHandler != nullptr)
            {
                ancestorUiHandler->PaintDescendantBranchBackground(painter, this, rect, ancestorIndex, index);
            }
        }
    }

    void EntityOutlinerTreeView::timerEvent(QTimerEvent* event)
    {
        if (event->timerId() == m_expandTimer.timerId())
        {
            //duplicates functionality from QTreeView, but won't collapse an already expanded item
            QPoint pos = this->viewport()->mapFromGlobal(QCursor::pos());
            if (state() == QAbstractItemView::DraggingState && this->rect().contains(pos))
            {
                QModelIndex index = indexAt(pos);
                if (!isExpanded(index))
                {
                    setExpanded(index, true);
                }
            }
            m_expandTimer.stop();
        }

        QTreeView::timerEvent(event);
    }

    void EntityOutlinerTreeView::processQueuedMousePressedEvent(QMouseEvent* event)
    {
        //interpret the mouse event as a button press
        QMouseEvent mousePressedEvent(
            QEvent::MouseButtonPress,
            event->localPos(),
            event->windowPos(),
            event->screenPos(),
            event->button(),
            event->buttons(),
            event->modifiers(),
            event->source());
        QTreeView::mousePressEvent(&mousePressedEvent);
    }

    void EntityOutlinerTreeView::StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions)
    {
        m_draggingUnselectedItem = true;

        //sort by container entity depth and order in hierarchy for proper drag image and drop order
        QModelIndexList indexListSorted = indexList;
        AZStd::unordered_map<AZ::EntityId, AZStd::list<AZ::u64>> locations;
        for (const auto& index : indexListSorted)
        {
            AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
            AzToolsFramework::GetEntityLocationInHierarchy(entityId, locations[entityId]);
        }
        AZStd::sort(indexListSorted.begin(), indexListSorted.end(), [&locations](const QModelIndex& index1, const QModelIndex& index2) {
            AZ::EntityId e1(index1.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
            AZ::EntityId e2(index2.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
            const auto& locationsE1 = locations[e1];
            const auto& locationsE2 = locations[e2];
            return AZStd::lexicographical_compare(locationsE1.begin(), locationsE1.end(), locationsE2.begin(), locationsE2.end());
        });

        StyledTreeView::StartCustomDrag(indexListSorted, supportedActions);
    }

    void EntityOutlinerTreeView::OnEditorFocusChanged(
        [[maybe_unused]] AZ::EntityId previousFocusEntityId, [[maybe_unused]] AZ::EntityId newFocusEntityId)
    {
        viewport()->repaint();
    }
}

#include <UI/Outliner/moc_EntityOutlinerTreeView.cpp>
