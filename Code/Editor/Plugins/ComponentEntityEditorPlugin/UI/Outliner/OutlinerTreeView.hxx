/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef OUTLINER_TREE_VIEW_H
#define OUTLINER_TREE_VIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QBasicTimer>
#include <QEvent>

#include <AzQtComponents/Components/Widgets/TreeView.h>
#endif

#pragma once
class QFocusEvent;
class QMouseEvent;
class OutlinerTreeViewModel;

//! This class largely exists to emit events for the OutlinerWidget to listen in on.
//! The logic for these events is best off not happening within the tree itself,
//! so it can be re-used in other interfaces.
//! The OutlinerWidget's need for these events is largely based on the concept of
//! delaying the Editor selection from updating with mouse interaction to
//! allow for dragging and dropping of entities from the outliner into the property editor
//! of other entities. If the selection updates instantly, this would never be possible.
class OutlinerTreeView
    : public AzQtComponents::StyledTreeView
{
    Q_OBJECT;
public:
    AZ_CLASS_ALLOCATOR(OutlinerTreeView, AZ::SystemAllocator, 0);

    OutlinerTreeView(QWidget* pParent = NULL);
    virtual ~OutlinerTreeView();

    void setAutoExpandDelay(int delay);

    static int GetLayerSquareSize() { return 20; }
Q_SIGNALS:
    void ItemDropped();

protected:
    // Qt overrides
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;

    void timerEvent(QTimerEvent* event) override;
private:
    void ClearQueuedMouseEvent();

    void processQueuedMousePressedEvent(QMouseEvent* event);

    void StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions) override;

    void DrawLayerUI(QPainter* painter, const QRect& rect, const QModelIndex& index) const;

    QMouseEvent* m_queuedMouseEvent;
    bool m_draggingUnselectedItem; // This is set when an item is dragged outside its bounding box.

    int m_expandOnlyDelay = -1;
    QBasicTimer m_expandTimer;
    
    const int m_branchLineWidth = 1;

    QColor GetHierarchyLineColor(bool isSliceEntity, bool isSelected) const;

    struct OutlinerTreeViewColorConfig
    {
        QColor hierarchyLinesSlices = "#7B7B7B";
        QColor hierarchyLinesSlicesSelected = "#7B7B7B";
        QColor hierarchyLinesNonSliceEntities = "transparent";
        QColor hierarchyLinesNonSliceEntitiesSelected = "transparent";

        QColor layerChildBGSelectionColor = "#464747";
        QColor layerChildBackgroundColor = "#333333";
    };

    OutlinerTreeViewColorConfig m_colorConfig;
};

#endif
