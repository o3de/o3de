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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_CONTROLS_FOLDERTREECTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_FOLDERTREECTRL_H
#pragma once

#include "Util/FileChangeMonitor.h"
#include <QList>
#include <QStandardItem>
#include <QTreeView>
#include <QIcon>

class QSortFilterProxyModel;
class QStandardItemModel;

//! Case insensetive less key for any type convertable to const char*.
struct qstring_icmp
{
    bool operator()(const QString& left, const QString& right) const
    {
        return QString::compare(left, right, Qt::CaseInsensitive) < 0;
    }
};

class CFolderTreeCtrl
    : public QTreeView
    , public CFileChangeMonitorListener
{
    Q_OBJECT // AUTOMOC

    friend class CTreeItem;

    class CTreeItem
        : public QStandardItem
    {
        // Only allow destruction through std::unique_ptr
        friend struct std::default_delete<CTreeItem>;

    public:
        explicit CTreeItem(CFolderTreeCtrl& folderTreeCtrl, const QString& path);
        explicit CTreeItem(CFolderTreeCtrl& folderTreeCtrl, CTreeItem* parent,
            const QString& name, const QString& path, const int image);

        void Remove();
        CTreeItem* AddChild(const QString& name, const QString& path, const int image);
        QString GetPath() const { return m_path; }
    private:
        ~CTreeItem();

        CFolderTreeCtrl& m_folderTreeCtrl;
        QString m_path;
    };

public:
    CFolderTreeCtrl(QWidget* parent = 0);
    CFolderTreeCtrl(const QStringList& folders, const QString& fileNameSpec,
        const QString& rootName, bool bDisableMonitor = false, bool bFlatTree = true, QWidget* parent = 0);
    virtual ~CFolderTreeCtrl();

    void init(const QStringList& folders, const QString& fileNameSpec,
        const QString& rootName, bool bDisableMonitor = false, bool bFlatTree = true);

    QString GetPath(QStandardItem* item) const;
    bool IsFolder(QStandardItem* item) const;
    bool IsFile(QStandardItem* item) const;

    QIcon GetItemIcon(int image) const;
    QList<QStandardItem*> GetSelectedItems() const;

    void SetSearchFilter(const QString& searchText);

Q_SIGNALS:
    void ItemDoubleClicked(QStandardItem* item);

protected Q_SLOTS:
    void OnIndexDoubleClicked(const QModelIndex& index);

protected:
    virtual void OnFileMonitorChange(const SFileChangeInfo& rChange);
    void contextMenuEvent(QContextMenuEvent* e) override;

    void InitTree();
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

    bool m_bDisableMonitor;
    bool m_bFlatStyle;
    std::unique_ptr< CTreeItem > m_rootTreeItem;
    QString m_fileNameSpec;
    QStringList m_folders;
    QString m_rootName;
    std::map<QString, unsigned int> m_foldersSegments;
    QIcon m_folderIcon;
    QIcon m_fileIcon;

    QSortFilterProxyModel* m_proxyModel = nullptr;
    QStandardItemModel* m_model = nullptr;
    std::map< QString, CTreeItem*, qstring_icmp > m_pathToTreeItem;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_FOLDERTREECTRL_H
