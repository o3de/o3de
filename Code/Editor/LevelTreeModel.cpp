/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "LevelTreeModel.h"

// Editor
#include "LevelFileDialog.h"

// Folder in which levels are stored
static const char kLevelsFolder[] = "Levels";

LevelTreeModelFilter::LevelTreeModelFilter(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

QVariant LevelTreeModelFilter::data(const QModelIndex& index, int role) const
{
    if (!sourceModel())
    {
        return QVariant();
    }

    return sourceModel()->data(mapToSource(index), role);
}

bool LevelTreeModelFilter::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (m_filterText.isEmpty())
    {
        return true;
    }

    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    if (!idx.isValid())
    {
        return false;
    }

    const QString text = idx.data(Qt::DisplayRole).toString();
    if (text.contains(m_filterText, Qt::CaseInsensitive))
    {
        return true;
    }

    // Recurse into the children
    const int count = sourceModel()->rowCount(idx);
    for (int i = 0; i < count; ++i)
    {
        if (filterAcceptsRow(i, idx))
        {
            return true;
        }
    }

    return false;
}

void LevelTreeModelFilter::setFilterText(const QString& text)
{
    QString lowerText = text.toLower();
    if (m_filterText != lowerText)
    {
        m_filterText = lowerText;
        invalidateFilter();
    }
}

LevelTreeModel::LevelTreeModel(QObject* parent)
    : QStandardItemModel(parent)
{
}

LevelTreeModel::~LevelTreeModel()
{
}

QVariant LevelTreeModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole)
    {
        const bool isLevelFolder = index.data(LevelTreeModel::IsLevelFolderRole).toBool();
        if (isLevelFolder)
        {
            return QIcon(":/img/tree_view_level.png");
        }
        else
        {
            return QIcon(":/img/tree_view_folder.png");
        }
    }

    return QStandardItemModel::data(index, role);
}

void LevelTreeModel::ReloadTree(bool recurseIfNoLevels)
{
    clear();

    QString levelsFolder = QString("%1/%2").arg(Path::GetEditingGameDataFolder().c_str()).arg(kLevelsFolder);
    QStandardItem* root = new QStandardItem(kLevelsFolder);
    root->setData(levelsFolder, FullPathRole);
    root->setEditable(false);
    invisibleRootItem()->appendRow(root);
    ReloadTree(root, recurseIfNoLevels);
}

void LevelTreeModel::ReloadTree(QStandardItem* root, bool recurseIfNoLevels)
{
    QStringList levelFiles;
    const QString parentFullPath = root->data(FullPathRole).toString();
    const bool isLevelFolder = CLevelFileDialog::CheckLevelFolder(parentFullPath, &levelFiles);
    root->setData(isLevelFolder, IsLevelFolderRole);

    // Recurse to sub folders if this is not a level folder
    // Also recurse to sub folders if we detected a level folder
    // but there are no level files in it in open mode
    if (!isLevelFolder || (recurseIfNoLevels && levelFiles.empty()))
    {
        QDir currentDir(parentFullPath);
        currentDir.setFilter(QDir::NoDot | QDir::NoDotDot | QDir::Dirs);
        const QStringList subFolders = currentDir.entryList();
        foreach (const QString &subFolder, subFolders)
        {
            auto child = new QStandardItem(subFolder);
            child->setData(parentFullPath + "/" + subFolder, FullPathRole);
            child->setEditable(false);
            root->appendRow(child);
            ReloadTree(child, recurseIfNoLevels);
        }
    }
    else
    {
        // eTreeImage_Level
        // Support for legacy folder structure: Multiple cry files in level folder
        if (recurseIfNoLevels && levelFiles.size() > 1)
        {
            foreach (const QString &levelFile, levelFiles)
            {
                auto child = new QStandardItem(levelFile);
                child->setData(root->data(FullPathRole).toString() + "/" + levelFile, FullPathRole);
                child->setData(false, IsLevelFolderRole);
                child->setEditable(false);
                root->appendRow(child);
            }
        }
    }
}

void LevelTreeModel::AddItem(const QString& name, const QModelIndex& parent)
{
    auto item = new QStandardItem(name);
    item->setData(false, IsLevelFolderRole);
    item->setEditable(false);
    QStandardItem* parentItem = itemFromIndex(parent);
    if (parentItem)
    {
        const QString parentPath = parent.data(FullPathRole).toString();
        item->setData(parentPath + "/" + name, FullPathRole);
        parentItem->appendRow(item);
    }
}

#include <moc_LevelTreeModel.cpp>
