/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        : QTreeView(pParent)
        , m_queuedMouseEvent(nullptr)
        , m_draggingUnselectedItem(false)
    {
        setUniformRowHeights(true);
        setHeaderHidden(true);

        m_editorEntityFrameworkInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();

        AZ_Assert((m_editorEntityFrameworkInterface != nullptr),
            "EntityOutlinerTreeView requires a EditorEntityFrameworkInterface instance on Construction.");
    }

    EntityOutlinerTreeView::~EntityOutlinerTreeView()
    {
        ClearQueuedMouseEvent();
    }

    void EntityOutlinerTreeView::setAutoExpandDelay(int delay)
    {
        m_expandOnlyDelay = delay;
    }

    void EntityOutlinerTreeView::ClearQueuedMouseEvent()
    {
        if (m_queuedMouseEvent)
        {
            delete m_queuedMouseEvent;
            m_queuedMouseEvent = nullptr;
        }
    }

    void EntityOutlinerTreeView::mousePressEvent(QMouseEvent* event)
    {
        //postponing normal mouse pressed logic until mouse is released or dragged
        //this means selection occurs on mouse released now
        //this is to support drag/drop of non-selected items
        ClearQueuedMouseEvent();
        m_queuedMouseEvent = new QMouseEvent(*event);
    }

    void EntityOutlinerTreeView::mouseReleaseEvent(QMouseEvent* event)
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

    void EntityOutlinerTreeView::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //cancel pending mouse press
        ClearQueuedMouseEvent();
        QTreeView::mouseDoubleClickEvent(event);
    }

    void EntityOutlinerTreeView::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_queuedMouseEvent)
        {
            //disable selection for the pending click if the mouse moved so selection is maintained for dragging
            QAbstractItemView::SelectionMode selectionModeBefore = selectionMode();
            setSelectionMode(QAbstractItemView::NoSelection);

            //treat this as a mouse pressed event to process everything but selection, but use the position data from the mousePress message
            processQueuedMousePressedEvent(m_queuedMouseEvent);

            //restore selection state
            setSelectionMode(selectionModeBefore);
        }

        //process mouse movement as normal, potentially triggering drag and drop
        QTreeView::mouseMoveEvent(event);
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

    void EntityOutlinerTreeView::startDrag(Qt::DropActions supportedActions)
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
                startCustomDrag({ index }, supportedActions);
                return;
            }
        }

        if (!selectionModel()->selectedIndexes().empty())
        {
            startCustomDrag(selectionModel()->selectedIndexes(), supportedActions);
            return;
        }
    }

    void EntityOutlinerTreeView::dragMoveEvent(QDragMoveEvent* event)
    {
        if (m_expandOnlyDelay >= 0)
        {
            m_expandTimer.start(m_expandOnlyDelay, this);
        }

        QTreeView::dragMoveEvent(event);
    }

    void EntityOutlinerTreeView::dropEvent(QDropEvent* event)
    {
        emit ItemDropped();
        QTreeView::dropEvent(event);

        m_draggingUnselectedItem = false;
    }

    void EntityOutlinerTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
    {
        // Paint the branch background as defined by the entity's handler, or its closes ancestor's.
        PaintBranchBackground(painter, rect, index);

        QTreeView::drawBranches(painter, rect, index);
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

    void EntityOutlinerTreeView::startCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions)
    {
        m_draggingUnselectedItem = true;

        //sort by container entity depth and order in hierarchy for proper drag image and drop order
        QModelIndexList indexListSorted = indexList;
        AZStd::unordered_map<AZ::EntityId, AZStd::list<AZ::u64>> locations;
        for (auto index : indexListSorted)
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

        //get the data for the unselected item(s)
        QMimeData* mimeData = model()->mimeData(indexListSorted);
        if (mimeData)
        {
            //initiate drag/drop for the item
            QDrag* drag = new QDrag(this);
            drag->setPixmap(QPixmap::fromImage(createDragImage(indexListSorted)));
            drag->setMimeData(mimeData);
            Qt::DropAction defDropAction = Qt::IgnoreAction;
            if (defaultDropAction() != Qt::IgnoreAction && (supportedActions & defaultDropAction()))
            {
                defDropAction = defaultDropAction();
            }
            else if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove)
            {
                defDropAction = Qt::CopyAction;
            }
            drag->exec(supportedActions, defDropAction);
        }
    }

    QImage EntityOutlinerTreeView::createDragImage(const QModelIndexList& indexList)
    {
        //generate a drag image of the item icon and text, normally done internally, and inaccessible 
        QRect rect(0, 0, 0, 0);
        for (auto index : indexList)
        {
            if (index.column() != 0)
            {
                continue;
            }
            QRect itemRect = visualRect(index);
            rect.setHeight(rect.height() + itemRect.height());
            rect.setWidth(AZStd::GetMax(rect.width(), itemRect.width()));
        }

        QImage dragImage(rect.size(), QImage::Format_ARGB32_Premultiplied);

        QPainter dragPainter(&dragImage);
        dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
        dragPainter.fillRect(dragImage.rect(), Qt::transparent);
        dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        dragPainter.setOpacity(0.35f);
        dragPainter.fillRect(rect, QColor("#222222"));
        dragPainter.setOpacity(1.0f);

        int imageY = 0;
        for (auto index : indexList)
        {
            if (index.column() != 0)
            {
                continue;
            }

            QRect itemRect = visualRect(index);
            dragPainter.drawPixmap(QPoint(0, imageY),
                model()->data(index, Qt::DecorationRole).value<QIcon>().pixmap(QSize(16, 16)));
            dragPainter.setPen(
                model()->data(index, Qt::ForegroundRole).value<QBrush>().color());
            dragPainter.setFont(
                font());
            dragPainter.drawText(QRect(20, imageY, rect.width() - 20, rect.height()),
                model()->data(index, Qt::DisplayRole).value<QString>());
            imageY += itemRect.height();
        }

        dragPainter.end();
        return dragImage;
    }

}

#include <UI/Outliner/moc_EntityOutlinerTreeView.cpp>
