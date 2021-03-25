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

#include "LensFlareEditor.h"

// Qt
#include <QMenu>
#include <QMimeData>

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

// AzQtComponents
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/StyleManager.h>

// LmbrCentral
#include <LmbrCentral/Rendering/LensFlareAsset.h>   // for LmbrCentral::LensFlareAsset

// Editor
#include "Clipboard.h"
#include "StringDlg.h"
#include "Settings.h"
#include "Include/IObjectManager.h"
#include "LensFlareView.h"
#include "LensFlareAtomicList.h"
#include "LensFlareElementTree.h"
#include "LensFlareManager.h"
#include "LensFlareItem.h"
#include "LensFlareLibrary.h"
#include "LensFlareUndo.h"
#include "LensFlareLightEntityTree.h"
#include "Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h"
#include "ui_DatabaseFrameWnd.h"
#include "LyViewPaneNames.h"




CLensFlareEditor* CLensFlareEditor::s_pLensFlareEditor = NULL;
const char* CLensFlareEditor::s_pLensFlareEditorClassName = LyViewPane::LensFlareEditor;

#define DISABLE_REFERENCETREE

CLensFlareEditor::CLensFlareEditor(QWidget* pParent)
    : CDatabaseFrameWnd(GetIEditor()->GetLensFlareManager(), pParent)
    , m_pLensFlareView(NULL)
    , m_pLensFlareAtomicList(NULL)
    , m_pLensFlareElementTree(NULL)
    , m_pWndProps(NULL)
    , m_pLensFlareLightEntityTree(NULL)
    , m_pLensFlareReferenceTree(NULL)
    , m_LensFlareItemTree(NULL)
{
    assert(!s_pLensFlareEditor);
    s_pLensFlareEditor = this;

    m_pLensFlareView = new CLensFlareView(this);
    m_pLensFlareAtomicList = new CLensFlareAtomicList(this);
    m_pLensFlareElementTree = new CLensFlareElementTree(this);
    m_pWndProps = new ReflectedPropertyControl(this);
    m_pWndProps->Setup();
    m_pLensFlareLightEntityTree = new CLensFlareLightEntityTree(this);
#ifndef DISABLE_REFERENCETREE
    m_pLensFlareReferenceTree = new CLensFlareReferenceTree;
#endif
    m_LensFlareItemTree = new CLensFlareItemTree(this);

    m_pLibraryItemTreeModel = new LensFlareItemTreeModel(this);
    m_LensFlareItemTree->setModel(m_pLibraryItemTreeModel);

    setCentralWidget(m_pLensFlareView);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    m_advancedDockManager = new AzQtComponents::FancyDocking(this, "lensFlareEditor");

    addDockWidget(Qt::LeftDockWidgetArea, m_LensFlareItemTree, tr("Lens Flare Tree"), false);
    addDockWidget(Qt::LeftDockWidgetArea, m_pLensFlareElementTree, tr("Element Tree"), false);

    addDockWidget(Qt::BottomDockWidgetArea, m_pLensFlareAtomicList, tr("Basic Set"), false);
#ifndef DISABLE_REFERENCETREE
    addDockWidget(Qt::TopDockWidgetArea, m_pLensFlareReferenceTree, tr("Reference Tree"), false);
#endif

    addDockWidget(Qt::RightDockWidgetArea, m_pWndProps, tr("Properties"), false);
    addDockWidget(Qt::RightDockWidgetArea, m_pLensFlareLightEntityTree, tr("Light Entities"), false);

    m_pLensFlareElementTree->RegisterListener(this);
    m_pLensFlareElementTree->RegisterListener(m_pLensFlareView);

    m_pWndProps->ExpandAll();
    m_pWndProps->SetUpdateCallback(AZStd::bind(&CLensFlareEditor::OnUpdateProperties, this, AZStd::placeholders::_1));
    m_pWndProps->SetCallbackOnNonModified(false);

    connect(ui->actionDBAdd, &QAction::triggered, this, &CLensFlareEditor::OnAddItem);
    connect(ui->actionDBAssignToSelection, &QAction::triggered, this, &CLensFlareEditor::OnAssignFlareToLightEntities);
    connect(ui->actionDBGetFromSelection, &QAction::triggered, this, &CLensFlareEditor::OnGetFlareFromSelection);

    connect(m_pLibraryItemTreeModel, &LibraryItemTreeModel::itemRenamed, this, &CLensFlareEditor::OnItemTreeDataRenamed);

    AzQtComponents::StyleManager::setStyleSheet(this, "style:LensFlareEditor.qss");
}

CLensFlareEditor::~CLensFlareEditor()
{
    if (m_pLensFlareElementTree)
    {
        m_pLensFlareElementTree->UnregisterListener(this);
        m_pLensFlareElementTree->UnregisterListener(m_pLensFlareView);
    }

    ReleaseWindowsToBePutIntoPanels();
    s_pLensFlareEditor = NULL;
}

QMenu* CLensFlareEditor::createPopupMenu()
{
    return QMainWindow::createPopupMenu();
}

