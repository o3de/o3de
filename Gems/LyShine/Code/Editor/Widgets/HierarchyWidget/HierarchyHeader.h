/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QHeaderView>
#include <QIcon>

class HierarchyWidget;
class QSize;
class QPainter;
class QRect;
class QEvent;

class HierarchyHeader : public QHeaderView
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
