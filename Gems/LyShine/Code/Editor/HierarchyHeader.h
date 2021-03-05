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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QHeaderView>
#endif

class HierarchyWidget;
class QSize;
class QPainter;
class QRect;
class QEvent;
class QIcon;

class HierarchyHeader
    : public QHeaderView
{
    Q_OBJECT

public:

    HierarchyHeader(HierarchyWidget* parent);

    QSize sizeHint() const override;

protected:

    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
    void enterEvent(QEvent* ev) override;

private:

    HierarchyWidget* m_hierarchy;

    QIcon m_visibleIcon;
    QIcon m_selectableIcon;
};