void CLensFlareEditor::OnInitDialog()
{
    InitTreeCtrl();

    connect(m_LensFlareItemTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CLensFlareEditor::OnTvnItemSelChanged);
    connect(m_LensFlareItemTree, &QWidget::customContextMenuRequested, this, &CLensFlareEditor::OnNotifyTreeRClick);

    m_pLensFlareAtomicList->FillAtomicItems();

    ReloadLibs();
}

void CLensFlareEditor::addDockWidget(Qt::DockWidgetArea area, QWidget* widget, const QString& title, bool closable)
{
    QDockWidget* w = new AzQtComponents::StyledDockWidget(title);
    w->setObjectName(widget->metaObject()->className());
    widget->setParent(w);
    w->setWidget(widget);
    if (!closable)
    {
        w->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    }
    QMainWindow::addDockWidget(area, w);
}

void CLensFlareEditor::ReleaseWindowsToBePutIntoPanels()
{
    SAFE_DELETE(m_pWndProps);
    SAFE_DELETE(m_pLensFlareAtomicList);
    SAFE_DELETE(m_pLensFlareView);
    SAFE_DELETE(m_pLensFlareElementTree);
    SAFE_DELETE(m_pLensFlareLightEntityTree);
#ifndef DISABLE_REFERENCETREE
    SAFE_DELETE(m_pLensFlareReferenceTree);
#endif
}

const GUID& CLensFlareEditor::GetClassID()
{
    // {7e7a40e0-f0b8-4918-b94f-8cdd6c55f30b}
    static const GUID guid = {
        0x7e7a40e0, 0xf0b8, 0x4918, { 0xb9, 0x4f, 0x8c, 0xdd, 0x6c, 0x55, 0xf3, 0x0b }
    };
    return guid;
}

void CLensFlareEditor::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    AzToolsFramework::RegisterViewPane<CLensFlareEditor>(s_pLensFlareEditorClassName, LyViewPane::CategoryOther, options);
    GetIEditor()->GetSettingsManager()->AddToolName("LensFlareEditor", "Lens Flare");
}

CLensFlareItem* CLensFlareEditor::GetSelectedLensFlareItem() const
{
    QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return nullptr;
    }
    return static_cast<CLensFlareItem*>(selected.first().data(Qt::UserRole).value<CBaseLibraryItem*>());
}

void CLensFlareEditor::ResetElementTreeControl()
{
    m_pLensFlareElementTree->InvalidateLensFlareItem();
}

void CLensFlareEditor::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    if (item != m_pCurrentItem || bForceReload)
    {
        //Ensure that the property window is emptied out whenever the selected library changes
        m_pWndProps->RemoveAllItems();
    }

    CDatabaseFrameWnd::SelectItem(item, bForceReload);
}

void CLensFlareEditor::OnTvnItemSelChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QModelIndex hItem;

    QModelIndexList selectedIndexes = selected.indexes();
    if (!selectedIndexes.isEmpty())
    {
        hItem = selectedIndexes.first();
    }

    QModelIndex hPrevItem;

    QModelIndexList deselectedIndexes = deselected.indexes();
    if (!deselectedIndexes.isEmpty())
    {
        hPrevItem = deselectedIndexes.first();
    }

    SelectLensFlareItem(hItem, hPrevItem);
    UpdateLensFlareItem(GetSelectedLensFlareItem());
}

void CLensFlareEditor::SelectLensFlareItem(const QModelIndex& hItem, const QModelIndex& hPrevItem)
{
    if (hPrevItem.isValid() && CUndo::IsRecording())
    {
        QString fullItemName = m_pLibraryItemTreeModel->GetFullName(hPrevItem);
        if (!fullItemName.isEmpty())
        {
            CUndo::Record(new CUndoLensFlareItemSelectionChange(fullItemName));
        }
    }
    m_pWndProps->RemoveAllItems();
    if (!hItem.isValid())
    {
        return;
    }
    CLensFlareItem* pSelectedLensFlareItem = static_cast<CLensFlareItem*>(hItem.data(Qt::UserRole).value<CBaseLibraryItem*>());
    SelectItem(pSelectedLensFlareItem);
}

void CLensFlareEditor::SelectLensFlareItem(const QModelIndex& hItem)
{
    QModelIndex hPrevItem;

    QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
    if (!selected.isEmpty())
    {
        hPrevItem = selected.first();
    }

    SelectLensFlareItem(hItem, hPrevItem);
}

void CLensFlareEditor::SelectLensFlareItem(const QString& fullItemName)
{
    QModelIndex hNewItem = m_pLibraryItemTreeModel->FindLibraryItemByFullName(fullItemName);
    if (!hNewItem.isValid())
    {
        return;
    }

    SelectLensFlareItem(hNewItem);
}

void CLensFlareEditor::RemovePropertyItems()
{
    m_pWndProps->RemoveAllItems();
}

