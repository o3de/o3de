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
