/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_LEVEL_ITEM_MODEL_H
#define CRYINCLUDE_EDITOR_LEVEL_ITEM_MODEL_H
#pragma once

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

class QString;
class QStandardItem;

class LevelTreeModelFilter
    : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LevelTreeModelFilter(QObject* parent = nullptr);
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
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
        IsLevelFolderRole,
        // Absolute path of the top-level root this node belongs to. Set on
        // every item so the dialog can resolve a selected item back to its
        // owning root without walking up the tree.
        RootPathRole,
        // True for the project's own root; used to keep project-only
        // affordances (e.g. validation messages) consistent.
        IsProjectRootRole
    };

    explicit LevelTreeModel(QObject* parent = nullptr);
    ~LevelTreeModel();
    // mode = Mode::ExistingOnly only surfaces gems that already have an
    // Assets/Levels folder (browse / open). Mode::AllActive surfaces every
    // active gem so the user can create the first level inside one (save).
    void ReloadTree(bool recurseIfNoLevels, LevelRoots::Mode mode = LevelRoots::Mode::ExistingOnly);
    // Reload the tree from a single explicit root. Used by the Save As
    // dialog after the user picks a target from the combo box - the tree
    // then shows just that root's contents instead of every available one.
    void ReloadTreeFromRoot(bool recurseIfNoLevels, const LevelRoots::Root& root);
    void AddItem(const QString& name, const QModelIndex& parent); // Called when clicking "New folder"
    QVariant data(const QModelIndex& index, int role) const override;
private:
    void ReloadTree(QStandardItem* root, bool recurseIfNoLevels);
};

#endif