bool CLensFlareEditor::IsExistTreeItem(const QString& name, bool bExclusiveSelectedItem)
{
    CBaseLibraryItem* pSelectedItem = GetSelectedLensFlareItem();

    for (int i = 0; i < m_pLibrary->GetItemCount(); i++)
    {
        CBaseLibraryItem* pItem = static_cast<CBaseLibraryItem*>(m_pLibrary->GetItem(i));

        if (!bExclusiveSelectedItem || pItem != pSelectedItem)
        {
            if (pItem->GetName() == name)
            {
                return true;
            }
        }
    }

    return false;
}

void CLensFlareEditor::RenameLensFlareItem(CLensFlareItem* pLensFlareItem, const QString& newGroupName, const QString& newShortName)
{
    if (pLensFlareItem == NULL || newShortName.isEmpty() || newGroupName.isEmpty())
    {
        return;
    }

    m_pLibraryItemTreeModel->Rename(pLensFlareItem, newGroupName, newShortName);
}

IOpticsElementBasePtr CLensFlareEditor::FindOptics(const QString& itemPath, const QString& opticsPath)
{
    CLensFlareItem* pItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(itemPath);
    if (pItem == NULL)
    {
        return NULL;
    }

    IOpticsElementBasePtr pOptics = pItem->GetOptics();
    if (pOptics == NULL)
    {
        return NULL;
    }

    return LensFlareUtil::FindOptics(pOptics, opticsPath);
}

void CLensFlareEditor::UpdateLensOpticsNames(const QString& oldFullName, const QString& newFullName)
{
    std::vector<CBaseObject*> pEntityObjects;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CEntityObject::staticMetaObject, pEntityObjects);
    for (int i = 0, iObjectSize(pEntityObjects.size()); i < iObjectSize; ++i)
    {
        CEntityObject* pEntity = (CEntityObject*)pEntityObjects[i];
        if (pEntity == NULL)
        {
            continue;
        }
        if (pEntity->CheckFlags(OBJFLAG_DELETED))
        {
            continue;
        }
        if (!pEntity->IsLight())
        {
            continue;
        }
        if (oldFullName == pEntity->GetEntityPropertyString(CEntityObject::s_LensFlarePropertyName))
        {
            pEntity->SetOpticsName(newFullName);
        }
    }
}

void CLensFlareEditor::OnCopy()
{
    UpdateClipboard(FLARECLIPBOARDTYPE_COPY);
}

void CLensFlareEditor::OnPaste()
{
    if (!m_pLibrary)
    {
        return;
    }

    CClipboard clipboard(this);
    if (clipboard.IsEmpty())
    {
        return;
    }

    XmlNodeRef clipboardXML = clipboard.Get();
    if (clipboardXML == NULL)
    {
        return;
    }

    Paste(clipboardXML);
}

void CLensFlareEditor::Paste(XmlNodeRef node)
{
    QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }
    Paste(selected.first(), node);
}

