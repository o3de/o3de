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
    {
        setUniformRowHeights(false);
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
                    CheckExpandedState(modelRow);
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
                    CheckExpandedState(modelRow);
                    RecursiveCheckExpandedStates(modelRow);
                }
            }
        }
        AzQtComponents::StyledTreeView::rowsInserted(parent, start, end);
    }

    void EntityOutlinerTreeView::RecursiveCheckExpandedStates(const QModelIndex& current)
    {
        const int rowCount = model()->rowCount(current);
        for (int i = 0; i < rowCount; i++)
        {
            auto modelRow = model()->index(i, EntityOutlinerListModel::ColumnName, current);
            if (modelRow.isValid())
            {
                CheckExpandedState(modelRow);
                RecursiveCheckExpandedStates(modelRow);
            }
        }
    }

    void EntityOutlinerTreeView::CheckExpandedState(const QModelIndex& current)
    {
        const bool expandState = current.data(EntityOutlinerListModel::ExpandedRole).template value<bool>();
        setExpanded(current, expandState);
    }

    void EntityOutlinerTreeView::mousePressEvent(QMouseEvent* event)
    {
        // Postponing normal mouse press logic until mouse is released or dragged.
        // This allows drag/drop of non-selected items.
        ClearQueuedMouseEvent();
        m_queuedMouseEvent = new QMouseEvent(*event);
    }

    void EntityOutlinerTreeView::mouseMoveEvent(QMouseEvent* event)
    {
        // Prevent multiple updates throughout the function for changing UIs.
        bool forceUpdate = false;

        m_mousePosition = event->pos();
        
        if (m_queuedMouseEvent)
        {
            if (!m_isDragSelectActive)
            {
                // Determine whether the mouse move should trigger a rect selection or an entity drag.
                QModelIndex clickedIndex = indexAt(m_queuedMouseEvent->pos());
                // Even though the drag started on an index, we want to trigger a drag select from the last column.
                // This is to allow drag selection to be triggered from anywhere in the hierarchy.
                if (clickedIndex.isValid() && clickedIndex.column() != EntityOutlinerListModel::ColumnSpacing)
                {
                    HandleDrag();
                }
                else
                {
                    m_isDragSelectActive = true;
                    forceUpdate = true;
                }
            }
            else
            {
                SelectAllEntitiesInSelectionRect();
                forceUpdate = true;
            }
        }

        if (forceUpdate)
        {
            update();
        }
    }

    void EntityOutlinerTreeView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_isDragSelectActive)
        {
            SelectAllEntitiesInSelectionRect();
            update();
        }
        else if (m_queuedMouseEvent)
        {
            ProcessQueuedMousePressedEvent(m_queuedMouseEvent);
        }

        ClearQueuedMouseEvent();
        m_isDragSelectActive = false;

        QTreeView::mouseReleaseEvent(event);
    }

    void EntityOutlinerTreeView::mouseDoubleClickEvent(QMouseEvent* event)
    {
        // Cancel pending mouse press.
        ClearQueuedMouseEvent();
        QTreeView::mouseDoubleClickEvent(event);
    }

    void EntityOutlinerTreeView::focusInEvent(QFocusEvent* event)
    {
        // Cancel pending mouse press.
        ClearQueuedMouseEvent();
        QTreeView::focusInEvent(event);
    }

    void EntityOutlinerTreeView::focusOutEvent(QFocusEvent* event)
    {
        // Cancel pending mouse press.
        ClearQueuedMouseEvent();
        QTreeView::focusOutEvent(event);
    }

    void EntityOutlinerTreeView::dragMoveEvent([[maybe_unused]] QDragMoveEvent* event)
    {
        if (m_expandOnlyDelay >= 0)
        {
            m_expandTimer.start(m_expandOnlyDelay, this);
        }

        QTreeView::dragMoveEvent(event);
    }

    void EntityOutlinerTreeView::dropEvent([[maybe_unused]] QDropEvent* event)
    {
        emit ItemDropped();
        QTreeView::dropEvent(event);

        ClearQueuedMouseEvent();
    }

    void EntityOutlinerTreeView::HandleDrag()
    {
        // Retrieve the index at the click position.
        QModelIndex indexAtClick = indexAt(m_queuedMouseEvent->pos()).siblingAtColumn(EntityOutlinerListModel::ColumnName);

        AZ::EntityId entityId(indexAtClick.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
        AZ::EntityId parentEntityId;
        EditorEntityInfoRequestBus::EventResult(parentEntityId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

        // If the entity is parented to a read-only entity, cancel the drag operation.
        if (m_readOnlyEntityPublicInterface->IsReadOnly(parentEntityId))
        {
            return;
        }

        // If the index is selected, we should move the whole selection.
        if (selectionModel()->isSelected(indexAtClick))
        {
            StartCustomDrag(selectionModel()->selectedIndexes(), defaultDropAction());
        }
        else
        {
            StartCustomDrag(QModelIndexList{ indexAtClick }, defaultDropAction());
        }
    }

    void EntityOutlinerTreeView::SelectAllEntitiesInSelectionRect()
    {
        if (!m_queuedMouseEvent)
        {
            return;
        }

        // Retrieve the two opposing corners of the rect.
        const QPoint point1 = (m_queuedMouseEvent->pos());  // The position the drag operation started at.
        const QPoint point2 = (m_mousePosition);            // The current mouse position.

        // Determine which point's y is the top and which is the bottom.
        const int top(AZStd::min(point1.y(), point2.y()));
        const int bottom(AZStd::max(point1.y(), point2.y()));
        // We don't really need the x values for the rect, just use the center of the viewport.
        const int middle(viewport()->rect().center().x());

        // Find the extremes of the range of indices that are in the selection rect.
        QModelIndex topIndex = indexAt(QPoint(middle, top));
        const QModelIndex bottomIndex = indexAt(QPoint(middle, bottom));

        // If we have no top index, the mouse may have been dragged above the top item. Let's try to course correct.
        const int topDistanceForFirstItem = 10; // A reasonable distance from the top we're sure to encounter the first item.
        const QModelIndex firstIndex = indexAt(QPoint(middle, topDistanceForFirstItem));

        if (!topIndex.isValid() && top < topDistanceForFirstItem)
        {
            topIndex = firstIndex;
        }

        // We can assume that if topIndex is still invalid, it was below the last item in the hierarchy, hence no selection is made.
        if (!topIndex.isValid())
        {
            selectionModel()->clear();
            return;
        }

        QItemSelection selection;

        // Starting from the top index, traverse all visible elements of the list and select them until the bottom index is hit.
        // If the bottom index is undefined, just keep going to the end.
        QModelIndex iter = topIndex;
        selection.select(iter, iter);

        while (iter.isValid() && iter != bottomIndex)
        {
            iter = indexBelow(iter);
            selection.select(iter, iter);
        }

        selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
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
        ClearQueuedMouseEvent();

        // Only clear the mouse position if the last mouse position registered is inside.
        // This allows drag to select to work correctly in all situations.
        if(this->viewport()->rect().contains(m_mousePosition))
        {
            m_mousePosition = QPoint(-1, -1);
        }
        update();
    }

    void EntityOutlinerTreeView::paintEvent(QPaintEvent* event)
    {
        AzQtComponents::StyledTreeView::paintEvent(event);

        // Draw the drag selection rect.
        if (m_isDragSelectActive && m_queuedMouseEvent)
        {
            // Create a painter to draw on the viewport.
            QPainter painter(viewport());

            // Retrieve the two corners of the rect.
            const QPoint point1 = (m_queuedMouseEvent->pos());  // The position the drag operation started at.
            const QPoint point2 = (m_mousePosition);            // The current mouse position.

            // We need the top left and bottom right corners, which may not be the two corners we got above.
            // So we composite the corners based on the coordinates of the points.
            const QPoint topLeft(AZStd::min(point1.x(), point2.x()), AZStd::min(point1.y(), point2.y()));
            const QPoint bottomRight(AZStd::max(point1.x(), point2.x()), AZStd::max(point1.y(), point2.y()));

            // Paint the rect.
            painter.setBrush(m_dragSelectRectColor);
            painter.setPen(m_dragSelectBorderColor);
            painter.drawRect(QRect(topLeft, bottomRight));
        }
    }

    void EntityOutlinerTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
    {
        const bool isEnabled = (this->model()->flags(index) & Qt::ItemIsEnabled);

        const bool isSelected = selectionModel()->isSelected(index);
        QModelIndex hoveredIndex = indexAt(m_mousePosition);
        const bool isHovered = index == hoveredIndex.siblingAtColumn(0) && isEnabled;

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

    void EntityOutlinerTreeView::ProcessQueuedMousePressedEvent(QMouseEvent* event)
    {
        QModelIndex clickedIndex = indexAt(m_queuedMouseEvent->pos());
        if (!clickedIndex.isValid() || clickedIndex.column() != EntityOutlinerListModel::ColumnSpacing)
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
    }

    void EntityOutlinerTreeView::StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions)
    {
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
