/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "HotTrackingTreeCtrl.h"

// Qt
#include <QMouseEvent>


CHotTrackingTreeCtrl::CHotTrackingTreeCtrl(QWidget* parent)
    : QTreeWidget(parent)
{
    setMouseTracking(true);
    m_hHoverItem = NULL;
}

void CHotTrackingTreeCtrl::mouseMoveEvent(QMouseEvent* event)
{
    QTreeWidgetItem* hItem = itemAt(event->pos());

    if (m_hHoverItem != NULL)
    {
        QFont font = m_hHoverItem->font(0);
        font.setBold(false);
        m_hHoverItem->setFont(0, font);
        m_hHoverItem = NULL;
    }

    if (hItem != NULL)
    {
        QFont font = hItem->font(0);
        font.setBold(true);
        hItem->setFont(0, font);
        m_hHoverItem = hItem;
    }

    QTreeWidget::mouseMoveEvent(event);
}

#include <Controls/moc_HotTrackingTreeCtrl.cpp>