void CLensFlareEditor::Paste(const QModelIndex& hSelectedTreeItem, XmlNodeRef node)
{
    if (!m_pLibrary || node == NULL)
    {
        return;
    }

    QString type;
    node->getAttr("Type", type);

    QString sourceGroupName;
    node->getAttr("GroupName", sourceGroupName);

    if (m_pLibrary == NULL)
    {
        return;
    }

    bool bShouldCreateNewGroup(false);
    if (!sourceGroupName.isEmpty())
    {
        unsigned int nAnswer = CryMessageBox("Do you want to create a new group(YES) or add to the selected group(NO)?", "Question", MB_YESNOCANCEL);
        if (nAnswer == IDCANCEL)
        {
            return;
        }
        bShouldCreateNewGroup = nAnswer == IDYES;
    }

    QString targetGroupName;

    if (bShouldCreateNewGroup)
    {
        using namespace AZStd::placeholders;
        targetGroupName = MakeValidName("NewGroup", AZStd::bind(&CDatabaseFrameWnd::DoesGroupExist, this, _1, _2));
    }
    else
    {
        QModelIndex hTargetItem;
        if (!m_pLibraryItemTreeModel->hasChildren(hSelectedTreeItem))
        {
            hTargetItem = m_pLibraryItemTreeModel->parent(hSelectedTreeItem);
        }
        else
        {
            hTargetItem = hSelectedTreeItem;
        }

        if (!hTargetItem.isValid())
        {
            return;
        }

        targetGroupName = m_pLibraryItemTreeModel->GetFullName(hTargetItem);
    }

    CUndo undo(tr("Copy/Cut & Paste for Lens Flare").toUtf8());
    CLensFlareItem* pNewItem = NULL;

    for (int i = 0, iChildCount(node->getChildCount()); i < iChildCount; ++i)
    {
        LensFlareUtil::SClipboardData clipboardData;
        clipboardData.FillThisFromXmlNode(node->getChild(i));

        _smart_ptr<CLensFlareItem> pSourceItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(clipboardData.m_LensFlareFullPath);
        if (pSourceItem == NULL)
        {
            continue;
        }

        IOpticsElementBasePtr pSourceOptics = FindOptics(clipboardData.m_LensFlareFullPath, clipboardData.m_LensOpticsPath);
        if (pSourceOptics == NULL)
        {
            continue;
        }

        if (type == FLARECLIPBOARDTYPE_CUT)
        {
            if (clipboardData.m_From == LENSFLARE_ELEMENT_TREE)
            {
                LensFlareUtil::RemoveOptics(pSourceOptics);
            }
            else
            {
                DeleteItem(pSourceItem);
            }
        }

        QString sourceName;
        if (clipboardData.m_From == LENSFLARE_ELEMENT_TREE)
        {
            sourceName = LensFlareUtil::GetShortName(pSourceOptics->GetName().c_str());
        }
        else
        {
            sourceName = pSourceItem->GetShortName();
        }

        QString candidateName = targetGroupName + QString(".") + sourceName;
        QString validName = MakeValidName(candidateName,
                                AZStd::bind(&CDatabaseFrameWnd::DoesItemExist, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
        QString validShortName = LensFlareUtil::GetShortName(validName);
        pNewItem = AddNewLensFlareItem(targetGroupName, validShortName);
        assert(pNewItem);
        if (pNewItem == NULL)
        {
            continue;
        }

        if (LensFlareUtil::IsGroup(pSourceOptics->GetType()))
        {
            LensFlareUtil::CopyOptics(pSourceOptics, pNewItem->GetOptics(), true);
        }
        else
        {
            IOpticsElementBasePtr pNewOptics = LensFlareUtil::CreateOptics(pSourceOptics);
            if (pNewOptics)
            {
                pNewItem->GetOptics()->AddElement(pNewOptics);
            }
        }

        if (type == FLARECLIPBOARDTYPE_CUT)
        {
            if (clipboardData.m_From == LENSFLARE_ITEM_TREE)
            {
                UpdateLensOpticsNames(pSourceItem->GetFullName(), pNewItem->GetFullName());
            }
            if (pSourceItem != GetSelectedLensFlareItem())
            {
                pSourceItem->UpdateLights();
            }
        }

        LensFlareUtil::UpdateOpticsName(pNewItem->GetOptics());
        LensFlareUtil::ChangeOpticsRootName(pNewItem->GetOptics(), validShortName);

        ReloadItems();
        SelectLensFlareItem(m_pLibraryItemTreeModel->index(pNewItem), {});
    }
}

void CLensFlareEditor::OnCut()
{
    UpdateClipboard(FLARECLIPBOARDTYPE_CUT);
}

XmlNodeRef CLensFlareEditor::CreateXML(const char* type) const
{
    std::vector<LensFlareUtil::SClipboardData> clipboardDataList;
    QString groupName;
    if (!GetClipboardDataList(clipboardDataList, groupName))
    {
        return NULL;
    }
    return LensFlareUtil::CreateXMLFromClipboardData(type, groupName, false, clipboardDataList);
}

void CLensFlareEditor::UpdateClipboard(const char* type) const
{
    std::vector<LensFlareUtil::SClipboardData> clipboardDataList;
    QString groupName;
    if (!GetClipboardDataList(clipboardDataList, groupName))
    {
        return;
    }
    LensFlareUtil::UpdateClipboard(type, groupName, false, clipboardDataList);
}

bool CLensFlareEditor::GetClipboardDataList(std::vector<LensFlareUtil::SClipboardData>& outList, QString& outGroupName) const
{
    CLensFlareLibrary* pLibrary = GetCurrentLibrary();
    if (pLibrary == NULL)
    {
        return false;
    }

    std::vector<LensFlareUtil::SClipboardData> clipboardDataList;

    CLensFlareItem* pLensFlareItem = GetSelectedLensFlareItem();
    if (pLensFlareItem)
    {
        clipboardDataList.push_back(LensFlareUtil::SClipboardData(LENSFLARE_ITEM_TREE, pLensFlareItem->GetFullName(), pLensFlareItem->GetShortName()));
    }
    else if (GetSelectedItemStatus() == eSIS_Group)
    {
        QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
        if (selected.isEmpty())
        {
            return false;
        }
        outGroupName = selected.first().data(Qt::DisplayRole).toString();
        for (CBaseLibraryItem* pItem : m_pLibraryItemTreeModel->ChildItems(selected.first()))
        {
            pLensFlareItem = static_cast<CLensFlareItem*>(pItem);
            clipboardDataList.push_back(LensFlareUtil::SClipboardData(LENSFLARE_ITEM_TREE, pLensFlareItem->GetFullName(), pLensFlareItem->GetShortName()));
        }
    }
    else
    {
        return false;
    }

    outList = clipboardDataList;

    return true;
}

void CLensFlareEditor::OnAddItem()
{
    if (!m_pLibrary)
    {
        return;
    }

    StringGroupDlg dlg(tr("New Flare Name"), this);
    dlg.SetGroup(m_selectedGroup);

    if (dlg.exec() != QDialog::Accepted || dlg.GetString().isEmpty())
    {
        return;
    }

    QString fullName = m_pItemManager->MakeFullItemName(m_pLibrary, dlg.GetGroup(), dlg.GetString());
    if (m_pItemManager->FindItemByName(fullName))
    {
        Warning("Item with name %s already exist", fullName.toUtf8().data());
        return;
    }

    CUndo undo(tr("Add flare library item").toUtf8());

    CLensFlareItem* pNewLensFlare = AddNewLensFlareItem(dlg.GetGroup(), dlg.GetString());
    if (pNewLensFlare)
    {
        m_pLibraryItemTreeModel->Add(pNewLensFlare);
        SelectItem(pNewLensFlare);
    }
}

CLensFlareItem* CLensFlareEditor::AddNewLensFlareItem(const QString& groupName, const QString& shortName)
{
    CLensFlareItem* pNewFlare = (CLensFlareItem*)m_pItemManager->CreateItem(m_pLibrary);
    if (pNewFlare == NULL)
    {
        return NULL;
    }
    if (!SetItemName(pNewFlare, groupName, shortName))
    {
        m_pItemManager->DeleteItem(pNewFlare);
        return NULL;
    }
    pNewFlare->GetOptics()->SetName(pNewFlare->GetShortName().toUtf8().data());
    return pNewFlare;
}

QModelIndex CLensFlareEditor::GetTreeLensFlareItem(CLensFlareItem* pItem) const
{
    return m_pLibraryItemTreeModel->index(pItem);
}

void CLensFlareEditor::OnCopyNameToClipboard()
{
    QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }
    QString itemName = m_pLibraryItemTreeModel->GetFullName(selected.first());
    CClipboard clipboard(this);
    clipboard.PutString(itemName);
}

void CLensFlareEditor::OnAssignFlareToLightEntities()
{
    CLensFlareItem* pLensFlareItem = GetSelectedLensFlareItem();
    if (pLensFlareItem == NULL)
    {
        return;
    }
    std::vector<CEntityObject*> entityList;
    LensFlareUtil::GetSelectedLightEntities(entityList);

    for (int i = 0, iEntityCount(entityList.size()); i < iEntityCount; ++i)
    {
        entityList[i]->ApplyOptics(pLensFlareItem->GetFullName(), pLensFlareItem->GetOptics());
    }

    m_pLensFlareLightEntityTree->OnLensFlareChangeItem(pLensFlareItem);
}

void CLensFlareEditor::OnSelectAssignedObjects()
{
    std::vector<CEntityObject*> lightEntities;
    LensFlareUtil::GetLightEntityObjects(lightEntities);

    CLensFlareItem* pLensFlareItem = GetSelectedLensFlareItem();
    if (pLensFlareItem == NULL)
    {
        return;
    }

    int iLightSize(lightEntities.size());
    std::vector<CBaseObject*> assignedObjects;
    assignedObjects.reserve(iLightSize);

    for (int i = 0; i < iLightSize; ++i)
    {
        CEntityObject* pLightEntity = lightEntities[i];
        if (pLightEntity == NULL)
        {
            continue;
        }

        IOpticsElementBasePtr pTargetOptics = pLightEntity->GetOpticsElement();
        if (pTargetOptics == NULL)
        {
            continue;
        }

        if (QString::compare(pLensFlareItem->GetFullName(), pTargetOptics->GetName().c_str(), Qt::CaseInsensitive))
        {
            continue;
        }

        assignedObjects.push_back(pLightEntity);
    }

    if (assignedObjects.empty())
    {
        return;
    }

    GetIEditor()->ClearSelection();
    for (int i = 0, iAssignedObjectsSize(assignedObjects.size()); i < iAssignedObjectsSize; ++i)
    {
        GetIEditor()->SelectObject(assignedObjects[i]);
    }
}

void CLensFlareEditor::OnGetFlareFromSelection()
{
    CEntityObject* pEntity = LensFlareUtil::GetSelectedLightEntity();
    if (pEntity == NULL)
    {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a light entity first."));
        return;
    }
    if (m_pLibrary == NULL)
    {
        return;
    }

    QString fullFlareName = pEntity->GetEntityPropertyString(CEntityObject::s_LensFlarePropertyName);
    int nDotPosition = fullFlareName.indexOf(".");
    if (nDotPosition == -1)
    {
        return;
    }

    QString libraryName = fullFlareName.left(nDotPosition);
    if (m_pLibrary->GetName() != libraryName)
    {
        GetIEditor()->GetLensFlareManager()->LoadItemByName(fullFlareName);
        SelectLibrary(libraryName);
    }

    SelectItemByName(fullFlareName);
}

void CLensFlareEditor::OnUpdateTreeCtrl()
{
    OnGetFlareFromSelection();
}

void CLensFlareEditor::OnNotifyTreeRClick()
{
    CLensFlareItem* pLensFlareItem = 0;

    QModelIndex hItem = LensFlareUtil::GetTreeItemByHitTest(*GetTreeCtrl());
    if (hItem.isValid())
    {
        pLensFlareItem = static_cast<CLensFlareItem*>(hItem.data(Qt::UserRole).value<CBaseLibraryItem*>());
    }

    SelectItem(pLensFlareItem);

    bool bGrayedFlag = false;
    if (!pLensFlareItem)
    {
        bGrayedFlag = true;
    }

    CClipboard clipboard(this);
    bool bPasteFlags = true;
    if (clipboard.IsEmpty())
    {
        bPasteFlags = false;
    }

    bool bCopyPasteCloneFlag = true;
    if (GetSelectedItemStatus() != eSIS_Flare && GetSelectedItemStatus() != eSIS_Group)
    {
        bCopyPasteCloneFlag = false;
    }

    QMenu menu;

    QAction* actionDBCut = menu.addAction(tr("Cut"));
    actionDBCut->setEnabled(bCopyPasteCloneFlag);
    connect(actionDBCut, &QAction::triggered, this, &CLensFlareEditor::OnCut);

    QAction* actionDBCopy = menu.addAction(tr("Copy"));
    actionDBCopy->setEnabled(bCopyPasteCloneFlag);
    connect(actionDBCopy, &QAction::triggered, this, &CLensFlareEditor::OnCopy);

    QAction* actionDBPaste = menu.addAction(tr("Paste"));
    actionDBPaste->setEnabled(bPasteFlags);
    connect(actionDBPaste, &QAction::triggered, this, &CLensFlareEditor::OnPaste);

    QAction* actionDBClone = menu.addAction(tr("Clone"));
    actionDBClone->setEnabled(bCopyPasteCloneFlag);
    connect(actionDBClone, &QAction::triggered, this, &CLensFlareEditor::OnClone);

    menu.addSeparator();

    QAction* actionDBRename = menu.addAction(tr("Rename\tF2"));
    connect(actionDBRename, &QAction::triggered, this, &CLensFlareEditor::OnRenameItem);

    QAction* actionDBRemove = menu.addAction(tr("Delete\tDel"));
    connect(actionDBRemove, &QAction::triggered, this, &CLensFlareEditor::OnRemoveItem);

    menu.addSeparator();

    QAction* actionDBAssignToSelection = menu.addAction(tr("Assign to Selected Objects"));
    actionDBAssignToSelection->setEnabled(!bGrayedFlag);
    connect(actionDBAssignToSelection, &QAction::triggered, this, &CLensFlareEditor::OnAssignFlareToLightEntities);

    QAction* actionDBSelectAssignedObjects = menu.addAction(tr("Select Assigned Objects"));
    actionDBSelectAssignedObjects->setEnabled(!bGrayedFlag);
    connect(actionDBSelectAssignedObjects, &QAction::triggered, this, &CLensFlareEditor::OnSelectAssignedObjects);

    QAction* actionDBCopyNameToClipboard = menu.addAction(tr("Copy Name to ClipBoard"));
    connect(actionDBCopyNameToClipboard, &QAction::triggered, this, &CLensFlareEditor::OnCopyNameToClipboard);

    menu.exec(QCursor::pos());
}

bool CLensFlareEditor::SelectItemByName(const QString& itemName)
{
    CLensFlareItem* pLensFlareItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(itemName);
    if (pLensFlareItem == NULL)
    {
        return false;
    }

    GetIEditor()->GetLensFlareManager()->SetSelectedItem(pLensFlareItem);

    SelectLensFlareItem(m_pLibraryItemTreeModel->index(pLensFlareItem));

    return true;
}


void CLensFlareEditor::StartEditItem(const QModelIndex& hItem)
{
    if (!hItem.isValid())
    {
        return;
    }
    GetTreeCtrl()->edit(hItem);
}

bool CLensFlareEditor::GetFullLensFlareItemName(const QModelIndex& hItem, QString& outFullName) const
{
    if (!hItem.isValid())
    {
        return false;
    }

    outFullName = m_pLibraryItemTreeModel->GetFullName(hItem);

    return true;
}

CLensFlareEditor::ESelectedItemStatus CLensFlareEditor::GetSelectedItemStatus() const
{
    QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return eSIS_Unselected;
    }
    if (m_pLibraryItemTreeModel->hasChildren(selected.first()))
    {
        return eSIS_Group;
    }
    return eSIS_Flare;
}

