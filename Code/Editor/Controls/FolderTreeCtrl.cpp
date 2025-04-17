/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "FolderTreeCtrl.h"

// Qt
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include <AzCore/std/algorithm.h>
#include <AzCore/IO/SystemFile.h>

#include <AzQtComponents/Utilities/DesktopUtilities.h> // for AzQtComponents::ShowFileOnDesktop

//////////////////////////////////////////////////////////////////////////
// CFolderTreeCtrl
//////////////////////////////////////////////////////////////////////////

CFolderTreeCtrl::CFolderTreeCtrl(QWidget* parent)
    : QTreeView(parent)
    , m_folderIcon(":/TreeView/folder-icon.svg")
    , m_fileIcon(":/TreeView/default-icon.svg")
    , m_model(new QStandardItemModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setRecursiveFilteringEnabled(true);
    setModel(m_proxyModel);

    QObject::connect(this, &QTreeView::doubleClicked, this, &CFolderTreeCtrl::OnIndexDoubleClicked);
}

void CFolderTreeCtrl::Configure(
    const AZStd::vector<QString>& folders, const QString& fileNameSpec, const QString& rootName, bool bEnabledMonitor, bool bFlatTree)
{
    m_folders.clear();
    AZStd::ranges::copy_if(
        folders,
        AZStd::back_inserter(m_folders),
        [](const auto& path)
        {
            if (AZ::IO::SystemFile::Exists(path.toLocal8Bit().constData()) || AZ::IO::SystemFile::IsDirectory(path.toLocal8Bit().constData()))
            {
                return true;
            }
            return false;
        });
    AZStd::transform(m_folders.begin(), m_folders.end(), m_folders.begin(), 
        [](auto& item) {
            return Path::RemoveBackslash(Path::ToUnixPath(item));
        });

    m_fileNameSpec = fileNameSpec;
    m_rootName = rootName;
    m_bEnableMonitor = bEnabledMonitor;
    m_bFlatStyle = bFlatTree;

    if (m_rootTreeItem)
    {
        delete m_rootTreeItem;
        m_rootTreeItem = nullptr;
    }

    m_rootTreeItem = new CTreeItem(*this, nullptr, QString(), m_rootName, IconType::FolderIcon);
    m_model->invisibleRootItem()->appendRow(m_rootTreeItem);

    setHeaderHidden(true);

    for (auto& item: m_folders)
    {
        if (!item.isEmpty())
        {
            LoadTreeRec(item);
        }
    }

    if (m_bEnableMonitor)
    {
        CFileChangeMonitor::Instance()->Subscribe(this);
    }

    setSortingEnabled(true);
}

QString CFolderTreeCtrl::GetPath(QStandardItem* item) const
{
    CTreeItem* treeItem = static_cast<CTreeItem*>(item);

    if (treeItem)
    {
        return treeItem->GetPath();
    }

    return "";
}

bool CFolderTreeCtrl::IsFolder(QStandardItem* item) const
{
    return item->data(aznumeric_cast<int>(Roles::IsFolderRole)).toBool();
}

bool CFolderTreeCtrl::IsFile(QStandardItem* item) const
{
    return !IsFolder(item);
}

CFolderTreeCtrl::~CFolderTreeCtrl()
{
    // Unsubscribe from file change notifications
    if (m_bEnableMonitor)
    {
        CFileChangeMonitor::Instance()->Unsubscribe(this);
    }
}

void CFolderTreeCtrl::OnIndexDoubleClicked(const QModelIndex& index)
{
    QStandardItem* item = GetSourceItemByIndex(index);
    if (item)
    {
        Q_EMIT ItemDoubleClicked(item);
    }
}

void CFolderTreeCtrl::OnFileMonitorChange(const SFileChangeInfo& rChange)
{
    const QString filePath = Path::ToUnixPath(Path::GetRelativePath(rChange.filename));
    for (auto item = m_folders.begin(), end = m_folders.end(); item != end; ++item)
    {
        // Only look for changes in folder
        if (filePath.indexOf((*item)) != 0)
        {
            return;
        }

        if (rChange.changeType == rChange.eChangeType_Created || rChange.changeType == rChange.eChangeType_RenamedNewName)
        {
            if (CFileUtil::PathExists(filePath))
            {
                LoadTreeRec(filePath);
            }
            else
            {
                AddItem(filePath);
            }
        }
        else if (rChange.changeType == rChange.eChangeType_Deleted || rChange.changeType == rChange.eChangeType_RenamedOldName)
        {
            RemoveItem(filePath);
        }
    }
}

