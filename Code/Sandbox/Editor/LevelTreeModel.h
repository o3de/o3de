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

#ifndef CRYINCLUDE_EDITOR_LEVEL_ITEM_MODEL_H
#define CRYINCLUDE_EDITOR_LEVEL_ITEM_MODEL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#endif
class QString;
class QStandardItem;

class LevelTreeModelFilter
    : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LevelTreeModelFilter(QObject* parent = nullptr);
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
    void setFilterText(const QString&);
    QVariant data(const QModelIndex& index, int role) const override;
private:
    QString m_filterText;
};

class LevelTreeModel
    : public QStandardItemModel
{
    Q_OBJECT
public:
    enum Role
    {
        FullPathRole = Qt::UserRole + 1,
        IsLevelFolderRole
    };

    explicit LevelTreeModel(QObject* parent = nullptr);
    ~LevelTreeModel();
    void ReloadTree(bool recurseIfNoLevels);
    void AddItem(const QString& name, const QModelIndex& parent); // Called when clicking "New folder"
    QVariant data(const QModelIndex& index, int role) const override;
private:
    void ReloadTree(QStandardItem* root, bool recurseIfNoLevels);
};

#endif