void CLensFlareEditor::SelectItemInLensFlareElementTreeByName(const QString& name)
{
    m_pLensFlareElementTree->SelectTreeItemByName(name);
}

bool CLensFlareEditor::GetSelectedLensFlareName(QString& outName) const
{
    const CLensFlareElement* pElement = m_pLensFlareElementTree->GetCurrentLensFlareElement();
    if (pElement == NULL)
    {
        return false;
    }
    return pElement->GetName(outName);
}

void CLensFlareEditor::ReloadItems()
{
    CDatabaseFrameWnd::ReloadItems();
    ResetElementTreeControl();
}

void CLensFlareEditor::OnRemoveItem()
{
    CLensFlareItem* pSelectedLensFlareItem = GetSelectedLensFlareItem();
    CUndo undo(tr("Remove Flare Group Item").toUtf8());

    if (pSelectedLensFlareItem == NULL)
    {
        QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
        if (selected.isEmpty())
        {
            return;
        }

        QModelIndex hLensFlareItem = selected.first();

        QString fullLensFlareItemName = m_pLibraryItemTreeModel->GetFullName(hLensFlareItem);

        if (!fullLensFlareItemName.isEmpty())
        {
            QString deleteMsgStr = tr("Delete %1?").arg(fullLensFlareItemName);
            if (QMessageBox::question(this, tr("Confirmation"), deleteMsgStr) == QMessageBox::Yes)
            {
                CLensFlareLibrary* pLensFlareLibrary = GetCurrentLibrary();
                if (pLensFlareLibrary)
                {
                    std::vector< _smart_ptr<CLensFlareItem> > deletedItems;
                    for (int i = 0, iItemCount(pLensFlareLibrary->GetItemCount()); i < iItemCount; ++i)
                    {
                        CLensFlareItem* pItem = (CLensFlareItem*)pLensFlareLibrary->GetItem(i);
                        if (pItem == NULL)
                        {
                            continue;
                        }
                        if (pItem->GetName().indexOf(fullLensFlareItemName) == -1)
                        {
                            continue;
                        }
                        QModelIndex hFlareItem = m_pLibraryItemTreeModel->index(pItem);
                        if (hFlareItem.isValid())
                        {
                            UpdateLensOpticsNames(pItem->GetFullName(), "");
                            deletedItems.push_back(pItem);
                        }
                    }
                    for (int i = 0, iDeleteItemSize(deletedItems.size()); i < iDeleteItemSize; ++i)
                    {
                        m_pLensFlareLightEntityTree->OnLensFlareDeleteItem(deletedItems[i]);
                        DeleteItem(deletedItems[i]);
                    }

                    m_pLibraryItemTreeModel->removeRow(hLensFlareItem.row(), hLensFlareItem.parent());
                }
                GetIEditor()->SetModifiedFlag();
            }
        }
    }
    else
    {
        QString deleteMsgStr = tr("Delete %1?").arg(pSelectedLensFlareItem->GetName());
        // Remove prototype from prototype manager and library.
        if (QMessageBox::question(this, tr("Confirmation"), deleteMsgStr) == QMessageBox::Yes)
        {
            m_pLensFlareLightEntityTree->OnLensFlareDeleteItem(pSelectedLensFlareItem);

            DeleteItem(pSelectedLensFlareItem);

            UpdateLensOpticsNames(pSelectedLensFlareItem->GetFullName(), "");

            QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
            if (!selected.isEmpty())
            {
                QModelIndex hLensFlareItem = selected.first();
                m_pLibraryItemTreeModel->removeRow(hLensFlareItem.row(), hLensFlareItem.parent());
            }

            m_pCurrentItem = NULL;

            GetIEditor()->SetModifiedFlag();
            SelectItem(0);
        }
    }
}

