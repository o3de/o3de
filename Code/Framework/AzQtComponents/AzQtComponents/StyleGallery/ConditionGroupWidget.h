/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CONDITIONGROUPWIDGET_H
#define CONDITIONGROUPWIDGET_H

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QIcon>
#endif

class QGridLayout;
class ConditionWidget;

class ConditionGroupWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConditionGroupWidget(QWidget *parent = nullptr);
    void appendCondition();
    void removeCondition(int n);
    int count() const;
    void paintEvent(QPaintEvent *) override;
private:
    ConditionWidget* conditionAt(int row) const;
    void updateButtons();
    QGridLayout *const m_layout;
    QIcon m_addIcon;
    QIcon m_delIcon;
};

#endif