void CFolderTreeCtrl::contextMenuEvent(QContextMenuEvent* e)
{
    if (!m_model)
    {
        return;
    }

    auto index = indexAt(e->pos());
    QStandardItem* item = GetSourceItemByIndex(index);

    if (!item)
    {
        return;
    }

    const QString path = GetPath(item);

    QMenu menu;
    QAction* editAction = menu.addAction(tr("Edit"));
    connect(editAction, &QAction::triggered, this, [&]()
        {
            Edit(path);
        });
    QAction* showInExplorerAction = menu.addAction(tr("Show In Explorer"));
    connect(showInExplorerAction,&QAction::triggered,this,[&]()
        {
            ShowInExplorer(path);
        });
    menu.exec(QCursor::pos());
}

void CFolderTreeCtrl::LoadTreeRec(const QString& currentFolder)
{
    CFileEnum fileEnum;
    QFileInfo fileData;

    QString currentFolderSlash = Path::AddSlash(currentFolder);
    QString targetFolder = currentFolder;

    if (currentFolder.startsWith('@'))
    {
        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
        if (AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(currentFolder.toLocal8Bit().constData(), resolvedPath, AZ_MAX_PATH_LEN))
        {
            targetFolder = resolvedPath;
        }

        // update the base folder name
        QStringList parts = Path::SplitIntoSegments(currentFolderSlash);
        if (parts.size() > 1)
        {
            parts.removeFirst();
            currentFolderSlash = Path::AddSlash(parts.join(QDir::separator()));
        }
    }

    for (bool bFoundFile = fileEnum.StartEnumeration(targetFolder, "*", &fileData); bFoundFile;
         bFoundFile = fileEnum.GetNextFile(&fileData))
    {
        const QString fileName = fileData.fileName();

        // Have we found a folder?
        if (fileData.isDir())
        {
            // Skip the parent folder entries
            if (fileName == "." || fileName == "..")
            {
                continue;
            }

            LoadTreeRec(currentFolderSlash + fileName);
        }

        AddItem(currentFolderSlash + fileName);
    }
}

