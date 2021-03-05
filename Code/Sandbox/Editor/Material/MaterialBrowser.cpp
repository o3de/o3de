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

#include "EditorDefs.h"

#include "MaterialBrowser.h"

// Qt
#include <QMessageBox>

// AzQtComponents
#include <AzQtComponents/Utilities/DesktopUtilities.h>

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>

// Editor
#include "MaterialManager.h"
#include "Clipboard.h"
#include "MaterialImageListCtrl.h"
#include "StringDlg.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Material/ui_MaterialBrowser.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


enum
{
    MENU_UNDEFINED = CMaterialImageListCtrl::MaterialBrowserWidgetActionsStart,
    MENU_CUT,
    MENU_COPY,
    MENU_COPY_NAME,
    MENU_PASTE,
    MENU_EXPLORE,
    MENU_DUPLICATE,
    MENU_EXTRACT,
    MENU_RENAME,
    MENU_DELETE,
    MENU_RESET,
    MENU_ASSIGNTOSELECTION,
    MENU_SELECTASSIGNEDOBJECTS,
    MENU_NUM_SUBMTL,
    MENU_ADDNEW,
    MENU_ADDNEW_MULTI,
    MENU_CONVERT_TO_MULTI,
    MENU_SUBMTL_MAKE,
    MENU_SUBMTL_CLEAR,
    MENU_SAVE_TO_FILE,
    MENU_SAVE_TO_FILE_MULTI,
    MENU_MERGE,

    MENU_SCM_ADD,
    MENU_SCM_CHECK_OUT,
    MENU_SCM_UNDO_CHECK_OUT,
    MENU_SCM_GET_LATEST,
    MENU_SCM_GET_LATEST_TEXTURES,
};

static QAction* CreateTreeViewAction(const char* text, const QKeySequence& shortcut, QWidget* shortcutContext, MaterialBrowserWidget* widget, void (MaterialBrowserWidget::*slot)())
{
    QAction* action = new QAction(text, shortcutContext);
    action->setShortcut(shortcut);
    QObject::connect(action, &QAction::triggered, widget, slot);
    widget->addAction(action);
    return action;
}

//////////////////////////////////////////////////////////////////////////
MaterialBrowserWidget::MaterialBrowserWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::MaterialBrowser)
    , m_filterModel(new MaterialBrowserFilterModel(this))
{
    using namespace AzToolsFramework::AssetBrowser;

    m_ui->setupUi(this);

    // create some permanent (for the life of this widget) actions for shortcut handling
    m_cutAction = CreateTreeViewAction("Cut", QKeySequence::Cut, m_ui->treeView, this, &MaterialBrowserWidget::OnCut);
    m_copyAction = CreateTreeViewAction("Copy", QKeySequence::Copy, m_ui->treeView, this, &MaterialBrowserWidget::OnCopy);
    m_pasteAction = CreateTreeViewAction("Paste", QKeySequence::Paste, m_ui->treeView, this, &MaterialBrowserWidget::OnPaste);
    m_duplicateAction = CreateTreeViewAction("Duplicate", QKeySequence(Qt::CTRL + Qt::Key_D), m_ui->treeView, this, &MaterialBrowserWidget::OnDuplicate);
    m_deleteAction = CreateTreeViewAction("Delete", QKeySequence::Delete, m_ui->treeView, this, &MaterialBrowserWidget::DeleteItem);
    m_renameItemAction = CreateTreeViewAction("Rename", Qt::Key_F2, m_ui->treeView, this, &MaterialBrowserWidget::OnRenameItem);
    m_addNewMaterialAction = CreateTreeViewAction("Add New Material", Qt::Key_Insert, m_ui->treeView, this, &MaterialBrowserWidget::OnAddNewMaterial);

    MaterialBrowserWidgetBus::Handler::BusConnect();

    // Get the asset browser model
    AssetBrowserComponentRequestBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(m_assetBrowserModel, "Failed to get filebrowser model");

    // Set up the filter model
    m_filterModel->setSourceModel(m_assetBrowserModel);
    m_ui->treeView->setModel(m_filterModel.data());
    m_ui->treeView->SetThumbnailContext("MaterialBrowser");
    m_ui->treeView->SetShowSourceControlIcons(true);

    m_ui->m_searchWidget->Setup(true, false);
    m_filterModel->SetSearchFilter(m_ui->m_searchWidget);
    connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel.data(), &MaterialBrowserFilterModel::SearchFilterUpdated);

    // Call LoadState to initialize the AssetBrowserTreeView's QTreeViewStateSaver
    // This must be done BEFORE StartRecordUpdateJobs(). A race condition from the update jobs was causing a 5-10% crash/hang when opening the Material Editor.
    m_ui->treeView->SetName("MaterialBrowserTreeView"); 
    
    // Override the AssetBrowserTreeView's custom context menu
    disconnect(m_ui->treeView, &QWidget::customContextMenuRequested, 0, 0);
    connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, [=](const QPoint& point)
        {
            CMaterialBrowserRecord record;
            bool found = TryGetSelectedRecord(record);
            ShowContextMenu(record, point);
        });

    connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MaterialBrowserWidget::OnSelectionChanged);
    //Wait for the signal emitted on record update jobs finished, then we can restore the selection for the previous selected item
    connect(this, SIGNAL(refreshSelection()), this, SLOT(OnRefreshSelection()));

    connect(this, SIGNAL(materialAdded()), this, SLOT(OnMaterialAdded()));

    m_bIgnoreSelectionChange = false;
    m_bItemsValid = true;

    m_pMatMan = GetIEditor()->GetMaterialManager();
    m_pMatMan->AddListener(this);
    m_pListener = NULL;

    m_viewType = VIEW_LEVEL;
    m_pMaterialImageListCtrl = NULL;
    m_pLastActiveMultiMaterial = 0;

    m_bNeedReload = false;

    m_bHighlightMaterial = false;
    m_timeOfHighlight = 0;

    m_bShowOnlyCheckedOut = false;

    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
