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

#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   DatabaseFrameWnd.h
//  Created:     10/Dec/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_DATABASEFRAMEWND_H
#define CRYINCLUDE_EDITOR_DATABASEFRAMEWND_H

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include "Undo/IUndoManagerListener.h"
#include "BaseLibrary.h"

#include <QMainWindow>
#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QScopedPointer>
#endif

class QComboBox;
class QTreeView;
class QMimeData;

class CBaseLibraryItem;
class CBaseLibraryManager;

class LibraryListModel;
class LibraryItemTreeModel;

namespace Ui {
    class DatabaseFrameWnd;
}

class CDatabaseFrameWnd
    : public AzQtComponents::DockMainWindow
    , public IEditorNotifyListener
    , public IUndoManagerListener
{
    Q_OBJECT
public:
    CDatabaseFrameWnd(CBaseLibraryManager* pItemManager, QWidget* pParent = nullptr);
    virtual ~CDatabaseFrameWnd();

    enum SortRecursionType
    {
        SORT_RECURSION_NONE = 1,
        SORT_RECURSION_ITEM = 2,
        SORT_RECURSION_FULL = 9999
    };

    virtual void ReloadLibs();
    virtual void ReloadItems();
    virtual void SelectLibrary(const QString& library, bool bForceSelect = false);
    virtual void SelectLibrary(CBaseLibrary* pItem, bool bForceSelect = false);
    virtual void SelectItem(CBaseLibraryItem* item, bool bForceReload = false);

    virtual CBaseLibrary* FindLibrary(const QString& libraryName);
    virtual CBaseLibrary* NewLibrary(const QString& libraryName);
    virtual void DeleteLibrary(CBaseLibrary* pLibrary);
    virtual void DeleteItem(CBaseLibraryItem* pItem);

    virtual void ReleasePreviewControl(){}

    virtual bool SetItemName(CBaseLibraryItem* item, const QString& groupName, const QString& itemName);

    void DoesItemExist(const QString& itemName, bool& bOutExist) const;
    void DoesGroupExist(const QString& groupName, bool& bOutExist) const;

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo) override;

    QString GetSelectedLibraryName() const;

    virtual const char* GetClassName() = 0;

protected:
    int GetComboBoxIndex(CBaseLibrary* pLibrary);

    virtual void OnInitDialog() = 0;

    void showEvent(QShowEvent* event) override;

    void OnUndo();
    void OnRedo();

    virtual void OnAddLibrary();
    virtual void OnRemoveLibrary();
    virtual void OnAddItem();
    virtual void OnRemoveItem();
    virtual void OnRenameItem();
    virtual void OnChangedLibrary();
    virtual void OnExportLibrary();
    virtual void OnSave();
    virtual void OnReloadLib();
    virtual void OnLoadLibrary();

    void OnSelChangedItemTree(const QModelIndex& index);
    bool eventFilter(QObject* watched, QEvent* event);

    virtual void OnCopy() = 0;
    virtual void OnPaste() = 0;
    virtual void OnCut();
    virtual void OnClone();

    void InitTreeCtrl();

    virtual AssetSelectionModel GetAssetSelectionModel() const = 0;

    void LoadLibrary();

    QString MakeValidName(const QString& candidateName, Functor2<const QString&, bool&> cb) const;

    virtual QTreeView* GetTreeCtrl() = 0;
    virtual const QTreeView* GetTreeCtrl() const = 0;

private:
    LibraryListModel* m_pLibraryListModel;
    QComboBox* m_pLibraryListComboBox;

    bool m_bLibsLoaded;

protected:
    LibraryItemTreeModel* m_pLibraryItemTreeModel;

    //! Selected library.
    _smart_ptr<CBaseLibrary> m_pLibrary;

    //! Last selected Item. (kept here for compatibility reasons)
    // See comments on m_cpoSelectedLibraryItems for more details.
    _smart_ptr<CBaseLibraryItem> m_pCurrentItem;

    // A set containing all the currently selected items
    // (it's disabled for MOST, but not ALL cases).
    // This should be the new standard way of storing selections as
    // opposed to the former mean, it allows us to store multiple selections.
    // The migration to this new style should be done according to the needs
    // for multiple selection.
    std::set<CBaseLibraryItem*> m_cpoSelectedLibraryItems;

    //! Pointer to item manager.
    CBaseLibraryManager* m_pItemManager;
    SortRecursionType m_sortRecursionType;
    QString m_selectedGroup;

    QScopedPointer<Ui::DatabaseFrameWnd> ui;

    bool m_initialized;
};

class LibraryListModel
    : public QAbstractListModel
{
    Q_OBJECT

public:
    LibraryListModel(CBaseLibraryManager* itemManager, QObject* pParent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void Reload();

    void clear();

private:
    void LibraryModified(bool bModified);

    CBaseLibraryManager* m_pItemManager;
};

class LibraryItemTreeModel
    : public QAbstractItemModel
{
    Q_OBJECT

    using Group = std::pair<QString, std::vector<CBaseLibraryItem*> >;

public:
    LibraryItemTreeModel(CDatabaseFrameWnd* pParent);

    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex index(CBaseLibraryItem* pItem) const;

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

    void Clear();

    void Reload(CBaseLibrary* library);

    void Add(CBaseLibraryItem* item);
    bool Remove(CBaseLibraryItem* item);

    void Rename(CBaseLibraryItem* item, const QString& groupName, const QString& shortName);

    std::vector<CBaseLibraryItem*> ChildItems(const QModelIndex& index) const;

    QString GetFullName(const QModelIndex& index) const;

    QModelIndex FindLibraryItemByFullName(const QString& fullName) const;
    bool DoesGroupExist(const QString& groupName) const;

signals:
    void itemRenamed(CBaseLibraryItem* item, const QString& prevFullName);

protected:
    void RenameItem(CBaseLibraryItem* item, const QString& fullName);
    QString MakeValidName(const Group& group, const QString& baseName) const;
    bool MoveItem(CBaseLibraryItem* item, const QModelIndex& parent);

    CDatabaseFrameWnd* m_dialog;

    std::map<QString, std::shared_ptr<Group> > m_groups;
};

Q_DECLARE_METATYPE(CBaseLibrary*)

#endif // CRYINCLUDE_EDITOR_DATABASEFRAMEWND_H
