/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ABSTRACTGROUPPROXYMODEL_H
#define ABSTRACTGROUPPROXYMODEL_H

#if !defined(Q_MOC_RUN)
#include <QAbstractProxyModel>
#endif

#if !defined(Q_MOC_RUN)
#include <QPixmap>
#endif

class AbstractGroupProxyModel
    : public QAbstractProxyModel
{
    Q_OBJECT

public:
    AbstractGroupProxyModel(QObject* parent = 0);
    ~AbstractGroupProxyModel();

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

signals:
    void GroupUpdated();

protected:
    virtual QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const = 0;
    virtual bool IsGroupIndex([[maybe_unused]] const QModelIndex& sourceIndex) const { return false; }

    void slotSourceAboutToBeReset();
    void slotSourceReset();

    void RebuildTree();
    int subGroupCount() const;

private:
    struct GroupItem
    {
        QPersistentModelIndex groupSourceIndex;
        QString groupTitle;
        QVector<GroupItem*> subGroups;
        QVector<QPersistentModelIndex> sourceIndexes;

        ~GroupItem()
        {
            qDeleteAll(subGroups);
        }
    };

    GroupItem* FindIndex(const QModelIndex& index, GroupItem* group = 0) const;
    GroupItem* FindGroup(GroupItem* group, GroupItem* parent = 0) const;

    void SourceRowsInserted(const QModelIndex& parent, int from, int to);
    void SourceRowsAboutToBeRemoved(const QModelIndex& parent, int from, int to);
    void SourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    GroupItem* CreateGroupIfNotExists(QStringList group);
    void RemoveEmptyGroup(GroupItem* group);

    GroupItem m_rootItem;
};

#endif // ABSTRACTGROUPPROXYMODEL_H
