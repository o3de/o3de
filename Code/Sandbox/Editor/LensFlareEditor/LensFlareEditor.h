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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREEDITOR_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREEDITOR_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "DatabaseFrameWnd.h"
#include "LensFlareUtil.h"
#include "ILensFlareListener.h"
#include "LensFlareItemTree.h"
#include "UserMessageDefines.h"
#endif

#include <AzQtComponents/Components/DockMainWindow.h>

namespace AzQtComponents
{
    class FancyDocking;
}

class CLensFlareView;
class CLensFlareElementTree;
class CLensFlareAtomicList;
class CLensFlareElementPropertyView;
class CLensFlareItem;
class CLensFlareLibrary;
class ReflectedPropertyControl;
class CLensFlareLightEntityTree;
class CLensFlareReferenceTree;

class CLensFlareEditor
    : public CDatabaseFrameWnd
    , public ILensFlareChangeElementListener
{
    Q_OBJECT

public:
    static const GUID& GetClassID();
    static void RegisterViewClass();

    CLensFlareEditor(QWidget* pParent = nullptr);
    ~CLensFlareEditor();

    QMenu* createPopupMenu() override;

    // CDatabaseFrameWnd overrides...
    virtual void SelectItem(CBaseLibraryItem* item, bool bForceReload = false) override;

    CLensFlareItem* GetSelectedLensFlareItem() const;
    bool GetSelectedLensFlareName(QString& outName) const;
    void UpdateLensFlareItem(CLensFlareItem* pLensFlareItem);
    void ResetElementTreeControl();

    CLensFlareLibrary* GetCurrentLibrary() const
    {
        if (m_pLibrary == NULL)
        {
            return NULL;
        }
        return (CLensFlareLibrary*)&(*m_pLibrary);
    }

    bool IsExistTreeItem(const QString& name, bool bExclusiveSelectedItem = false);
    void RenameLensFlareItem(CLensFlareItem* pLensFlareItem, const QString& newGroupName, const QString& newShortName);

    IOpticsElementBasePtr FindOptics(const QString& itemPath, const QString& opticsPath);

    CLensFlareElementTree* GetLensFlareElementTree()
    {
        return m_pLensFlareElementTree;
    }

    CLensFlareView* GetLensFlareView() const
    {
        return m_pLensFlareView;
    }

    CLensFlareItemTree* GetLensFlareItemTree()
    {
        return m_LensFlareItemTree;
    }

    void RemovePropertyItems();

    ReflectedPropertyControl* GetPropertyCtrl()
    {
        return m_pWndProps;
    }

    void UpdateLensOpticsNames(const QString& oldFullName, const QString& newFullName);
    void SelectItemInLensFlareElementTreeByName(const QString& name);
    void ReloadItems();

    void RegisterLensFlareItemChangeListener(ILensFlareChangeItemListener* pListener);
    void UnregisterLensFlareItemChangeListener(ILensFlareChangeItemListener* pListener);

    bool SelectItemByName(const QString& itemName);

    static CLensFlareEditor* GetLensFlareEditor()
    {
        return s_pLensFlareEditor;
    }

    bool GetFullSelectedFlareItemName(QString& outFullName) const
    {
        QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
        if (selected.isEmpty())
        {
            return false;
        }
        return GetFullLensFlareItemName(selected.first(), outFullName);
    }

    void SelectLensFlareItem(const QString& fullItemName);

    const char* GetClassName()
    {
        return s_pLensFlareEditorClassName;
    }

    void Paste(XmlNodeRef node);
    void Paste(const QModelIndex& index, XmlNodeRef node);
    XmlNodeRef CreateXML(const char* type) const;

    static const char* s_pLensFlareEditorClassName;

    QTreeView* GetTreeCtrl() override
    {
        return m_LensFlareItemTree;
    }

    const QTreeView* GetTreeCtrl() const override
    {
        return m_LensFlareItemTree;
    }

    void AddNewItemByAtomicOptics(const QModelIndex& hSelectedItem, EFlareType flareType);

public slots:
    void OnUpdateTreeCtrl();

protected:
    static CLensFlareEditor* s_pLensFlareEditor;

    void OnInitDialog() override;

    void OnCopy();
    void OnPaste();
    void OnCut();

    void UpdateClipboard(const char* type) const;
    bool GetClipboardDataList(std::vector<LensFlareUtil::SClipboardData>& outList, QString& outGroupName) const;
    void OnLensFlareChangeElement(CLensFlareElement* pLensFlareElement);

    void OnAddLibrary();
    void OnAssignFlareToLightEntities();
    void OnSelectAssignedObjects();
    void OnGetFlareFromSelection();
    void OnRenameItem();
    void OnAddItem();
    void OnRemoveItem();
    void OnCopyNameToClipboard();
    void OnNotifyTreeRClick();

    void OnTvnItemSelChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void OnReloadLib();
    void ReleaseWindowsToBePutIntoPanels();
    void SelectLensFlareItem(const QModelIndex& hItem);
    void SelectLensFlareItem(const QModelIndex& hItem, const QModelIndex& hPrevItem);

    void StartEditItem(const QModelIndex& hItem);
    bool GetFullLensFlareItemName(const QModelIndex& hItem, QString& outFullName) const;
    AssetSelectionModel GetAssetSelectionModel() const override;

    CLensFlareItem* AddNewLensFlareItem(const QString& groupName, const QString& shortName);
    QModelIndex GetTreeLensFlareItem(CLensFlareItem* pItem) const;

    void OnItemTreeDataRenamed(CBaseLibraryItem* pItem, const QString& prevFullName);

    enum ESelectedItemStatus
    {
        eSIS_Unselected,
        eSIS_Group,
        eSIS_Flare
    };
    ESelectedItemStatus GetSelectedItemStatus() const;

    void addDockWidget(Qt::DockWidgetArea area, QWidget* widget, const QString& title, bool closable = true);
    void OnUpdateProperties(IVariable* var);

private:

    CLensFlareView* m_pLensFlareView;
    CLensFlareAtomicList* m_pLensFlareAtomicList;
    CLensFlareElementTree* m_pLensFlareElementTree;
    ReflectedPropertyControl* m_pWndProps;
    CLensFlareLightEntityTree* m_pLensFlareLightEntityTree;
    CLensFlareReferenceTree* m_pLensFlareReferenceTree;
    CLensFlareItemTree* m_LensFlareItemTree;

    std::vector<ILensFlareChangeItemListener*> m_LensFlareChangeItemListenerList;
    AzQtComponents::FancyDocking* m_advancedDockManager = nullptr;
};

class LensFlareItemTreeModel
    : public LibraryItemTreeModel
{
    Q_OBJECT

public:
    LensFlareItemTreeModel(CDatabaseFrameWnd* pParent);

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;
};

#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREEDITOR_H
