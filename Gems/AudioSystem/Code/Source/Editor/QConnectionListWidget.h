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
