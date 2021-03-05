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

#if !defined(Q_MOC_RUN)
#include "MaterialBrowser.h"

#include <QMainWindow>
#include <QPointer>
#include <QScopedPointer>
#endif

static const char* MATERIAL_EDITOR_NAME = "Material Editor";
static const char* MATERIAL_EDITOR_VER = "1.00";


class QComboBox;
class QLabel;

class CMaterial;
class CMaterialManager;
class CMatEditPreviewDlg;
class CMaterialSender;
class CMaterialImageListCtrl;
class QMaterialImageListModel;
class TwoColumnPropertyControl;

/** Dialog which hosts entity prototype library.
*/


struct SMaterialExcludedVars
{
    CMaterial* pMaterial;
    CVarBlock vars;

    SMaterialExcludedVars()
        : pMaterial(nullptr)
    {
    }
};


class CMaterialDialog
    : public QMainWindow
    , public IMaterialBrowserListener
    , public IDataBaseManagerListener
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    CMaterialDialog(QWidget* parent = 0);
    ~CMaterialDialog();

    static void RegisterViewClass();
    static const GUID& GetClassID();

public slots:
    void OnAssignMaterialToSelection();
    void OnResetMaterialOnSelection();
    void OnGetMaterialFromSelection();

protected:
    BOOL OnInitDialog();
    void closeEvent(QCloseEvent *ev) override;

protected slots:
    void OnAddItem();
    void OnDeleteItem();
    void OnSaveItem();
    void OnPickMtl();
    void OnCopy();
    void OnPaste();
    void OnMaterialPreview();
    void OnSelectAssignedObjects();
    void OnChangedBrowserListType(int);
    void OnResetMaterialViewport();

    void UpdateActions();

protected:
    //////////////////////////////////////////////////////////////////////////
    // Some functions can be overriden to modify standart functionality.
    //////////////////////////////////////////////////////////////////////////
    virtual void InitToolbar(UINT nToolbarResID);

    virtual void SelectItem(CBaseLibraryItem* item, bool bForceReload = false);
    virtual void DeleteItem(CBaseLibraryItem* pItem);
    virtual bool SetItemName(CBaseLibraryItem* item, const QString& groupName, const QString& itemName);
    virtual void ReloadItems();

    //////////////////////////////////////////////////////////////////////////
    // IMaterialBrowserListener implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnBrowserSelectItem(IDataBaseItem* pItem, bool bForce);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IDataBaseManagerListener implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);
    //////////////////////////////////////////////////////////////////////////

    // IEditorNotifyListener implementation.
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    //////////////////////////////////////////////////////////////////////////
    CMaterial* GetSelectedMaterial();
    void OnUpdateProperties(IVariable* var);
    void OnUndo(IVariable* pVar);

    void UpdateShaderParamsUI(CMaterial* pMtl);

    void UpdatePreview();

    //void SetTextureVars( CVariableArray *texVar,CMaterial *mtl,int id,const CString &name );
    void SetMaterialVars(CMaterial* mtl);

    MaterialBrowserWidget* m_wndMtlBrowser;

    QStatusBar* m_statusBar;
    //CXTCaption    m_wndCaption;

    TwoColumnPropertyControl* m_propsCtrl;
    bool m_bForceReloadPropsCtrl;

    QLabel* m_placeHolderLabel;

    CBaseLibraryItem* m_pPrevSelectedItem;

    // Material manager.
    CMaterialManager* m_pMatManager;

    CVarBlockPtr m_vars;
    CVarBlockPtr m_publicVars;

    // collection of excluded vars from m_publicVars for remembering values
    // when updating shader params
    SMaterialExcludedVars m_excludedPublicVars;

    CVarBlockPtr m_shaderGenParamsVars;
    CVarBlockPtr m_textureSlots;

    class CMaterialUI* m_pMaterialUI;

    QPointer<CMatEditPreviewDlg> m_pPreviewDlg;

    QScopedPointer<CMaterialImageListCtrl> m_pMaterialImageListCtrl;
    QScopedPointer<QMaterialImageListModel> m_pMaterialImageListModel;

    QToolBar* m_toolbar;
    QComboBox* m_filterTypeSelection;
    QAction* m_addAction;
    QAction* m_assignToSelectionAction;
    QAction* m_copyAction;
    QAction* m_getFromSelectionAction;
    QAction* m_pasteAction;
    QAction* m_pickAction;
    QAction* m_previewAction;
    QAction* m_removeAction;
    QAction* m_resetAction;
    QAction* m_saveAction;
    QAction* m_resetViewporAction;
};

