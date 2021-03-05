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

#ifndef CRYINCLUDE_EDITOR_MATERIAL_MATERIALBROWSER_H
#define CRYINCLUDE_EDITOR_MATERIAL_MATERIALBROWSER_H
#pragma once


#if !defined(Q_MOC_RUN)
#include "Include/IDataBaseManager.h"
#include "Material/Material.h"
#include "Material/MaterialBrowserFilterModel.h"

#include <QMenu>
#include <QDateTime>
#include <QModelIndex>
#include <ISourceControl.h>

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#endif

class CMaterial;

class MaterialBrowserWidget;
class CMaterialImageListCtrl;
class QMaterialImageListModel;

class QTreeView;
class QAction;

struct IDataBaseItem;
class CMaterialBrowserRecord;
typedef std::vector<CMaterialBrowserRecord> TMaterialBrowserRecords;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserModel;
        class AssetBrowserFilterModel;
        class AssetTypeFilter;
    }
}

enum ESccFileAttributes;

namespace Ui
{
    class MaterialBrowser;
}


//////////////////////////////////////////////////////////////////////////
struct IMaterialBrowserListener
{
    virtual void OnBrowserSelectItem(IDataBaseItem* pItem, bool bForce) = 0;
};

//////////////////////////////////////////////////////////////////////////
// MaterialBrowserWidget
//////////////////////////////////////////////////////////////////////////
class MaterialBrowserFilterModel;

