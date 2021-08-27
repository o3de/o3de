/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "OutlinerTreeView.hxx"
#include "OutlinerListModel.hxx"

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/algorithm.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzQtComponents/Components/Style.h>

#include <QDrag>
#include <QPainter>
#include <QHeaderView>
#include <QMouseEvent>

OutlinerTreeView::OutlinerTreeView(QWidget* pParent)
    : AzQtComponents::StyledTreeView(pParent)
    , m_queuedMouseEvent(nullptr)
    , m_draggingUnselectedItem(false)
{
    setUniformRowHeights(true);
    setHeaderHidden(true);
}

OutlinerTreeView::~OutlinerTreeView()
{
    ClearQueuedMouseEvent();
}

void OutlinerTreeView::setAutoExpandDelay(int delay)
{
    m_expandOnlyDelay = delay;
}

void OutlinerTreeView::ClearQueuedMouseEvent()
{
    if (m_queuedMouseEvent)
    {
        delete m_queuedMouseEvent;
        m_queuedMouseEvent = nullptr;
    }
}

void OutlinerTreeView::mousePressEvent(QMouseEvent* event)
{
    //postponing normal mouse pressed logic until mouse is released or dragged
    //this means selection occurs on mouse released now
    //this is to support drag/drop of non-selected items
    ClearQueuedMouseEvent();
    m_queuedMouseEvent = new QMouseEvent(*event);
}

void OutlinerTreeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_queuedMouseEvent && !m_draggingUnselectedItem)
    {
        // mouseMoveEvent will set the state to be DraggingState, which will make Qt ignore 
        // mousePressEvent in QTreeViewPrivate::expandOrCollapseItemAtPos. So we manually 
        // and temporarily set it to EditingState.
        QAbstractItemView::State stateBefore = QAbstractItemView::state();
        QAbstractItemView::setState(QAbstractItemView::State::EditingState);

        //treat this as a mouse pressed event to process selection etc
        processQueuedMousePressedEvent(m_queuedMouseEvent);

        QAbstractItemView::setState(stateBefore);
    }

    ClearQueuedMouseEvent();
    m_draggingUnselectedItem = false;

    QTreeView::mouseReleaseEvent(event);
}

void OutlinerTreeView::mouseDoubleClickEvent(QMouseEvent* event)
{
    //cancel pending mouse press
    ClearQueuedMouseEvent();
    QTreeView::mouseDoubleClickEvent(event);
}

void OutlinerTreeView::mouseMoveEvent(QMouseEvent* event)
{
    // Store selection mode
    QAbstractItemView::SelectionMode selectionModeBefore = selectionMode();

    // Disable selection for the pending click so selection is maintained for dragging
    setSelectionMode(QAbstractItemView::NoSelection);

    // If a mouse event is queued, treat this as a mouse pressed event to process everything
    // but selection, but use the position data from the mousePress message
    if (m_queuedMouseEvent)
    {
        processQueuedMousePressedEvent(m_queuedMouseEvent);
    }

    // Process mouse movement as normal, potentially triggering drag and drop
    QTreeView::mouseMoveEvent(event);

    // Restore selection state
    setSelectionMode(selectionModeBefore);
}

void OutlinerTreeView::focusInEvent(QFocusEvent* event)
{
    //cancel pending mouse press
    ClearQueuedMouseEvent();
    QTreeView::focusInEvent(event);
}

void OutlinerTreeView::focusOutEvent(QFocusEvent* event)
{
    //cancel pending mouse press
    ClearQueuedMouseEvent();
    QTreeView::focusOutEvent(event);
}

void OutlinerTreeView::startDrag(Qt::DropActions supportedActions)
{
    //if we are attempting to drag an unselected item then we must special case drag and drop logic
    //QAbstractItemView::startDrag only supports selected items
    if (m_queuedMouseEvent)
    {
        QModelIndex index = indexAt(m_queuedMouseEvent->pos());
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
}

void OutlinerTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    if (m_expandOnlyDelay >= 0)
    {
        m_expandTimer.start(m_expandOnlyDelay, this);
    }

    QTreeView::dragMoveEvent(event);
}

void OutlinerTreeView::dropEvent(QDropEvent* event)
{
    emit ItemDropped();
    QTreeView::dropEvent(event);

    m_draggingUnselectedItem = false;
}

QColor OutlinerTreeView::GetHierarchyLineColor(bool isSliceEntity, bool isSelected) const
{
    if (isSliceEntity)
    {
        if (isSelected)
        {
            return m_colorConfig.hierarchyLinesSlicesSelected;
        }
        else
        {
            return m_colorConfig.hierarchyLinesSlices;
        }
    }
    else
    {
        if (isSelected)
        {
            return m_colorConfig.hierarchyLinesNonSliceEntitiesSelected;
        }
        else
        {
            return m_colorConfig.hierarchyLinesNonSliceEntities;
        }
    }
}

