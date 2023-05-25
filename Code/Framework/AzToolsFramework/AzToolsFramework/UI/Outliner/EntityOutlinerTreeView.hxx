/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QBasicTimer>
#include <QEvent>

#include <AzToolsFramework/FocusMode/FocusModeNotificationBus.h>
#include <AzQtComponents/Components/Widgets/TreeView.h>
#endif

#pragma once
class QFocusEvent;
class QMouseEvent;

namespace AzToolsFramework
{
    class EditorEntityUiInterface;
    class ReadOnlyEntityPublicInterface;

    //! This class largely exists to emit events for the OutlinerWidget to listen in on.
    //! The logic for these events is best off not happening within the tree itself,
    //! so it can be re-used in other interfaces.
    //! The OutlinerWidget's need for these events is largely based on the concept of
    //! delaying the Editor selection from updating with mouse interaction to
    //! allow for dragging and dropping of entities from the outliner into the property editor
    //! of other entities. If the selection updates instantly, this would never be possible.
    class EntityOutlinerTreeView
        : public AzQtComponents::StyledTreeView
        , private FocusModeNotificationBus::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerTreeView, AZ::SystemAllocator);

        EntityOutlinerTreeView(QWidget* pParent = NULL);
        virtual ~EntityOutlinerTreeView();

        void setAutoExpandDelay(int delay);

    Q_SIGNALS:
        void ItemDropped();

    protected Q_SLOTS:
        void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
        void rowsInserted(const QModelIndex &parent, int start, int end) override;

    protected:
        // Qt overrides
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

        // FocusModeNotificationBus overrides ...
        void OnEditorFocusChanged(AZ::EntityId previousFocusEntityId, AZ::EntityId newFocusEntityId) override;

        //! Renders the left side of the item: appropriate background, branch lines, icons.
        void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;

        void timerEvent(QTimerEvent* event) override;
    private:
        void ClearQueuedMouseEvent();
        void ProcessQueuedMousePressedEvent(QMouseEvent* event);

        void SelectAllEntitiesInSelectionRect();

        void HandleDrag();
        void StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions) override;

        void RecursiveCheckExpandedStates(const QModelIndex& parent);
        void CheckExpandedState(const QModelIndex& current);

        void PaintBranchBackground(QPainter* painter, const QRect& rect, const QModelIndex& index) const;
        void PaintBranchSelectionHoverRect(QPainter* painter, const QRect& rect, bool isSelected, bool isHovered) const;
        
        QMouseEvent* m_queuedMouseEvent;
        QPoint m_mousePosition;

        bool m_isDragSelectActive = false;
        int m_expandOnlyDelay = -1;
        QBasicTimer m_expandTimer;

        const QColor m_selectedColor = QColor(255, 255, 255, 45);
        const QColor m_hoverColor = QColor(255, 255, 255, 30);
        const QColor m_dragSelectRectColor = QColor(255, 255, 255, 20);
        const QColor m_dragSelectBorderColor = QColor(255, 255, 255);

        EditorEntityUiInterface* m_editorEntityFrameworkInterface = nullptr;
        ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;
    };

}
