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

// AzQtComponents
#include <AzQtComponents/Utilities/DesktopUtilities.h>      // for AzQtComponents::ShowFileOnDesktop


enum ETreeImage
{
    eTreeImage_Folder = 0,
    eTreeImage_File = 2
};

enum CustomRoles
{
    IsFolderRole = Qt::UserRole
};

//////////////////////////////////////////////////////////////////////////
// CFolderTreeCtrl
//////////////////////////////////////////////////////////////////////////
CFolderTreeCtrl::CFolderTreeCtrl(const QStringList& folders, const QString& fileNameSpec,
    const QString& rootName, bool bDisableMonitor, bool bFlatTree, QWidget* parent)
    : QTreeView(parent)
    , m_rootTreeItem(nullptr)
    , m_folders(folders)
    , m_fileNameSpec(fileNameSpec)
    , m_rootName(rootName)
    , m_bDisableMonitor(bDisableMonitor)
    , m_bFlatStyle(bFlatTree)
{
    init(folders, fileNameSpec, rootName, bDisableMonitor, bFlatTree);
}

CFolderTreeCtrl::CFolderTreeCtrl(QWidget* parent)
    : QTreeView(parent)
    , m_rootTreeItem(nullptr)
    , m_bDisableMonitor(false)
    , m_bFlatStyle(true)

{
}


void CFolderTreeCtrl::init(const QStringList& folders, const QString& fileNameSpec, const QString& rootName, bool bDisableMonitor /*= false*/, bool bFlatTree /*= true*/)
{
    m_model = new QStandardItemModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setRecursiveFilteringEnabled(true);

    m_proxyModel->setSourceModel(m_model);
    setModel(m_proxyModel);

    m_folders = folders;
    m_fileNameSpec = fileNameSpec;
    m_rootName = rootName;
    m_bDisableMonitor = bDisableMonitor;
    m_bFlatStyle = bFlatTree;
    m_fileIcon = QIcon(":/TreeView/default-icon.svg");
    m_folderIcon = QIcon(":/TreeView/folder-icon.svg");

    for (auto item = m_folders.begin(), end = m_folders.end(); item != end; ++item)
    {
        (*item) = Path::RemoveBackslash(Path::ToUnixPath((*item)));

        if (CFileUtil::PathExists(*item))
        {
            m_foldersSegments.insert(std::make_pair((*item), Path::SplitIntoSegments((*item)).size()));
        }
        else if (Path::IsFolder((*item).toLocal8Bit().constData()))
        {
            m_foldersSegments.insert(std::make_pair((*item), Path::SplitIntoSegments((*item)).size()));
        }
        else
        {
            (*item).clear();
        }
    }

    setHeaderHidden(true);

    QObject::connect(this, &QTreeView::doubleClicked, this, &CFolderTreeCtrl::OnIndexDoubleClicked);

    InitTree();

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
    return item->data(IsFolderRole).toBool();
}

bool CFolderTreeCtrl::IsFile(QStandardItem* item) const
{
    return !IsFolder(item);
}

CFolderTreeCtrl::~CFolderTreeCtrl()
{
    // Obliterate tree items before destroying the controls
    m_rootTreeItem.reset(nullptr);

    // Unsubscribe from file change notifications
    if (!m_bDisableMonitor)
    {
        CFileChangeMonitor::Instance()->Unsubscribe(this);
    }
}

void CFolderTreeCtrl::OnIndexDoubleClicked(const QModelIndex& index)
{
    if (!m_proxyModel || !m_model)
    {
        return;
    }

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
    connect(editAction, &QAction::triggered, this, [=]()
        {
            this->Edit(path);
        });
    QAction* showInExplorerAction = menu.addAction(tr("Show In Explorer"));
    connect(showInExplorerAction, &QAction::triggered, this, [=]()
        {
            this->ShowInExplorer(path);
        });
    menu.exec(QCursor::pos());
}

void CFolderTreeCtrl::InitTree()
{
    m_rootTreeItem.reset(new CTreeItem(*this, m_rootName));

    for (auto item = m_folders.begin(), end = m_folders.end(); item != end; ++item)
    {
        if (!(*item).isEmpty())
        {
            LoadTreeRec((*item));
        }
    }

    if (!m_bDisableMonitor)
    {
        CFileChangeMonitor::Instance()->Subscribe(this);
    }


    expandAll();
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
        QStringList parts =  Path::SplitIntoSegments(currentFolderSlash);
        if (parts.size() > 1)
        {
            parts.removeFirst();
            currentFolderSlash = Path::AddSlash(parts.join(QDir::separator()));
        }
    }

    for (bool bFoundFile = fileEnum.StartEnumeration(targetFolder, "*", &fileData);
         bFoundFile; bFoundFile = fileEnum.GetNextFile(&fileData))
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
        folderTreeItem->AddChild(QString::fromUtf8(fileNameWithoutExtension.c_str(),
            static_cast<int>(fileNameWithoutExtension.Native().size())), path, eTreeImage_File);
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
    QStringList splittedFolder = Path::SplitIntoSegments(folder);
    CTreeItem* currentTreeItem = m_rootTreeItem.get();

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
                currentTreeItem = currentTreeItem->AddChild(currentFolder, fullpath, eTreeImage_Folder);
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
    QString absolutePath = QDir::currentPath();

    CTreeItem* root = m_rootTreeItem.get();
    CTreeItem* item = GetItem(path);

    if (item != root)
    {
        absolutePath += QStringLiteral("/%1").arg(path);
    }

    AzQtComponents::ShowFileOnDesktop(absolutePath);
}

QIcon CFolderTreeCtrl::GetItemIcon(int image) const
{
    return image == eTreeImage_File ? m_fileIcon : m_folderIcon;
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
CFolderTreeCtrl::CTreeItem::CTreeItem(CFolderTreeCtrl& folderTreeCtrl, const QString& path)
    : QStandardItem(folderTreeCtrl.GetItemIcon(eTreeImage_Folder), folderTreeCtrl.m_rootName)
    , m_folderTreeCtrl(folderTreeCtrl)
    , m_path(path)
{
    setData(true, IsFolderRole);

    m_folderTreeCtrl.m_model->invisibleRootItem()->appendRow(this);
    m_folderTreeCtrl.m_pathToTreeItem[ m_path ] = this;
}

CFolderTreeCtrl::CTreeItem::CTreeItem(CFolderTreeCtrl& folderTreeCtrl, CFolderTreeCtrl::CTreeItem* parent,
    const QString& name, const QString& path, const int image)
    : QStandardItem(folderTreeCtrl.GetItemIcon(image), name)
    , m_folderTreeCtrl(folderTreeCtrl)
    , m_path(path)
{
    parent->appendRow(this);
    setData(image == eTreeImage_Folder, IsFolderRole);

    m_folderTreeCtrl.m_pathToTreeItem[ m_path ] = this;
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

CFolderTreeCtrl::CTreeItem* CFolderTreeCtrl::CTreeItem::AddChild(const QString& name, const QString& path, const int image)
{
    CTreeItem* newItem = new CTreeItem(m_folderTreeCtrl, this, name, path, image);
    return newItem;
}