void OutlinerTreeView::DrawLayerUI(QPainter* painter, const QRect& rect, const QModelIndex& index) const
{
    bool isSelected = selectionModel()->isSelected(index);

    QColor layerBranchesBGColor = isSelected ? m_colorConfig.layerChildBGSelectionColor : m_colorConfig.layerChildBackgroundColor;

    painter->save();
    painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);

    bool hasLayerAncestor = false;
    for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
    {
        auto ancestorType = OutlinerListModel::EntryType(ancestorIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
        if (ancestorType == OutlinerListModel::LayerType)
        {
            hasLayerAncestor = true;
            break;
        }
    }

    if (hasLayerAncestor)
    {
        QPainterPath layerBGPath;
        QRect layerBGRect(rect);
        layerBGRect.setLeft(layerBGRect.left() + indentation());
        layerBGPath.addRect(layerBGRect);
        painter->fillPath(layerBGPath, layerBranchesBGColor);
    }

    OutlinerListModel::EntryType indexEntryType = OutlinerListModel::EntryType(index.data(OutlinerListModel::EntityTypeRole).value<int>());
    if (indexEntryType == OutlinerListModel::LayerType)
    {
        QColor layerColor = index.data(OutlinerListModel::LayerColorRole).value<QColor>();
        QPainterPath layerIconPath;
        const int layerSquareSize = GetLayerSquareSize();
        QPoint layerBoxOffset(1 + OutlinerListModel::GetLayerStripeWidth()*2, (rect.height() - layerSquareSize) / 2);
        QRect layerIconRect(rect.topRight() + layerBoxOffset, QSize(layerSquareSize, layerSquareSize));
        layerIconPath.addRect(layerIconRect);
        painter->fillPath(layerIconPath, layerColor);
    }

    painter->restore();
}

void OutlinerTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
{
    DrawLayerUI(painter, rect, index);

    // Make sure the base class is called after the layer rect is drawn,
    // so that the foldout arrow draws on top of the layer color.
    QTreeView::drawBranches(painter, rect, index);

    // No need to draw connecting lines if this has no parent.
    if (!index.parent().isValid())
    {
        return;
    }

    QPen branchLinePen;
    branchLinePen.setWidthF(m_branchLineWidth);

    int lineBaseX = rect.right();
    QModelIndex previousIndex = index;
    for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
    {
        auto ancestorType = OutlinerListModel::EntryType(ancestorIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
        bool isSliceEntity = ancestorType == OutlinerListModel::SliceEntityType || ancestorType == OutlinerListModel::SliceHandleType;
        bool isSelected = selectionModel()->isSelected(index);
        branchLinePen.setColor(GetHierarchyLineColor(isSliceEntity, isSelected));

        // Layers don't have connecting lines drawn.
        if (ancestorType == OutlinerListModel::LayerType)
        {
            // Layers can only have other layers as parents, so once a layer is found in a hierarchy
            // the line drawing can stop.
            break;
        }
        painter->save();
        painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);
        painter->setPen(branchLinePen);
        int rectHalfHeight = rect.height() / 2;
        if (previousIndex == index)
        {
            // draw a horizontal line from the parent branch to the item
            // if the item has children offset the drawn line to compensate for drawn expander buttons
            bool hasChildren = previousIndex.model()->index(0, 0, previousIndex).isValid();
            int horizontalLineY = rect.top() + rectHalfHeight;
            int horizontalLineLeft = static_cast<int>(rect.right() - indentation() * 1.5f);
            int horizontalLineRight = hasChildren ? (lineBaseX - indentation()) : static_cast<int>(lineBaseX - indentation() * 0.5f);
            painter->drawLine(horizontalLineLeft, horizontalLineY, horizontalLineRight, horizontalLineY);
        }

        // draw a vertical line segment connecting parent to child and child to child
        // if this is the last item, only draw half the line to terminate the segment
        bool hasNext = previousIndex.sibling(previousIndex.row() + 1, previousIndex.column()).isValid();
        if (hasNext || previousIndex == index)
        {
            int verticalLineX = static_cast<int>(lineBaseX - indentation() * 1.5f);
            int verticalLineTop = rect.top();
            int verticalLineBottom = hasNext ? rect.bottom() : rect.bottom() - rectHalfHeight;
            painter->drawLine(verticalLineX, verticalLineTop, verticalLineX, verticalLineBottom);
        }

        painter->restore();

        lineBaseX -= indentation();
        previousIndex = ancestorIndex;
    }
}

void OutlinerTreeView::timerEvent(QTimerEvent* event)
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

void OutlinerTreeView::processQueuedMousePressedEvent(QMouseEvent* event)
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

void OutlinerTreeView::StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions)
{
    m_draggingUnselectedItem = true;

    //sort by container entity depth and order in hierarchy for proper drag image and drop order
    QModelIndexList indexListSorted = indexList;
    AZStd::unordered_map<AZ::EntityId, AZStd::list<AZ::u64>> locations;
    for (const auto& index : indexListSorted)
    {
        AZ::EntityId entityId(index.data(OutlinerListModel::EntityIdRole).value<AZ::u64>());
        AzToolsFramework::GetEntityLocationInHierarchy(entityId, locations[entityId]);
    }
    AZStd::sort(indexListSorted.begin(), indexListSorted.end(), [&locations](const QModelIndex& index1, const QModelIndex& index2) {
        AZ::EntityId e1(index1.data(OutlinerListModel::EntityIdRole).value<AZ::u64>());
        AZ::EntityId e2(index2.data(OutlinerListModel::EntityIdRole).value<AZ::u64>());
        const auto& locationsE1 = locations[e1];
        const auto& locationsE2 = locations[e2];
        return AZStd::lexicographical_compare(locationsE1.begin(), locationsE1.end(), locationsE2.begin(), locationsE2.end());
    });

    StyledTreeView::StartCustomDrag(indexListSorted, supportedActions);
}

#include <UI/Outliner/moc_OutlinerTreeView.cpp>