MaterialBrowserWidget::~MaterialBrowserWidget()
{
    m_filterModel->CancelRecordUpdateJobs();
    m_filterModel->deleteLater();
    m_ui->treeView->SaveState();
    GetIEditor()->UnregisterNotifyListener(this);

    m_pMaterialImageListCtrl = NULL;
    m_pMatMan->RemoveListener(this);
    ClearItems();

    if (m_bHighlightMaterial)
    {
        m_pMatMan->SetHighlightedMaterial(0);
    }

    MaterialBrowserWidgetBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::ClearItems()
{
    m_bIgnoreSelectionChange = true;

    if (m_pMaterialImageListCtrl)
    {
        QMaterialImageListModel* materialModel =
            qobject_cast<QMaterialImageListModel*>(m_pMaterialImageListCtrl->model());
        Q_ASSERT(materialModel);
        materialModel->DeleteAllItems();
    }

    m_pLastActiveMultiMaterial = nullptr;
    m_delayedSelection = nullptr;

    m_bIgnoreSelectionChange = false;
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::TryLoadRecordMaterial(CMaterialBrowserRecord& record)
{
    // If material already loaded ignore.
    if (record.m_material)
    {
        return;
    }

    m_bIgnoreSelectionChange = true;
    // Try to load material for it.
    record.m_material = m_pMatMan->LoadMaterial(record.GetRelativeFilePath().c_str(), false);

    m_filterModel->SetRecord(record);
    m_bIgnoreSelectionChange = false;
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::TickRefreshMaterials()
{
    if (m_bHighlightMaterial)
    {
        uint32 t = GetTickCount();
        if ((t - m_timeOfHighlight) > 300)
        {
            m_bHighlightMaterial = false;
            m_pMatMan->SetHighlightedMaterial(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnIdleUpdate:
    {
        TickRefreshMaterials();
    }
    break;
    case eNotify_OnBeginLoad:
    {
        //Need to make sure the selection is cleared before clearing the record map
        SetSelectedItem(nullptr, nullptr, true);
        m_filterModel->ClearRecordMap();
    }
    break;
    case eNotify_OnCloseScene:
    {
        m_filterModel->ShowOnlyLevelMaterials(false, true);
        ClearItems();
        m_ui->treeView->SaveState();
        //Need to make sure the selection is cleared before clearing the record map
        SetSelectedItem(nullptr, nullptr, true);
        m_filterModel->ClearRecordMap();
    }
    break;
    case eNotify_OnEndNewScene:
    case eNotify_OnEndSceneOpen:
    {
        m_filterModel->ShowOnlyLevelMaterials(false, true);
        m_filterModel->StartRecordUpdateJobs();
        if (m_ui->treeView->IsTreeViewSavingReady())
        {
            m_ui->treeView->ApplyTreeViewSnapshot();
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::SetSelectedItem(_smart_ptr<CMaterial> material, const TMaterialBrowserRecords* pMarkedRecords, bool selectInTreeView)
{
    if (m_bIgnoreSelectionChange)
    {
        return;
    }

    m_bIgnoreSelectionChange = true;
    if (pMarkedRecords)
    {
        m_markedRecords = *pMarkedRecords;
    }
    else
    {
        m_markedRecords.clear();
    }

    m_pMatMan->SetCurrentFolder("");

    _smart_ptr<CMaterial> selectedMaterial = material;
    if (material && material->IsPureChild())
    {
        selectedMaterial = material->GetParent();
    }

    // Clear the delayed selection whenever a new material is selected
    m_delayedSelection = nullptr;

    // In some cases, such as when SetSelectedItem is called from the material picker or the rollup bar,
    // the material has not yet been selected in the tree-view, so that item must be selected here
    bool validSelection = false;
    if (selectInTreeView)
    {
        if (!selectedMaterial)
        {
            // Clear the current selection so that the upcoming call to RefreshSelected()
            // doesn't try to refresh the previously selected material which may be invalid
            m_ui->treeView->clearSelection();
            m_ui->treeView->setCurrentIndex(QModelIndex());
        }
        else if (m_markedRecords.size() > 0)
        {
            QItemSelection selection;
            for (CMaterialBrowserRecord record : m_markedRecords)
            {
                QModelIndex currentIndex = m_filterModel->GetIndexFromMaterial(record.m_material);
                if (currentIndex.isValid())
                {
                    selection.push_back(QItemSelectionRange(currentIndex));
                    validSelection = true;
                }
            }
            m_ui->treeView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            QModelIndex currentIndex = m_filterModel->GetIndexFromMaterial(selectedMaterial);
            if (currentIndex.isValid())
            {
                m_ui->treeView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
                validSelection = true;

                // Now that the parent material is selected in the browser,
                // select the appropriate sub-material in the material preview window
                if (selectedMaterial == material->GetParent())
                {
                    for (int i = 0; i < selectedMaterial->GetSubMaterialCount(); ++i)
                    {
                        if (material == selectedMaterial->GetSubMaterial(i))
                        {
                            m_selectedSubMaterialIndex = i;
                            break;
                        }
                    }
                }
            }
            else
            {
                // Hold on to a pointer to this material, listen for the MaterialFinishedProcessing event,
                // and attempt to re-select this material if it finishes processing in the background
                m_delayedSelection = selectedMaterial;
            }
        }
    }

    RefreshSelected();

    if (!selectedMaterial)
    {
        QModelIndex current = m_ui->treeView->currentIndex();
        QString path;
        while (current.isValid())
        {
            path = '/' + current.data().toString() + path;
            current = current.parent();
        }
        m_pMatMan->SetCurrentFolder((Path::GetEditingGameDataFolder() + path.toUtf8().data()).c_str());
    }
    else
    {
        if (selectedMaterial->IsMultiSubMaterial() && m_selectedSubMaterialIndex >= 0)
        {
            selectedMaterial = selectedMaterial->GetSubMaterial(m_selectedSubMaterialIndex);
        }
    }

    if (m_pListener)
    {
        // Don't call the listener event if we attempted to select an item in the tree view and failed
        // to prevent the material parameter editor from switching to a new material if it wasn't actually selected
        if (!selectInTreeView || validSelection)
        {
            m_pListener->OnBrowserSelectItem(selectedMaterial, false);
        }
        //Update the selected item if no material is selected in tree view
        else if (!selectedMaterial && selectInTreeView)
        {
            m_pListener->OnBrowserSelectItem(nullptr, false);
        }
    }

    m_timeOfHighlight = GetTickCount();
    m_pMatMan->SetHighlightedMaterial(selectedMaterial);
    if (selectedMaterial)
    {
        m_bHighlightMaterial = true;
    }

    std::vector<_smart_ptr<CMaterial> > markedMaterials;
    if (pMarkedRecords)
    {
        for (size_t i = 0; i < pMarkedRecords->size(); ++i)
        {
            markedMaterials.push_back((*pMarkedRecords)[i].m_material);
        }
    }
    m_pMatMan->SetMarkedMaterials(markedMaterials);

    m_bIgnoreSelectionChange = false;
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::SelectItem(IDataBaseItem* pItem, [[maybe_unused]] IDataBaseItem* pParentItem)
{
    if (m_bIgnoreSelectionChange)
    {
        return;
    }

    if (!pItem)
    {
        return;
    }

    bool bFound = false;

    _smart_ptr<CMaterial> material = static_cast<CMaterial*>(pItem);
    SetSelectedItem(material, 0, true);
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnDuplicate()
{
    GetIEditor()->GetMaterialManager()->Command_Merge();
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnCut()
{
    CMaterialBrowserRecord record;
    bool found = TryGetSelectedRecord(record);
    if (found)
    {
        OnCopy();
        DeleteItem(record);
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnCopyName()
{
    _smart_ptr<CMaterial> pMtl = GetCurrentMaterial();
    if (pMtl)
    {
        CClipboard clipboard(this);
        clipboard.PutString(pMtl->GetName(), "Material Name");
    }
}

void MaterialBrowserWidget::ShowOnlyLevelMaterials(bool levelOnly)
{
    m_filterModel->ShowOnlyLevelMaterials(levelOnly);
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnCopy()
{
    _smart_ptr<CMaterial> pMtl = GetCurrentMaterial();
    if (pMtl)
    {
        CClipboard clipboard(this);
        XmlNodeRef node = XmlHelpers::CreateXmlNode("Material");
        node->setAttr("Name", pMtl->GetName().toUtf8().data());
        CBaseLibraryItem::SerializeContext ctx(node, false);
        ctx.bCopyPaste = true;
        pMtl->Serialize(ctx);
        clipboard.Put(node);
    }
}

//////////////////////////////////////////////////////////////////////////
bool MaterialBrowserWidget::CanPaste() const
{
    CClipboard clipboard(nullptr);
    if (clipboard.IsEmpty())
    {
        return false;
    }
    XmlNodeRef node = clipboard.Get();
    if (!node)
    {
        return false;
    }

    if (strcmp(node->getTag(), "Material") == 0)
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnPaste()
{
    CClipboard clipboard(this);
    if (clipboard.IsEmpty())
    {
        return;
    }
    XmlNodeRef node = clipboard.Get();
    if (!node)
    {
        return;
    }

    if (strcmp(node->getTag(), "Material") == 0)
    {
        if (!m_pMatMan->GetCurrentMaterial())
        {
            GetIEditor()->GetMaterialManager()->Command_Create();
        }

        _smart_ptr<CMaterial> pMtl = m_pMatMan->GetCurrentMaterial();
        if (pMtl)
        {
            // This is material node.
            CBaseLibraryItem::SerializeContext serCtx(node, true);
            serCtx.bCopyPaste = true;
            serCtx.bUniqName = true;
            pMtl->Serialize(serCtx);

            SelectItem(pMtl, NULL);
            pMtl->Save();
            pMtl->Reload();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnAddNewMaterial()
{
    GetIEditor()->GetMaterialManager()->Command_Create();
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnAddNewMultiMaterial()
{
    GetIEditor()->GetMaterialManager()->Command_CreateMulti();
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnMergeMaterials()
{
    GetIEditor()->GetMaterialManager()->Command_Merge();
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnConvertToMulti()
{
    GetIEditor()->GetMaterialManager()->Command_ConvertToMulti();
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::DeleteItem()
{
    CMaterialBrowserRecord record;
    bool found = TryGetSelectedRecord(record);
    if (found)
    {
        DeleteItem(record);
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnResetItem()
{
    if (QMessageBox::question(this, tr("Reset Material"), tr("Reset Material to default?")) == QMessageBox::Yes)
    {
        _smart_ptr<CMaterial> pMtl = GetCurrentMaterial();
        int index;

        pMtl->GetSubMaterialCount() > 0 ? index = pMtl->GetSubMaterialCount() : index = 1;

        if (pMtl)
        {
            for (int i = 0; i < index; i++)
            {
                pMtl->GetSubMaterialCount() > 0 ? pMtl->GetSubMaterial(i)->Reload() : pMtl->Reload();
                TickRefreshMaterials();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::DeleteItem(const CMaterialBrowserRecord& record)
{
    _smart_ptr<CMaterial> pMtl = record.m_material;
    if (pMtl)
    {
        if (m_selectedSubMaterialIndex >= 0)
        {
            pMtl = pMtl->GetSubMaterial(m_selectedSubMaterialIndex);
            OnClearSubMtlSlot(pMtl);
        }
        else
        {
            GetIEditor()->GetMaterialManager()->Command_Delete();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnRenameItem()
{
    _smart_ptr<CMaterial> pMtl = GetCurrentMaterial();
    if (!pMtl)
    {
        return;
    }

    if (pMtl->IsPureChild())
    {
        StringDlg dlg(tr("Enter New Sub-Material Name"), this);
        dlg.SetString(pMtl->GetName());
        dlg.SetCheckCallback([this](QString name) -> bool
        {
            static const int MAX_REASONABLE_MATERIAL_NAME = 128;
            if (name.length() >= MAX_REASONABLE_MATERIAL_NAME)
            {
                QMessageBox::warning(this, tr("Name too long"), tr("Please enter a name less than %1 characters").arg(MAX_REASONABLE_MATERIAL_NAME));
                return false;
            }
            return true;
        });

        if (dlg.exec() == QDialog::Accepted)
        {
            pMtl->SetName(dlg.GetString());
            pMtl->Save();
            pMtl->Reload();
        }
    }
    else
    {
        if ((pMtl->GetFileAttributes() & SCC_FILE_ATTRIBUTE_MANAGED) &&
            !(pMtl->GetFileAttributes() & SCC_FILE_ATTRIBUTE_CHECKEDOUT))
        {
            if (QMessageBox::Cancel == QMessageBox::information(this, tr("Confirm"), tr("Only checked-out files can be renamed. Check out and mark for delete before rename it?"), QMessageBox::Ok | QMessageBox::Cancel))
            {
                return;
            }
        }

        QFileInfo info(pMtl->GetFilename());
        QString filename = info.fileName();
        if (!CFileUtil::SelectSaveFile("Material Files (*.mtl)", "mtl", info.path(), filename))
        {
            return;
        }

        QString itemName = m_pMatMan->FilenameToMaterial(Path::GetRelativePath(filename));
        if (itemName.isEmpty())
        {
            return;
        }

        if (m_pMatMan->FindItemByName(itemName))
        {
            Warning("Material with name %s already exist", itemName.toUtf8().data());
            return;
        }

        if (pMtl->GetFileAttributes() & SCC_FILE_ATTRIBUTE_MANAGED)
        {
            if (pMtl->GetFileAttributes() & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
            {
                if (QMessageBox::Cancel == QMessageBox::information(this, tr("Confirm"), tr("The original file will be marked for delete and the new named file will be marked for integration."), QMessageBox::Ok | QMessageBox::Cancel))
                {
                    return;
                }
            }
            else
            {
                CFileUtil::CheckoutFile(pMtl->GetFilename().toUtf8().data(), this);
            }

            if (!CFileUtil::RenameFile(pMtl->GetFilename().toUtf8().data(), filename.toUtf8().data()))
            {
                QMessageBox::critical(this, tr("Error"), tr("Could not rename file in Source Control."));
            }
        }


        // Delete file on disk.
        if (!pMtl->GetFilename().isEmpty())
        {
            QFile::remove(pMtl->GetFilename());
        }
        pMtl->SetName(itemName);
        pMtl->Save();
        SetSelectedItem(pMtl, 0, true);
    }
}


//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnSetSubMtlCount(const CMaterialBrowserRecord& record)
{
    _smart_ptr<CMaterial> pMtl = record.m_material;
    if (!pMtl)
    {
        return;
    }

    if (!pMtl->IsMultiSubMaterial())
    {
        return;
    }

    int num = pMtl->GetSubMaterialCount();
    bool ok = false;
    num = QInputDialog::getInt(this, tr("Number of Sub Materials"), QStringLiteral(""), num, 0, MAX_SUB_MATERIALS, 1, &ok);
    if (ok)
    {
        if (num != pMtl->GetSubMaterialCount())
        {
            if (m_selectedSubMaterialIndex >= num)
            {
                m_selectedSubMaterialIndex = num - 1;
            }

            CUndo undo("Set SubMtl Count");
            pMtl->SetSubMaterialCount(num);

            for (int i = 0; i < num; i++)
            {
                if (pMtl->GetSubMaterial(i) == 0)
                {
                    // Allocate pure childs for all empty slots.
                    QString name;
                    name = QStringLiteral("SubMtl%1").arg(i + 1);
                    _smart_ptr<CMaterial> pChild = new CMaterial(name, MTL_FLAG_PURE_CHILD);
                    pMtl->SetSubMaterial(i, pChild);
                }
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::DoSourceControlOp(CMaterialBrowserRecord& record, ESourceControlOp scmOp)
{
    if (!GetIEditor()->IsSourceControlAvailable())
    {
        return;
    }

    _smart_ptr<CMaterial> pMtl = record.m_material;

    if (pMtl && pMtl->IsPureChild())
    {
        pMtl = pMtl->GetParent();
    }

    if (scmOp == ESCM_IMPORT) // don't save unless you're doing an operation which writes.
    {
        if (pMtl && pMtl->IsModified())
        {
            pMtl->Save();
        }
    }

    QString path = pMtl ? pMtl->GetFilename() : QString();

    if (path.isEmpty())
    {
        return;
    }

    bool bRes = true;
    switch (scmOp)
    {
    case ESCM_IMPORT:
        if (pMtl)
        {
            QStringList filenames;
            int nTextures = pMtl->GetTextureFilenames(filenames);
            for (int i = 0; i < nTextures; ++i)
            {
                CFileUtil::CheckoutFile(filenames[i].toUtf8().data(), this);
            }

            bRes = CFileUtil::CheckoutFile(path.toUtf8().data(), this);
        }
        break;
    case ESCM_CHECKOUT:
    {
        if (pMtl && (pMtl->GetFileAttributes() & SCC_FILE_ATTRIBUTE_BYANOTHER))
        {
            AZStd::string otherUser("another user");
            AzToolsFramework::SourceControlFileInfo fileInfo;
            if (CFileUtil::GetFileInfoFromSourceControl(pMtl->GetFilename().toUtf8().data(), fileInfo, this))
            {
                // Sanity check the source control api reports the file is checked out by another
                AZ_Assert(fileInfo.HasFlag(AzToolsFramework::SCF_OtherOpen), "File attributes reporting incorrectly");
                otherUser = fileInfo.m_StatusUser;
            }

            if (QMessageBox::question(this, QString(), tr("This file is checked out by %1. Try to continue?").arg(otherUser.c_str())) != QMessageBox::Yes)
            {
                return;
            }
        }
        bRes = CFileUtil::GetLatestFromSourceControl(path.toUtf8().data(), this);
        if (bRes)
        {
            bRes = CFileUtil::CheckoutFile(path.toUtf8().data(), this);
        }
    }
    break;
    case ESCM_UNDO_CHECKOUT:
        bRes = CFileUtil::RevertFile(path.toUtf8().data(), this);
        break;
    case ESCM_GETLATEST:
        bRes = CFileUtil::GetLatestFromSourceControl(path.toUtf8().data(), this);
        break;
    case ESCM_GETLATESTTEXTURES:
        if (pMtl)
        {
            QString message;
            QStringList filenames;
            int nTextures = pMtl->GetTextureFilenames(filenames);
            for (int i = 0; i < nTextures; ++i)
            {
                bRes = CFileUtil::GetLatestFromSourceControl(filenames[i].toUtf8().data(), this);
                message += Path::GetRelativePath(filenames[i], true) + (bRes ? " [OK]" : " [Fail]") + "\n";
            }
            QMessageBox::information(this, QString(), message.isEmpty() ? tr("No files are affected") : message);
        }
        return;
    }

    if (!bRes)
    {
        QMessageBox::critical(this, tr("Error"), tr("Source Control Operation Failed.\r\nCheck if Source Control Provider correctly setup and working directory is correct."));
        return;
    }

    // force the cache to be rebuilt next time we repaint
    record.InitializeSourceControlAttributes();
    m_filterModel->SetRecord(record);

    if (pMtl)
    {
        pMtl->Reload();
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnMakeSubMtlSlot(const CMaterialBrowserRecord& record)
{
    if (m_selectedSubMaterialIndex >= 0)
    {
        _smart_ptr<CMaterial> parentMaterial = record.m_material;
        if (parentMaterial && parentMaterial->IsMultiSubMaterial())
        {
            const QString str = tr("Making new material will override material currently assigned to the slot %1 of %2\r\nMake new sub material?")
                    .arg(m_selectedSubMaterialIndex).arg(parentMaterial->GetName());
            if (QMessageBox::question(this, tr("Confirm Override"), str) == QMessageBox::Yes)
            {
                QString name = tr("SubMtl%1")
                        .arg(m_selectedSubMaterialIndex + 1);
                _smart_ptr<CMaterial> pMtl = new CMaterial(name, MTL_FLAG_PURE_CHILD);
                parentMaterial->SetSubMaterial(m_selectedSubMaterialIndex, pMtl);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnClearSubMtlSlot(_smart_ptr<CMaterial> subMaterial)
{
    if (subMaterial && m_selectedSubMaterialIndex >= 0)
    {
        _smart_ptr<CMaterial> parentMaterial = subMaterial->GetParent();
        if (parentMaterial && parentMaterial->IsMultiSubMaterial())
        {
            const QString str = tr("Clear Sub-Material Slot %1 of %2?").arg(m_selectedSubMaterialIndex).arg(parentMaterial->GetName());
            if (QMessageBox::question(this, tr("Clear Sub-Material"), str) == QMessageBox::Yes)
            {
                CUndo undo("Material Change");
                SetSubMaterial(parentMaterial, m_selectedSubMaterialIndex, 0);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::SetSubMaterial(_smart_ptr<CMaterial> parentMaterial, int slot, _smart_ptr<CMaterial> subMaterial)
{
    if (parentMaterial && parentMaterial->IsMultiSubMaterial())
    {
        // If the last sub-material is being removed, select the 2nd to last sub-material
        if (!subMaterial && slot == parentMaterial->GetSubMaterialCount() - 1)
        {
            m_selectedSubMaterialIndex = slot - 1;
        }
        parentMaterial->SetSubMaterial(slot, subMaterial);
    }
}


//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    if (m_bIgnoreSelectionChange)
    {
        return;
    }

    if (!pItem)
    {
        return;
    }


    switch (event)
    {
    case EDB_ITEM_EVENT_ADD:
        break;
    case EDB_ITEM_EVENT_DELETE:
        {
            if (pItem)
            {
                //If the deleted item is selected, remove selection
                CMaterial* pMtl = static_cast<CMaterial*>(pItem);
                CMaterial* selectedMaterial = GetCurrentMaterial();
                if (selectedMaterial && selectedMaterial->IsPureChild())
                {
                    selectedMaterial = selectedMaterial->GetParent();
                }
                if (pMtl == selectedMaterial)
                {
                    SetSelectedItem(nullptr, nullptr, true);
                }
            }
        }
        break;
    case EDB_ITEM_EVENT_CHANGED:
        {
            CMaterial* pMtl = static_cast<CMaterial*>(pItem);
            CMaterial* selectedMaterial = GetCurrentMaterial();
            if (selectedMaterial && selectedMaterial->IsPureChild())
            {
                selectedMaterial = selectedMaterial->GetParent();
            }
            // If this is a sub material, refresh parent
            if (pMtl->IsPureChild())
            {
                pMtl = pMtl->GetParent();
            }

            if (pMtl == selectedMaterial)
            {
                if (pMtl->IsMultiSubMaterial())
                {
                    m_pLastActiveMultiMaterial = NULL;
                }
                RefreshSelected();
            }
            m_bItemsValid = false;
        }
        break;
    case EDB_ITEM_EVENT_SELECTED:
        {
            SelectItem(pItem, NULL);
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::SetImageListCtrl(CMaterialImageListCtrl* pCtrl)
{
    m_pMaterialImageListCtrl = pCtrl;
    if (m_pMaterialImageListCtrl)
    {
        connect(m_pMaterialImageListCtrl->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MaterialBrowserWidget::OnSubMaterialSelectedInPreviewPane);

        connect(m_pMaterialImageListCtrl, &QAbstractItemView::clicked,
            this, &MaterialBrowserWidget::OnSubMaterialSelectedInPreviewPane);

        m_pMaterialImageListCtrl->SetMaterialBrowserWidget(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnSaveToFile(bool bMulti)
{
    _smart_ptr<CMaterial> pCurrentMaterial = GetCurrentMaterial();
    if (pCurrentMaterial)
    {
        QString startPath = GetIEditor()->GetSearchPath(EDITOR_PATH_MATERIALS);
        QString filename;
        if (!CFileUtil::SelectSaveFile("Material Files (*.mtl)", "mtl", startPath, filename))
        {
            return;
        }
        QFileInfo info(filename);
        QString itemName = info.baseName();
        itemName = Path::MakeGamePath(itemName);

        if (m_pMatMan->FindItemByName(itemName))
        {
            Warning("Material with name %s already exist", itemName.toUtf8().data());
            return;
        }
        int flags = pCurrentMaterial->GetFlags();
        if (bMulti)
        {
            flags |= MTL_FLAG_MULTI_SUBMTL;
        }

        pCurrentMaterial->SetFlags(flags);

        if (pCurrentMaterial->IsDummy())
        {
            pCurrentMaterial->ClearMatInfo();
            pCurrentMaterial->SetDummy(false);
        }
        pCurrentMaterial->SetModified(true);
        pCurrentMaterial->Save();
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::RefreshSelected()
{
    CMaterialBrowserRecord record;
    bool found = TryGetSelectedRecord(record);
    _smart_ptr<CMaterial> pMtl = nullptr;
    if (!found)
    {
        ClearImageListControlSelection();
        return;
    }
    else
    {
        pMtl = record.m_material;
    }

    if (m_pMaterialImageListCtrl)
    {
        QMaterialImageListModel* materialModel =
            qobject_cast<QMaterialImageListModel*>(m_pMaterialImageListCtrl->model());
        Q_ASSERT(materialModel);

        materialModel->InvalidateMaterial(pMtl);
        if (pMtl)
        {
            _smart_ptr<CMaterial> pMultiMtl = nullptr;
            if (pMtl->IsMultiSubMaterial())
            {
                // It's possible that the currently selected sub-material index is beyond the range of the current multi-material if, for example,
                // the last sub-material was selected but then the source .mtl file was changed to have fewer total sub-materials
                if (m_selectedSubMaterialIndex >= pMtl->GetSubMaterialCount())
                {
                    // If that's the case, select the last sub-material
                    // If the sub-material count has dropped to 0, setting m_selectedSubMaterialIndex to -1 will cause the parent material to be selected
                    m_selectedSubMaterialIndex = pMtl->GetSubMaterialCount() - 1;
                }

                pMultiMtl = pMtl;
                if (m_selectedSubMaterialIndex >= 0)
                {
                    pMtl = pMtl->GetSubMaterial(m_selectedSubMaterialIndex);
                }
            }

            if (m_pLastActiveMultiMaterial != pMultiMtl || pMultiMtl == nullptr)
            {
                // A new material was selected, so the previewer needs to be cleared
                materialModel->DeleteAllItems();
                // If the new material was not a multi-material, add it to the previewer
                if (pMultiMtl == nullptr)
                {
                    materialModel->AddMaterial(pMtl);
                }
            }

            // If a new multi-material was selected
            if (m_pLastActiveMultiMaterial != pMultiMtl && pMultiMtl != nullptr)
            {
                // Add all of its submaterials to the previewer
                for (size_t i = 0; i < pMultiMtl->GetSubMaterialCount(); i++)
                {
                    if (pMultiMtl->GetSubMaterial(i))
                    {
                        materialModel->AddMaterial(pMultiMtl->GetSubMaterial(i), reinterpret_cast<void*>(i));
                    }
                }
                m_pMaterialImageListCtrl->selectionModel()->clear();

                // If a sub-material was selected in the material browser, select it in the previewer
                QModelIndex modelIndex;
                if (m_selectedSubMaterialIndex >= 0)
                {
                    modelIndex = materialModel->index(m_selectedSubMaterialIndex, 0);
                }
                m_pMaterialImageListCtrl->selectionModel()->select(modelIndex, QItemSelectionModel::SelectCurrent);
            }

            m_pMaterialImageListCtrl->SelectMaterial(pMtl);
            m_pLastActiveMultiMaterial = pMultiMtl;
        }
        else
        {
            ClearSelection(materialModel);
        }
    }
}

void MaterialBrowserWidget::ClearImageListControlSelection()
{
    QMaterialImageListModel* materialModel =
        qobject_cast<QMaterialImageListModel*>(m_pMaterialImageListCtrl->model());
    Q_ASSERT(materialModel);

    ClearSelection(materialModel);
}

void MaterialBrowserWidget::ClearSelection(QMaterialImageListModel* materialModel)
{
    materialModel->DeleteAllItems();
    m_pLastActiveMultiMaterial = nullptr;
}

//////////////////////////////////////////////////////////////////////////

void MaterialBrowserWidget::AddContextMenuActionsMultiSelect(QMenu& menu) const
{
    int numMaterialsSelected = 0;
    for (int i = 0; i < m_markedRecords.size(); ++i)
    {
        if (m_markedRecords[i].m_material)
        {
            ++numMaterialsSelected;
        }
    }
    const QString itemsSelected = tr("  (%1 Materials Selected)").arg(numMaterialsSelected);
    menu.addAction(itemsSelected)->setEnabled(false);
    menu.addSeparator();

    if (numMaterialsSelected >= 2) // ... and at least two materials
    {
        menu.addAction(tr("Merge"))->setData(MENU_MERGE);
    }
    else
    {
        menu.addAction(tr("Merge (Select two or more)"))->setEnabled(false);
    }
}

void MaterialBrowserWidget::AddContextMenuActionsNoSelection(QMenu& menu) const
{
    QAction* action = menu.addAction(tr("Paste"));
    action->setShortcut(QKeySequence::Paste);
    action->setData(MENU_PASTE);
    action->setEnabled(CanPaste());

    menu.addSeparator();
    menu.addAction(tr("Add New Material"))->setData(MENU_ADDNEW);
    menu.addAction(tr("Add New Multi Material"))->setData(MENU_ADDNEW_MULTI);
    if (GetIEditor()->IsSourceControlAvailable())
    {
        menu.addSeparator();
        menu.addAction(tr("Source Control"))->setEnabled(false);
        menu.addAction(tr("Check Out"))->setData(MENU_SCM_CHECK_OUT);
        menu.addAction(tr("Undo Check Out"))->setData(MENU_SCM_UNDO_CHECK_OUT);
        menu.addAction(tr("Get Latest Version"))->setData(MENU_SCM_GET_LATEST);
    }
}

void MaterialBrowserWidget::AddContextMenuActionsSingleSelection(QMenu& menu, _smart_ptr<CMaterial> material) const
{
    if (material)
    {
        if (material->IsMultiSubMaterial())
        {
            if (m_selectedSubMaterialIndex >= 0)
            {
                AddContextMenuActionsSubMaterial(menu, material, material->GetSubMaterial(m_selectedSubMaterialIndex));
            }
            else
            {
                AddContextMenuActionsMultiMaterial(menu);
                AddContextMenuActionsCommon(menu, material);
            }
        }
        else if (material->IsPureChild())
        {
            AddContextMenuActionsSubMaterial(menu, material->GetParent(), material);
        }
        else
        {
            AddContextMenuActionsSingleMaterial(menu);
            AddContextMenuActionsCommon(menu, material);
        }
    }
}

void MaterialBrowserWidget::AddContextMenuActionsSubMaterial(QMenu& menu, _smart_ptr<CMaterial> parentMaterial, _smart_ptr<CMaterial> subMaterial) const
{
    bool enabled = true;

    if (parentMaterial)
    {
        uint32 nFileAttr = parentMaterial->GetFileAttributes();
        if (nFileAttr & SCC_FILE_ATTRIBUTE_READONLY)
        {
            enabled = false;
        }
    }

    QAction* action = menu.addAction(tr("Reset sub-material to default"));
    action->setData(MENU_SUBMTL_MAKE);
    action->setEnabled(enabled);

    menu.addSeparator();

    action = menu.addAction(tr("Cut"));
    action->setShortcut(QKeySequence::Cut);
    action->setData(MENU_CUT);
    action->setEnabled(enabled);

    action = menu.addAction(tr("Copy"));
    action->setShortcut(QKeySequence::Copy);
    action->setData(MENU_COPY);

    action = menu.addAction(tr("Paste"));
    action->setShortcut(QKeySequence::Paste);
    action->setData(MENU_PASTE);
    action->setEnabled(CanPaste() && enabled);

    action = menu.addAction(tr("Rename\tF2"));
    action->setData(MENU_RENAME);
    action->setEnabled(enabled);

    action = menu.addAction(tr("Delete"));
    action->setData(MENU_SUBMTL_CLEAR);
    action->setEnabled(enabled);
}

void MaterialBrowserWidget::AddContextMenuActionsMultiMaterial(QMenu& menu) const
{
    menu.addAction(tr("Set Number of Sub-Materials"))->setData(MENU_NUM_SUBMTL);
    menu.addSeparator();
}

void MaterialBrowserWidget::AddContextMenuActionsSingleMaterial(QMenu& menu) const
{
    menu.addAction(tr("Convert To Multi Material"))->setData(MENU_CONVERT_TO_MULTI);
    menu.addSeparator();
}

void MaterialBrowserWidget::AddContextMenuActionsCommon(QMenu& menu, _smart_ptr<CMaterial> material) const
{
    uint32 fileAttributes = material->GetFileAttributes();

    bool modificationsEnabled = true;
    if (fileAttributes & SCC_FILE_ATTRIBUTE_READONLY)
    {
        modificationsEnabled = false;
    }

    QAction* action = menu.addAction(tr("Cut"));
    action->setShortcut(QKeySequence::Cut);
    action->setData(MENU_CUT);
    action = menu.addAction(tr("Copy"));
    action->setShortcut(QKeySequence::Copy);
    action->setData(MENU_COPY);
    action = menu.addAction(tr("Paste"));
    action->setShortcut(QKeySequence::Paste);
    action->setData(MENU_PASTE);
    action->setEnabled(CanPaste() && modificationsEnabled);
    menu.addAction(tr("Copy Path to Clipboard"))->setData(MENU_COPY_NAME);
    if (fileAttributes & SCC_FILE_ATTRIBUTE_INPAK)
    {
        menu.addAction(tr("Extract"))->setData(MENU_EXTRACT);
    }
    else
    {
        menu.addAction(tr("Explore"))->setData(MENU_EXPLORE);
    }
    menu.addSeparator();
    action = menu.addAction(tr("Duplicate"));
    action->setShortcut(tr("Ctrl+D"));
    action->setData(MENU_DUPLICATE);
    menu.addAction(tr("Rename\tF2"))->setData(MENU_RENAME);
    action = menu.addAction(tr("Delete"));
    action->setShortcut(QKeySequence::Delete);
    action->setData(MENU_DELETE);
    menu.addSeparator();
    menu.addAction(tr("Assign to Selected Objects"))->setData(MENU_ASSIGNTOSELECTION);
    menu.addAction(tr("Select Assigned Objects"))->setData(MENU_SELECTASSIGNEDOBJECTS);
    menu.addSeparator();

    menu.addAction(tr("Add New Material"))->setData(MENU_ADDNEW);
    menu.addAction(tr("Add New Multi Material"))->setData(MENU_ADDNEW_MULTI);
    menu.addAction(tr("Merge (Select two or more)"))->setEnabled(false);

    AddContextMenuActionsSourceControl(menu, material, fileAttributes);
}

void MaterialBrowserWidget::AddContextMenuActionsSourceControl(QMenu& menu, _smart_ptr<CMaterial> material, uint32 fileAttributes) const
{
    if (GetIEditor()->IsSourceControlAvailable())
    {
        menu.addSeparator();

        if (fileAttributes & SCC_FILE_ATTRIBUTE_INPAK)
        {
            menu.addAction(tr("  Material In Pak (Read Only)"))->setEnabled(false);
        }
        else
        {
            menu.addAction(tr("  Source Control"))->setEnabled(false);
            if (!(fileAttributes & SCC_FILE_ATTRIBUTE_MANAGED))
            {
                menu.addAction(tr("Add To Source Control"))->setData(MENU_SCM_ADD);
            }
        }
        QAction* action = nullptr;
        if (fileAttributes & SCC_FILE_ATTRIBUTE_MANAGED)
        {
            action = menu.addAction(tr("Check Out"));
            action->setData(MENU_SCM_CHECK_OUT);
            action->setEnabled(fileAttributes & SCC_FILE_ATTRIBUTE_READONLY || fileAttributes & SCC_FILE_ATTRIBUTE_INPAK);
            action = menu.addAction(tr("Undo Check Out"));
            action->setData(MENU_SCM_UNDO_CHECK_OUT);
            action->setEnabled(fileAttributes & SCC_FILE_ATTRIBUTE_CHECKEDOUT);
            menu.addAction(tr("Get Latest Version"))->setData(MENU_SCM_GET_LATEST);
        }

        if (material)
        {
            QStringList filenames;
            int nTextures = material->GetTextureFilenames(filenames);
            action = menu.addAction(tr("Get Textures"));
            action->setData(MENU_SCM_GET_LATEST_TEXTURES);
            action->setEnabled(nTextures > 0);
        }
    }
}

void MaterialBrowserWidget::ShowContextMenu(const CMaterialBrowserRecord& record, const QPoint& point)
{
    _smart_ptr<CMaterial> material = record.m_material;

    // Create pop up menu.
    QMenu menu;
    if (m_markedRecords.size() >= 2) // it makes sense when we have at least two items selected
    {
        AddContextMenuActionsMultiSelect(menu);
    }
    else if (!material) // click on root, background or folder
    {
        AddContextMenuActionsNoSelection(menu);
    }
    else
    {
        // When right-clicking a single item in the material browser, select the parent material that was clicked, rather than the currently selected sub-material
        // The context menu for sub-materials will be handled by the image list control rather than the material browser widget
        m_selectedSubMaterialIndex = -1;
        SetSelectedItem(material, nullptr, true);
        AddContextMenuActionsSingleSelection(menu, material);
    }
    QAction* action = menu.exec(m_ui->treeView->mapToGlobal(point));
    const int cmd = action ? action->data().toInt() : 0;

    OnContextMenuAction(cmd, material);
}

void MaterialBrowserWidget::OnContextMenuAction(int command, _smart_ptr<CMaterial> material)
{
    CMaterialBrowserRecord record;
    TryGetSelectedRecord(record);
    switch (command)
    {
    case MENU_UNDEFINED:
        return;                  // do nothing
    case MENU_CUT:
        OnCut();
        break;
    case MENU_COPY:
        OnCopy();
        break;
    case MENU_COPY_NAME:
        OnCopyName();
        break;
    case MENU_PASTE:
        OnPaste();
        break;
    case MENU_EXPLORE:
    {
        if (material && material->IsPureChild())
        {
            material = material->GetParent();
        }

        if (material)
        {
            QString fullPath = material->GetFilename();
            AzQtComponents::ShowFileOnDesktop(fullPath);
        }
    }
    break;
    case MENU_EXTRACT:
    {
        if (material && material->IsPureChild())
        {
            material = material->GetParent();
        }

        if (material)
        {
            QString fullPath = material->GetFilename();
            if (CFileUtil::ExtractFile(fullPath, true, fullPath.toUtf8().data()))
            {
                AzQtComponents::ShowFileOnDesktop(fullPath);
            }
        }
    }
    break;
    case MENU_DUPLICATE:
        OnDuplicate();
        break;
    case MENU_RENAME:
        OnRenameItem();
        break;
    case MENU_DELETE:
        DeleteItem(record);
        break;
    case MENU_RESET:
        OnResetItem();
        break;
    case MENU_ASSIGNTOSELECTION:
    {
        CUndo undo("Assign Material To Selection");
        GetIEditor()->GetMaterialManager()->Command_AssignToSelection();
        break;
    }
    case MENU_SELECTASSIGNEDOBJECTS:
    {
        CUndo undo("Select Objects With Current Material");
        GetIEditor()->GetMaterialManager()->Command_SelectAssignedObjects();
        break;
    }
    case MENU_NUM_SUBMTL:
        OnSetSubMtlCount(record);
        break;
    case MENU_ADDNEW:
        OnAddNewMaterial();
        break;
    case MENU_ADDNEW_MULTI:
        OnAddNewMultiMaterial();
        break;
    case MENU_CONVERT_TO_MULTI:
        OnConvertToMulti();
        break;
    case MENU_MERGE:
        OnMergeMaterials();
        break;

    case MENU_SUBMTL_MAKE:
        OnMakeSubMtlSlot(record);
        break;
    case MENU_SUBMTL_CLEAR:
        OnClearSubMtlSlot(material);
        break;

    case MENU_SCM_ADD:
        DoSourceControlOp(record, ESCM_IMPORT);
        break;
    case MENU_SCM_CHECK_OUT:
        DoSourceControlOp(record, ESCM_CHECKOUT);
        break;
    case MENU_SCM_UNDO_CHECK_OUT:
        DoSourceControlOp(record, ESCM_UNDO_CHECKOUT);
        break;
    case MENU_SCM_GET_LATEST:
        DoSourceControlOp(record, ESCM_GETLATEST);
        break;
    case MENU_SCM_GET_LATEST_TEXTURES:
        DoSourceControlOp(record, ESCM_GETLATESTTEXTURES);
        break;

    case MENU_SAVE_TO_FILE:
        OnSaveToFile(false);
        break;
    case MENU_SAVE_TO_FILE_MULTI:
        OnSaveToFile(true);
        break;
        //case MENU_MAKE_SUBMTL: OnAddNewMultiMaterial(); break;
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::PopulateItems()
{
    if (m_bIgnoreSelectionChange)
    {
        return;
    }

    if (m_bItemsValid)
    {
        return;
    }

    m_bItemsValid = true;
    m_bIgnoreSelectionChange = true;

    IDataBaseItem* pSelection = m_pMatMan->GetSelectedItem();
    IDataBaseItem* pSelectionParent = m_pMatMan->GetSelectedParentItem();

    m_bIgnoreSelectionChange = false;

    if (pSelection)
    {
        SelectItem(pSelection, pSelectionParent);
        if (m_bHighlightMaterial)
        {
            m_bHighlightMaterial = false;
            m_pMatMan->SetHighlightedMaterial(0);
        }
    }
}

void MaterialBrowserWidget::StartRecordUpdateJobs()
{
    m_filterModel->StartRecordUpdateJobs();
}

//////////////////////////////////////////////////////////////////////////
uint32 MaterialBrowserWidget::MaterialNameToCrc32(const QString& str)
{
    return CCrc32::ComputeLowercase(str.toUtf8());
}

//////////////////////////////////////////////////////////////////////////
bool MaterialBrowserWidget::TryGetSelectedRecord(CMaterialBrowserRecord& record)
{
    QVariant variant = m_filterModel->data(m_ui->treeView->currentIndex(), Qt::UserRole);
    if (variant.isValid())
    {
        record = variant.value<CMaterialBrowserRecord>();
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<CMaterial> MaterialBrowserWidget::GetCurrentMaterial()
{
    CMaterialBrowserRecord record;
    bool found = TryGetSelectedRecord(record);
    if (found && record.m_material)
    {
        if (record.m_material->IsMultiSubMaterial() && m_selectedSubMaterialIndex >= 0)
        {
            return record.m_material->GetSubMaterial(m_selectedSubMaterialIndex);
        }
        else
        {
            return record.m_material;
        }
    }

    return m_pMatMan->GetCurrentMaterial();
}

//////////////////////////////////////////////////////////////////////////
AZStd::string MaterialBrowserWidget::GetSelectedMaterialID()
{
    CMaterialBrowserRecord record;
    bool found = TryGetSelectedRecord(record);
    if (found)
    {
        return record.GetRelativeFilePath();
    }
    return AZStd::string();
}

//////////////////////////////////////////////////////////////////////////
void MaterialBrowserWidget::OnSelectionChanged()
{
    m_selectedSubMaterialIndex = -1;
    TMaterialBrowserRecords markedRecords;

    CMaterialBrowserRecord record;
    bool found = TryGetSelectedRecord(record);
    for (const QModelIndex& row : m_ui->treeView->selectionModel()->selectedRows())
    {
        QVariant userRole = row.data(Qt::UserRole);
        if (userRole.isValid())
        {
            CMaterialBrowserRecord currentRecord = userRole.value<CMaterialBrowserRecord>();
            markedRecords.push_back(currentRecord);
        }
    }

    if (!found && !markedRecords.empty())
    {
        record = markedRecords.front();
        found = true;
    }
    if (found)
    {
        // Since the call to SetSelectedItem is coming from an OnSelectionChanged event, the appropriate
        // element of the tree view has already been selected. Pass in false for the selectInTreeView argument
        // to prevent SetSelectedItem from trying to re-select the material in the browser
        SetSelectedItem(record.m_material, &markedRecords, false);
    }
}

void MaterialBrowserWidget::OnRefreshSelection()
{
    // force RefreshSelected to repopulate by setting m_pLastActiveMultiMaterial to null
    // so it thinks we selected a new material.
    m_pLastActiveMultiMaterial = nullptr;

    //If no material is selected, clear preview.
    if (!GetCurrentMaterial())
    {
        if (m_pMaterialImageListCtrl)
        {
            QMaterialImageListModel* materialModel =
                qobject_cast<QMaterialImageListModel*>(m_pMaterialImageListCtrl->model());
            Q_ASSERT(materialModel);
            materialModel->DeleteAllItems();
        }
    }

    RefreshSelected();

    // Force update the material dialog
    if (m_pListener)
    {
        m_pListener->OnBrowserSelectItem(GetCurrentMaterial(), true);
    }
}

void MaterialBrowserWidget::OnMaterialAdded()
{
    if (m_delayedSelection)
    {
        SetSelectedItem(m_delayedSelection, nullptr, true);

        // Force update the material dialog
        if (m_pListener)
        {
            m_pListener->OnBrowserSelectItem(GetCurrentMaterial(), true);
        }
    }
}

void MaterialBrowserWidget::OnSubMaterialSelectedInPreviewPane(const QModelIndex& current)
{
    if (!m_pMaterialImageListCtrl)
    {
        return;
    }

    QMaterialImageListModel* materialModel =
        qobject_cast<QMaterialImageListModel*>(m_pMaterialImageListCtrl->model());
    Q_ASSERT(materialModel);

    int nSlot = (INT_PTR)materialModel->UserDataFromIndex(current);
    if (nSlot < 0)
    {
        return;
    }

    CMaterialBrowserRecord record;
    bool found = TryGetSelectedRecord(record);
    if (!found || !record.m_material)
    {
        return;
    }

    if (!record.m_material->IsMultiSubMaterial())
    {
        return; // must be multi sub material.
    }
    if (nSlot >= record.m_material->GetSubMaterialCount())
    {
        return;
    }

    if (nSlot == m_selectedSubMaterialIndex)
    {
        return;
    }

    m_selectedSubMaterialIndex = nSlot;

    SetSelectedItem(record.m_material, nullptr, false);
}

void MaterialBrowserWidget::SaveCurrentMaterial()
{
    // Saving might open a modal dialog asking to overwrite, therefore don't run this from DTOR, might crash

    _smart_ptr<CMaterial> pCurrentMtl = GetCurrentMaterial();
    if (pCurrentMtl && pCurrentMtl->IsModified())
    {
        pCurrentMtl->Save();
    }
}

void MaterialBrowserWidget::expandAllNotMatchingIndexes(const QModelIndex& parent)
{
    if (!parent.isValid())
    {
        m_ui->treeView->collapseAll();
    }

    const int rowCount = m_ui->treeView->model()->rowCount(parent);
    for (int row = 0; row < rowCount; ++row)
    {
        const QModelIndex index = m_ui->treeView->model()->index(row, 0, parent);
        const QString text = index.data().toString();
        bool contains = true;
        m_ui->treeView->setExpanded(index, !contains);
        if (!contains)
        {
            expandAllNotMatchingIndexes(index);
        }
    }
}

void MaterialBrowserWidget::MaterialAddFinished()
{
    emit materialAdded();
}

void MaterialBrowserWidget::MaterialFinishedProcessing(_smart_ptr<CMaterial> material, const QPersistentModelIndex& filterModelIndex)
{
    // If the currently selected material finished processing
    if (filterModelIndex.isValid() && filterModelIndex == m_ui->treeView->currentIndex())
    {
        // Stash the delayed selection so that it is not cleared just because the currently selected material finished processing
        _smart_ptr<CMaterial> tempDelayedSelection = m_delayedSelection;

        // Re-select the material to update the material dialogue
        // but ignore the tree-view selection since the current index is already the correct one for this material
        SetSelectedItem(material, nullptr, false);

        // Force update the material dialog
        if (m_pListener)
        {
            m_pListener->OnBrowserSelectItem(GetCurrentMaterial(), true);
        }

        if (material != m_delayedSelection)
        {
            // If the current selection that just finished processing was not the delayed selection, restore the delayed selection
            m_delayedSelection = tempDelayedSelection;
        }
    }
    // If there was a failed attempt to select the material before it finished processing
    else if (m_delayedSelection && m_delayedSelection == material)
    {
        // Re-select the material to update the material dialogue
        // and also select the material in the tree-view
        SetSelectedItem(material, nullptr, true);

        // Force update the material dialog
        if (m_pListener)
        {
            m_pListener->OnBrowserSelectItem(GetCurrentMaterial(), true);
        }
    }
}

void MaterialBrowserWidget::MaterialRecordUpdateFinished()
{
    //The event is sent by a AZJob working thread, need to emit a signal here for qt to catch later.
    //It can make sure all the qt functions need to be called at this point are only executed in the qt thread. 
    emit refreshSelection();
}


#include <Material/moc_MaterialBrowser.cpp>