void CLensFlareEditor::OnAddLibrary()
{
    StringDlg dlg(tr("Library Name"), this);

    dlg.SetCheckCallback([this](QString library) -> bool
    {
        QString path = m_pItemManager->MakeFilename(library);
        if (CFileUtil::FileExists(path))
        {
            QMessageBox::warning(this, tr("Library exists"), tr("Library '%1' already exists.").arg(library));
            return false;
        }
        return true;
    });

    if (dlg.exec() == QDialog::Accepted)
    {
        if (!dlg.GetString().isEmpty())
        {
            SelectItem(0);
            // Make new library.
            QString library = dlg.GetString();
            NewLibrary(library);
            ReloadLibs();
            SelectLibrary(library);
            GetIEditor()->SetModifiedFlag();
        }
    }
}

void CLensFlareEditor::OnRenameItem()
{
    QModelIndexList selected = GetTreeCtrl()->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }
    StartEditItem(selected.first());
}

void CLensFlareEditor::OnReloadLib()
{
    CDatabaseFrameWnd::OnReloadLib();
    CLensFlareLibrary* pLibrary = GetCurrentLibrary();
    if (pLibrary == NULL)
    {
        return;
    }
    for (int i = 0, iItemCount(pLibrary->GetItemCount()); i < iItemCount; ++i)
    {
        CLensFlareItem* pItem = (CLensFlareItem*)pLibrary->GetItem(i);
        if (pItem == NULL)
        {
            continue;
        }
        pItem->UpdateLights(NULL);
    }
}

