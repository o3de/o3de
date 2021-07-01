/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CONDITIONWIDGET_H
#define CONDITIONWIDGET_H

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

class ConditionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConditionWidget(QWidget *parent = nullptr);

signals:
    void geometryChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
};

#endif
