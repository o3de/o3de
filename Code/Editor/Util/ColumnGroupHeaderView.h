/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