void CLensFlareEditor::RegisterLensFlareItemChangeListener(ILensFlareChangeItemListener* pListener)
{
    for (int i = 0, iSize(m_LensFlareChangeItemListenerList.size()); i < iSize; ++i)
    {
        if (m_LensFlareChangeItemListenerList[i] == pListener)
        {
            return;
        }
    }
    m_LensFlareChangeItemListenerList.push_back(pListener);
}

void CLensFlareEditor::UnregisterLensFlareItemChangeListener(ILensFlareChangeItemListener* pListener)
{
    auto iter = m_LensFlareChangeItemListenerList.begin();
    for (; iter != m_LensFlareChangeItemListenerList.end(); ++iter)
    {
        if (*iter == pListener)
        {
            m_LensFlareChangeItemListenerList.erase(iter);
            return;
        }
    }
}

void CLensFlareEditor::OnItemTreeDataRenamed(CBaseLibraryItem* pItem, const QString& prevFullName)
{
    CLensFlareItem* pLensFlareItem = static_cast<CLensFlareItem*>(pItem);

    UpdateLensOpticsNames(prevFullName, pLensFlareItem->GetFullName());
    LensFlareUtil::ChangeOpticsRootName(pLensFlareItem->GetOptics(), pLensFlareItem->GetShortName());

    if (pLensFlareItem == GetSelectedLensFlareItem())
    {
        UpdateLensFlareItem(pLensFlareItem);
    }

    GetIEditor()->GetLensFlareManager()->Modified();
}

