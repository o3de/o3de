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
#ifndef COLUMNGROUPHEADERVIEW_H
#define COLUMNGROUPHEADERVIEW_H

#if !defined(Q_MOC_RUN)
#include <QHeaderView>
#include <QVector>
#endif

class ColumnGroupProxyModel;

class ColumnGroupHeaderView
    : public QHeaderView
{
    Q_OBJECT

public:
    ColumnGroupHeaderView(QWidget* parent = 0);

    void setModel(QAbstractItemModel* model) override;

    QSize sizeHint() const override;

    bool IsGroupsShown() const;

    bool event(QEvent* event) override;

public slots:
    void ShowGroups(bool showGroups);

protected slots:
    void updateGeometries() override;

private:
    int GroupViewHeight() const;

private:
    struct Group
    {
        QRect rect;
        int col;
    };

    ColumnGroupProxyModel* m_groupModel;
    bool m_showGroups;

    QVector<Group> m_groups;
};

#endif // COLUMNGROUPHEADERVIEW_H
