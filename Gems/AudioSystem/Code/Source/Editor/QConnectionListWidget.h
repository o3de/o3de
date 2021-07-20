/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <QListWidget>
#include <QPainter>
#include <QRect>


//-----------------------------------------------------------------------------------------------//
class QConnectionListWidget
    : public QListWidget
{
public:
    QConnectionListWidget(QWidget* pParent)
        : QListWidget(pParent)
    {}

private:
    void paintEvent(QPaintEvent* pEvent) override
    {
        QListView::paintEvent(pEvent);
        if (count() == 0)
        {
            QPainter painter(viewport());
            QRect area = rect();
            painter.drawText(area, Qt::AlignCenter, "Drag and drop a control to connect to it");
            area.setY(area.y() - 25);
            painter.drawText(area, Qt::AlignCenter, "No connections");
        }
    }
};