class MaterialBrowserWidget
    : public QWidget
    , public IDataBaseManagerListener
    , public IEditorNotifyListener
    , public MaterialBrowserWidgetBus::Handler
{
    Q_OBJECT

public:
    enum EViewType
    {
        VIEW_LEVEL = 0,
        VIEW_ALL = 1,
    };

    enum EFilter
    {
        eFilter_Materials = 0x01,
        eFilter_Textures = 0x02,
        eFilter_Materials_And_Textures = 0x03,
        eFilter_Submaterials = 0x04
    };

    MaterialBrowserWidget(QWidget* parent);
    ~MaterialBrowserWidget();

    void SetListener(IMaterialBrowserListener* pListener) { m_pListener = pListener; }

    EViewType GetViewType() const { return m_viewType; };

    void ClearItems();

    void SelectItem(IDataBaseItem* pItem, IDataBaseItem* pParentItem);
    void DeleteItem();

    void PopulateItems();
    void StartRecordUpdateJobs();

    bool ShowCheckedOutRecursive(TMaterialBrowserRecords* pRecords);

    void ShowOnlyLevelMaterials(bool levelOnly);
    
    void OnCopy();
    void OnCopyName();
    void OnPaste();
    void OnCut();
    void OnDuplicate();
    void OnAddNewMaterial();
    void OnAddNewMultiMaterial();
    void OnConvertToMulti();
    void OnMergeMaterials();

public slots:
    void OnSelectionChanged();
    void OnSubMaterialSelectedInPreviewPane(const QModelIndex& current);
    void SaveCurrentMaterial();
    void OnRefreshSelection();
    void OnMaterialAdded();

signals:
    void refreshSelection();
    void materialAdded();
    
public:
    void OnUpdateShowCheckedOut();
    bool CanPaste() const;

    void SetImageListCtrl(CMaterialImageListCtrl* pCtrl);

    //////////////////////////////////////////////////////////////////////////
    // IDataBaseManagerListener implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);
    //////////////////////////////////////////////////////////////////////////

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    void AddContextMenuActionsSingleSelection(QMenu &menu, _smart_ptr<CMaterial> material) const;
    void OnContextMenuAction(int command, _smart_ptr<CMaterial> material);

    // MaterialBrowserWidgetBus event handlers
    void MaterialAddFinished() override;
    void MaterialFinishedProcessing(_smart_ptr<CMaterial> material, const QPersistentModelIndex &filterModelIndex) override;
    void MaterialRecordUpdateFinished() override;

protected:
    // Item definition.
    enum ESourceControlOp
    {
        ESCM_IMPORT,
        ESCM_CHECKOUT,
        ESCM_UNDO_CHECKOUT,
        ESCM_GETLATEST,
        ESCM_GETLATESTTEXTURES,
    };

    void DeleteItem(const CMaterialBrowserRecord &record);
    void SetSelectedItem(_smart_ptr<CMaterial> material, const TMaterialBrowserRecords* pMarkedRecords, bool selectInTreeView);
    void OnAddSubMtl();
    void OnSelectAssignedObjects();
    void OnAssignMaterialToSelection();
    void OnRenameItem();
    void OnResetItem();
    void OnSetSubMtlCount(const CMaterialBrowserRecord &record);
    
    void DoSourceControlOp(CMaterialBrowserRecord &record, ESourceControlOp);

    void OnMakeSubMtlSlot(const CMaterialBrowserRecord &record);
    void OnClearSubMtlSlot(_smart_ptr<CMaterial> subMaterial);
    void SetSubMaterial(_smart_ptr<CMaterial> parentMaterial, int slot, _smart_ptr<CMaterial> subMaterial);

    void OnSaveToFile(bool bMulti);

    void RefreshSelected();

    void TickRefreshMaterials();
    void TryLoadRecordMaterial(CMaterialBrowserRecord &record);

    void ShowContextMenu(const CMaterialBrowserRecord &record, const QPoint& point);
    _smart_ptr<CMaterial> GetCurrentMaterial();

    uint32 MaterialNameToCrc32(const QString& str);

    bool TryGetSelectedRecord(CMaterialBrowserRecord &record);
    AZStd::string GetSelectedMaterialID();

private:
    void expandAllNotMatchingIndexes(const QModelIndex& parent = QModelIndex());
    void ClearImageListControlSelection();
    void ClearSelection(QMaterialImageListModel* materialModel);
    QMenu *InitializeSearchMenu();

    void AddContextMenuActionsMultiSelect(QMenu &menu) const;
    void AddContextMenuActionsNoSelection(QMenu &menu) const;
    void AddContextMenuActionsSubMaterial(QMenu &menu, _smart_ptr<CMaterial> parentMaterial, _smart_ptr<CMaterial> subMaterial) const;
    void AddContextMenuActionsMultiMaterial(QMenu &menu) const;
    void AddContextMenuActionsSingleMaterial(QMenu &menu) const;
    void AddContextMenuActionsCommon(QMenu &menu, _smart_ptr<CMaterial> material) const;
    void AddContextMenuActionsSourceControl(QMenu &menu, _smart_ptr<CMaterial> material, uint32 fileAttributes) const;

    QScopedPointer<Ui::MaterialBrowser> m_ui;

    AzToolsFramework::AssetBrowser::AssetBrowserModel* m_assetBrowserModel;
    QSharedPointer<MaterialBrowserFilterModel> m_filterModel;
    int m_selectedSubMaterialIndex = -1;
    
    bool m_bIgnoreSelectionChange;
    bool m_bItemsValid;

    CMaterialManager* m_pMatMan;
    IMaterialBrowserListener* m_pListener;
    CMaterialImageListCtrl* m_pMaterialImageListCtrl;

    EViewType m_viewType;
    bool m_bNeedReload;

    bool m_bHighlightMaterial;
    uint32 m_timeOfHighlight;
    
    TMaterialBrowserRecords m_markedRecords;

    _smart_ptr<CMaterial> m_pLastActiveMultiMaterial;
    _smart_ptr<CMaterial> m_delayedSelection;

    bool m_bShowOnlyCheckedOut;

    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_duplicateAction;
    QAction* m_deleteAction;
    QAction* m_renameItemAction;
    QAction* m_addNewMaterialAction;
};

#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIALBROWSER_H
