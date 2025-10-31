/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Util/FileChangeMonitor.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>

#include <QList>
#include <QStandardItem>
#include <QTreeView>
#include <QIcon>

class QSortFilterProxyModel;
class QStandardItemModel;

class CFolderTreeCtrl
    : public QTreeView
    , public CFileChangeMonitorListener
{
    Q_OBJECT 

    friend class CTreeItem;

    enum class IconType
    {
        FolderIcon = 0,
        FileIcon = 2
    };

    enum class Roles : int
    {
        IsFolderRole = Qt::UserRole
    };

    class CTreeItem
        : public QStandardItem
    {
    public:
        CTreeItem(CFolderTreeCtrl& folderTreeCtrl, CTreeItem* parent,
            const QString& name, const QString& path, IconType image);
        ~CTreeItem();

        void Remove();
        CTreeItem* AddChild(const QString& name, const QString& path, IconType image);
        QString GetPath() const { return m_path; }
    private:

        CFolderTreeCtrl& m_folderTreeCtrl;
        QString m_path;
    };

public:
    CFolderTreeCtrl(QWidget* parent = 0);
    virtual ~CFolderTreeCtrl();

    void Configure(const AZStd::vector<QString>& folders, const QString& fileNameSpec,
        const QString& rootName, bool bEnabledMonitor = true, bool bFlatTree = true);

    QString GetPath(QStandardItem* item) const;
    bool IsFolder(QStandardItem* item) const;
    bool IsFile(QStandardItem* item) const;

    QIcon GetItemIcon(IconType image) const;
    QList<QStandardItem*> GetSelectedItems() const;

    void SetSearchFilter(const QString& searchText);

Q_SIGNALS:
    void ItemDoubleClicked(QStandardItem* item);

protected Q_SLOTS:
    void OnIndexDoubleClicked(const QModelIndex& index);

protected:
    struct CaseInsensitiveCompare
    {
        bool operator()(const QString& left, const QString& right) const
        {
            return QString::compare(left, right, Qt::CaseInsensitive) < 0;
        }
    };


    void OnFileMonitorChange(const SFileChangeInfo& rChange) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

    void LoadTreeRec(const QString& currentFolder);

    void AddItem(const QString& path);
    void RemoveItem(const QString& path);
    CTreeItem* GetItem(const QString& path);

    QStandardItem* GetSourceItemByIndex(const QModelIndex& index) const;

    QString CalculateFolderFullPath(const QStringList& splittedFolder, int idx);
    CTreeItem* CreateFolderItems(const QString& folder);
    void RemoveEmptyFolderItems(const QString& folder);

    void Edit(const QString& path);
    void ShowInExplorer(const QString& path);

    bool m_bEnableMonitor = false;
    bool m_bFlatStyle = false;
    QString m_fileNameSpec = "";
    AZStd::vector<QString> m_folders = {};
    QString m_rootName = "";
    AZStd::map< QString, CTreeItem*, CaseInsensitiveCompare > m_pathToTreeItem = {};
    CTreeItem* m_rootTreeItem = nullptr;

    QIcon m_folderIcon;
    QIcon m_fileIcon;
    QStandardItemModel* m_model;
    QSortFilterProxyModel* m_proxyModel;

};
