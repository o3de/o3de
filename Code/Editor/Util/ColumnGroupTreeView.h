/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef COLUMNGROUPTREEVIEW_H
#define COLUMNGROUPTREEVIEW_H

#if !defined(Q_MOC_RUN)
#include <QTreeView>
#include <QPainter>
#endif

class ColumnGroupProxyModel;
class ColumnGroupHeaderView;

class ColumnGroupTreeView
    : public QTreeView
{
    Q_OBJECT

public:
    ColumnGroupTreeView(QWidget* parent = 0);

    void setModel(QAbstractItemModel* model) override;

    bool IsGroupsShown() const;

    QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
    QModelIndex mapFromSource(const QModelIndex& sourceModel) const;

public slots:
    void ShowGroups(bool showGroups);

    void Sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    void ToggleSortOrder(int column);

    void AddGroup(int column);
    void RemoveGroup(int column);
    void SetGroups(const QVector<int>& columns);
    void ClearGroups();
    QVector<int> Groups() const;

protected:
    void paintEvent(QPaintEvent* event) override
    {
        if (model() && model()->rowCount() > 0)
        {
            QTreeView::paintEvent(event);
        }
        else
        {
            const QMargins margins(2, 2, 2, 2);
            QPainter painter(viewport());
            QString text(tr("There are no items to show."));
            QRect textRect = painter.fontMetrics().boundingRect(text).marginsAdded(margins);
            textRect.moveCenter(viewport()->rect().center());
            textRect.moveTop(viewport()->rect().top());
            painter.drawText(textRect, Qt::AlignCenter, text);
        }
    }

private slots:
    void SaveOpenState();
    void RestoreOpenState();
    void SpanGroups(const QModelIndex& index = QModelIndex());

private:
    ColumnGroupHeaderView* m_header;
    ColumnGroupProxyModel* m_groupModel;
    QSet<QString> m_openNodes;
};

#endif // COLUMNGROUPTREEVIEW_H