void CFolderTreeCtrl::AddItem(const QString& path)
{
    AZ::IO::FixedMaxPath folder{ AZ::IO::PathView(path.toUtf8().constData()) };
    AZ::IO::FixedMaxPath fileNameWithoutExtension = folder.Stem();
    folder = folder.ParentPath();

    auto regex = QRegExp(m_fileNameSpec, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (regex.exactMatch(path))
    {
        CTreeItem* folderTreeItem = CreateFolderItems(QString::fromUtf8(folder.c_str(), static_cast<int>(folder.Native().size())));
        if(folderTreeItem)
        {
            folderTreeItem->AddChild(
                QString::fromUtf8(fileNameWithoutExtension.c_str(), static_cast<int>(fileNameWithoutExtension.Native().size())),
                path,
                IconType::FileIcon);
        }
    }
}

void CFolderTreeCtrl::RemoveItem(const QString& path)
{
    if (!CFileUtil::FileExists(path))
    {
        auto findIter = m_pathToTreeItem.find(path);

        if (findIter != m_pathToTreeItem.end())
        {
            CTreeItem* foundItem = findIter->second;
            foundItem->Remove();
            RemoveEmptyFolderItems(Path::GetPath(path));
        }
    }
}

CFolderTreeCtrl::CTreeItem* CFolderTreeCtrl::GetItem(const QString& path)
{
    auto findIter = m_pathToTreeItem.find(path);

    if (findIter == m_pathToTreeItem.end())
    {
        return nullptr;
    }

    return findIter->second;
}

QStandardItem* CFolderTreeCtrl::GetSourceItemByIndex(const QModelIndex& index) const
{
    if (!m_proxyModel || !m_model)
    {
        return nullptr;
    }

    // Since our tree view has a proxy model to handle the sorting/filtering, any index
    // found on the tree view (e.g. the selected index) needs to be mapped back to the source
    // model to find the actual item.
    auto sourceIndex = m_proxyModel->mapToSource(index);
    return m_model->itemFromIndex(sourceIndex);
}

QString CFolderTreeCtrl::CalculateFolderFullPath(const QStringList& splittedFolder, int idx)
{
    QString path;
    for (int segIdx = 0; segIdx <= idx; ++segIdx)
    {
        if (segIdx != 0)
        {
            path.append(QLatin1Char('/'));
        }
        path.append(splittedFolder[segIdx]);
    }
    return path;
}

CFolderTreeCtrl::CTreeItem* CFolderTreeCtrl::CreateFolderItems(const QString& folder)
{
    if(!m_rootTreeItem) 
    {
        return nullptr;
    }

    QStringList splittedFolder = Path::SplitIntoSegments(folder);
    CTreeItem* currentTreeItem = m_rootTreeItem;

    if (!m_bFlatStyle)
    {
        QString currentFolder;
        QString fullpath;
        const int splittedFoldersCount = splittedFolder.size();
        for (int idx = 0; idx < splittedFoldersCount; ++idx)
        {
            currentFolder = Path::RemoveBackslash(splittedFolder[idx]);
            fullpath = CalculateFolderFullPath(splittedFolder, idx);

            CTreeItem* folderItem = GetItem(fullpath);
            if (!folderItem)
            {
                currentTreeItem = currentTreeItem->AddChild(currentFolder, fullpath, IconType::FolderIcon);
            }
            else
            {
                currentTreeItem = folderItem;
            }
        }
    }

    return currentTreeItem;
}

void CFolderTreeCtrl::RemoveEmptyFolderItems(const QString& folder)
{
    QStringList splittedFolder = Path::SplitIntoSegments(folder);
    const int splittedFoldersCount = splittedFolder.size();
    QString fullpath;
    for (int idx = 0; idx < splittedFoldersCount; ++idx)
    {
        fullpath = CalculateFolderFullPath(splittedFolder, idx);
        CTreeItem* folderItem = GetItem(fullpath);
        if (!folderItem)
        {
            continue;
        }

        if (!folderItem->hasChildren())
        {
            folderItem->Remove();
        }
    }
}

void CFolderTreeCtrl::Edit(const QString& path)
{
    CFileUtil::EditTextFile(path.toUtf8().data(), 0, IFileUtil::FILE_TYPE_SCRIPT);
}

void CFolderTreeCtrl::ShowInExplorer(const QString& path)
{
    if (QFileInfo(path).isAbsolute())
    {
        AzQtComponents::ShowFileOnDesktop(path);
    }
    else
    {
        QString absolutePath = QDir::currentPath();

        CTreeItem* item = GetItem(path);
        if (item != m_rootTreeItem)
        {
            absolutePath += QStringLiteral("/%1").arg(path);
        }

        AzQtComponents::ShowFileOnDesktop(absolutePath);
    }
}

QIcon CFolderTreeCtrl::GetItemIcon(IconType image) const
{
    return image == IconType::FolderIcon ? m_folderIcon : m_fileIcon;
}

QList<QStandardItem*> CFolderTreeCtrl::GetSelectedItems() const
{
    QList<QStandardItem*> items;

    for (auto index : selectedIndexes())
    {
        QStandardItem* item = GetSourceItemByIndex(index);
        if (item)
        {
            items.append(item);
        }
    }

    return items;
}

void CFolderTreeCtrl::SetSearchFilter(const QString& searchText)
{
    if (m_proxyModel)
    {
        m_proxyModel->setFilterFixedString(searchText);
    }
}

//////////////////////////////////////////////////////////////////////////
// CFolderTreeCtrl::CTreeItem
//////////////////////////////////////////////////////////////////////////

CFolderTreeCtrl::CTreeItem::CTreeItem(
    CFolderTreeCtrl& folderTreeCtrl, CFolderTreeCtrl::CTreeItem* parent, const QString& name, const QString& path, IconType icon)
    : QStandardItem(folderTreeCtrl.GetItemIcon(icon), name)
    , m_folderTreeCtrl(folderTreeCtrl)
    , m_path(path)
{
    if (parent)
    {
        parent->appendRow(this);
    }
    setData(icon == IconType::FolderIcon, aznumeric_cast<int>(Roles::IsFolderRole));

    m_folderTreeCtrl.m_pathToTreeItem[m_path] = this;
}

CFolderTreeCtrl::CTreeItem::~CTreeItem()
{
    m_folderTreeCtrl.m_pathToTreeItem.erase(m_path);
}

void CFolderTreeCtrl::CTreeItem::Remove()
{
    // Root can't be deleted this way
    if (auto parentItem = parent())
    {
        int numRows = parentItem->rowCount();
        for (int i = 0; i < numRows; ++i)
        {
            if (parentItem->child(i) == this)
            {
                parentItem->removeRow(i);
                break;
            }
        }
    }
}

CFolderTreeCtrl::CTreeItem* CFolderTreeCtrl::CTreeItem::AddChild(const QString& name, const QString& path, IconType icon)
{
    return new CTreeItem(m_folderTreeCtrl, this, name, path, icon);
}
