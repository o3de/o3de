/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