void CLensFlareEditor::UpdateLensFlareItem(CLensFlareItem* pLensFlareItem)
{
    for (int i = 0, iSize(m_LensFlareChangeItemListenerList.size()); i < iSize; ++i)
    {
        m_LensFlareChangeItemListenerList[i]->OnLensFlareChangeItem(pLensFlareItem);
    }
}

void CLensFlareEditor::OnLensFlareChangeElement(CLensFlareElement* pLensFlareElement)
{
    ReflectedPropertyControl* pPropertyCtrl = m_pWndProps;
    pPropertyCtrl->RemoveAllItems();

    if (pLensFlareElement == NULL)
    {
        return;
    }

    // Update properties in a property panel
    if (pLensFlareElement && pLensFlareElement->GetProperties())
    {
        pPropertyCtrl->AddVarBlock(pLensFlareElement->GetProperties());
        pPropertyCtrl->ExpandAllChildren(pPropertyCtrl->GetRootItem(), false);
        pPropertyCtrl->setEnabled(true);
    }
}

void CLensFlareEditor::AddNewItemByAtomicOptics(const QModelIndex& hSelectedItem, EFlareType flareType)
{
    if (!LensFlareUtil::IsValidFlare(flareType))
    {
        return;
    }

    QString groupName;
    QModelIndex hParentItem = hSelectedItem.parent();
    if (hParentItem.isValid())
    {
        groupName = hParentItem.data(Qt::DisplayRole).toString();
    }
    else
    {
        groupName = hSelectedItem.data(Qt::DisplayRole).toString();
    }

    FlareInfoArray::Props flareProps = FlareInfoArray::Get();
    QString itemName = MakeValidName(groupName + QString(".") + flareProps.p[flareType].name,
                            AZStd::bind(&CDatabaseFrameWnd::DoesItemExist, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
    CLensFlareItem* pNewItem = AddNewLensFlareItem(groupName, LensFlareUtil::GetShortName(itemName));

    if (pNewItem)
    {
        pNewItem->GetOptics()->AddElement(gEnv->pOpticsManager->Create(flareType));
        ReloadItems();
        SelectLensFlareItem(GetTreeLensFlareItem(pNewItem), {});
    }
}

void CLensFlareEditor::OnUpdateProperties([[maybe_unused]] IVariable* var)
{
    GetIEditor()->GetLensFlareManager()->Modified();
}

AssetSelectionModel CLensFlareEditor::GetAssetSelectionModel() const
{
    return AssetSelectionModel::AssetTypeSelection(AZ::AzTypeInfo<LmbrCentral::LensFlareAsset>::Uuid());
}

LensFlareItemTreeModel::LensFlareItemTreeModel(CDatabaseFrameWnd* pParent)
    : LibraryItemTreeModel(pParent)
{
}

QStringList LensFlareItemTreeModel::mimeTypes() const
{
    QStringList mimeTypes = LibraryItemTreeModel::mimeTypes();
    mimeTypes << QStringLiteral("application/x-lumberyard-flaretypes");
    return mimeTypes;
}

bool LensFlareItemTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (LibraryItemTreeModel::dropMimeData(data, action, row, column, parent))
    {
        return true;
    }

    if (data->hasFormat(QStringLiteral("application/x-lumberyard-flaretypes")))
    {
        QByteArray encoded = data->data(QStringLiteral("application/x-lumberyard-flaretypes"));
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
            int flareType;
            stream >> flareType;
            static_cast<CLensFlareEditor*>(m_dialog)->AddNewItemByAtomicOptics(parent, static_cast<EFlareType>(flareType));
        }

        return true;
    }

    return false;
}

Qt::ItemFlags LensFlareItemTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::ItemFlags();
    }

    return LibraryItemTreeModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
}

Qt::DropActions LensFlareItemTreeModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions LensFlareItemTreeModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

#include <LensFlareEditor/moc_LensFlareEditor.cpp>
