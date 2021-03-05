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

#include "DatabaseFrameWnd.h"

// Qt
#include <QComboBox>
#include <QMimeData>
#include <QTreeView>
#include <QItemSelectionModel>

// AzToolsFramework
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

// Editor
#include "BaseLibraryManager.h"
#include "BaseLibraryItem.h"
#include "StringDlg.h"
#include "QtViewPaneManager.h"
#include "Undo/Undo.h"

#include "ui_DatabaseFrameWnd.h"


class CUndoSelectLibraryUndo
    : public IUndoObject
{
public:
    CUndoSelectLibraryUndo(const QString& libraryName, const QString& wndClssName)
    {
        m_LibraryName = libraryName;
        m_WndClassName = wndClssName;
    }
    ~CUndoSelectLibraryUndo(){}

protected:
    int GetSize()
    {
        return sizeof(*this);
    }
    QString GetDescription() { return "Select database library."; };
    void Undo(bool bUndo)
    {
        SelectLibrary(bUndo);
    }
    void Redo()
    {
        SelectLibrary(true);
    }

private:

    void SelectLibrary(bool bUndo)
    {
        CDatabaseFrameWnd* pDatabaseEditor = FindViewPane<CDatabaseFrameWnd>(m_WndClassName);
        if (!pDatabaseEditor)
        {
            return;
        }

        QString libraryNameForUndo(m_LibraryName);
        if (bUndo)
        {
            m_LibraryName = pDatabaseEditor->GetSelectedLibraryName();
        }
        pDatabaseEditor->SelectLibrary(libraryNameForUndo);
    }

    QString m_LibraryName;
    QString m_WndClassName;
};

static const int LIBRARY_CB_WIDTH(150);

CDatabaseFrameWnd::CDatabaseFrameWnd(CBaseLibraryManager* pItemManager, QWidget* pParent)
    : AzQtComponents::DockMainWindow(pParent)
    , m_pItemManager(pItemManager)
    , ui(new Ui::DatabaseFrameWnd)
    , m_initialized(false)
    , m_pLibraryItemTreeModel(nullptr)
{
    ui->setupUi(this);
    m_sortRecursionType = SORT_RECURSION_FULL;
    m_pLibrary = NULL;
    m_bLibsLoaded = false;
    m_pCurrentItem = NULL;

    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetUndoManager()->AddListener(this);

    m_pLibraryListComboBox = new QComboBox;
    m_pLibraryListComboBox->setFixedSize(LIBRARY_CB_WIDTH, 16);
    ui->m_toolBar->insertWidget(ui->m_toolBar->actions()[4], m_pLibraryListComboBox);
    connect(m_pLibraryListComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CDatabaseFrameWnd::OnChangedLibrary);

    m_pLibraryListModel = new LibraryListModel(m_pItemManager, this);
    m_pLibraryListComboBox->setModel(m_pLibraryListModel);

    connect(ui->actionDBAddLib, &QAction::triggered, this, &CDatabaseFrameWnd::OnAddLibrary);
    connect(ui->actionDBDelLib, &QAction::triggered, this, &CDatabaseFrameWnd::OnRemoveLibrary);
    connect(ui->actionDBRemove, &QAction::triggered, this, &CDatabaseFrameWnd::OnRemoveItem);
    connect(ui->actionDBSave, &QAction::triggered, this, &CDatabaseFrameWnd::OnSave);
    connect(ui->actionDBLoadLib, &QAction::triggered, this, &CDatabaseFrameWnd::OnLoadLibrary);
    connect(ui->actionDBReload, &QAction::triggered, this, &CDatabaseFrameWnd::OnReloadLib);
    connect(ui->actionDBReloadLib, &QAction::triggered, this, &CDatabaseFrameWnd::OnReloadLib);
    connect(ui->actionDBCopy, &QAction::triggered, this, &CDatabaseFrameWnd::OnCopy);
    connect(ui->actionDBPaste, &QAction::triggered, this, &CDatabaseFrameWnd::OnPaste);
    connect(ui->actionDBClone, &QAction::triggered, this, &CDatabaseFrameWnd::OnClone);
    connect(ui->actionUndo, &QAction::triggered, this, &CDatabaseFrameWnd::OnUndo);
    connect(ui->actionRedo, &QAction::triggered, this, &CDatabaseFrameWnd::OnRedo);
}

CDatabaseFrameWnd::~CDatabaseFrameWnd()
{
    // Block Signals to prevent changes in the m_pLibraryListComboBox
    // from triggering OnChangedLibrary on teardown.
    m_pLibraryListComboBox->blockSignals(true);

    m_pLibraryListModel->clear();

    GetIEditor()->UnregisterNotifyListener(this);
    GetIEditor()->GetUndoManager()->RemoveListener(this);
}

void CDatabaseFrameWnd::InitTreeCtrl()
{
    QTreeView* treeView = GetTreeCtrl();

    connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CDatabaseFrameWnd::OnSelChangedItemTree);
    treeView->installEventFilter(this);
}

void CDatabaseFrameWnd::ReloadLibs()
{
    if (!m_pItemManager)
    {
        return;
    }

    SelectItem(0);

    m_pLibraryListModel->Reload();

    if (m_pLibraryListComboBox)
    {
        SelectLibrary(m_pLibraryListComboBox->itemData(0, Qt::UserRole).value<CBaseLibrary*>());
    }

    m_bLibsLoaded = true;
}

void CDatabaseFrameWnd::ReloadItems()
{
    SelectItem(0);
    m_selectedGroup = "";
    m_pCurrentItem = 0;
    m_cpoSelectedLibraryItems.clear();
    if (m_pItemManager)
    {
        m_pItemManager->SetSelectedItem(0);
    }
    ReleasePreviewControl();

    if (m_pLibrary && m_pLibraryItemTreeModel)
    {
        m_pLibraryItemTreeModel->Reload(m_pLibrary);
        GetTreeCtrl()->expandAll();
    }
}

void CDatabaseFrameWnd::SelectLibrary(const QString& library, bool bForceSelect)
{
    QWaitCursor wait;
    if (GetSelectedLibraryName() != library || bForceSelect)
    {
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoSelectLibraryUndo(GetSelectedLibraryName(), GetClassName()));
        }

        SelectItem(0);
        m_pLibrary = FindLibrary(library);
        ReloadItems();
    }
    if (m_pLibraryListComboBox)
    {
        m_pLibraryListComboBox->setCurrentIndex(GetComboBoxIndex(m_pLibrary));
    }
}

void CDatabaseFrameWnd::SelectLibrary(CBaseLibrary* pItem, bool bForceSelect)
{
    QWaitCursor wait;
    if (m_pLibrary != pItem || bForceSelect)
    {
        if (CUndo::IsRecording() && m_pLibrary)
        {
            CUndo::Record(new CUndoSelectLibraryUndo(GetSelectedLibraryName(), GetClassName()));
        }

        SelectItem(0);
        m_pLibrary = pItem;
        ReloadItems();
    }
    if (m_pLibraryListComboBox)
    {
        m_pLibraryListComboBox->setCurrentIndex(GetComboBoxIndex(pItem));
    }
}

void CDatabaseFrameWnd::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    if (item == m_pCurrentItem && !bForceReload)
    {
        return;
    }

    QModelIndex index = m_pLibraryItemTreeModel->index(item);
    if (index.isValid())
    {
        GetTreeCtrl()->expand(index.parent());
        GetTreeCtrl()->setCurrentIndex(index);
    }
    else
    {
        GetTreeCtrl()->selectionModel()->clearCurrentIndex();
    }
}

CBaseLibrary* CDatabaseFrameWnd::FindLibrary(const QString& libraryName)
{
    return (CBaseLibrary*)m_pItemManager->FindLibrary(libraryName);
}

int CDatabaseFrameWnd::GetComboBoxIndex(CBaseLibrary* pLibrary)
{
    return m_pLibraryListComboBox->findData(QVariant::fromValue<CBaseLibrary*>(pLibrary));
}

CBaseLibrary* CDatabaseFrameWnd::NewLibrary(const QString& libraryName)
{
    return (CBaseLibrary*)m_pItemManager->AddLibrary(libraryName, false);
}

void CDatabaseFrameWnd::DeleteLibrary(CBaseLibrary* pLibrary)
{
    m_pItemManager->DeleteLibrary(pLibrary->GetName());
}

void CDatabaseFrameWnd::DeleteItem(CBaseLibraryItem* pItem)
{
    m_pItemManager->DeleteItem(pItem);
}

void CDatabaseFrameWnd::OnAddLibrary()
{
    StringDlg dlg(tr("New Library Name"), this);
    CUndo undo(tr("Add Database Library").toUtf8());
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

void CDatabaseFrameWnd::OnRemoveLibrary()
{
    QString library = GetSelectedLibraryName();
    if (library.isEmpty())
    {
        return;
    }
    if (m_pLibrary->IsModified())
    {
        QString ask = tr("Save changes to the library %1?").arg(library);
        if (QMessageBox::question(this, tr("Editor"), ask) == QMessageBox::Yes)
        {
            OnSave();
        }
    }
    QString ask = tr("When removing library All items contained in this library will be deleted.\r\nAre you sure you want to remove libarary %1?\r\n(Note: Library file will not be deleted from the disk)").arg(library);
    if (QMessageBox::question(this, tr("Editor"), ask) == QMessageBox::Yes)
    {
        SelectItem(0);
        DeleteLibrary(m_pLibrary);
        m_pLibrary = 0;
        ReleasePreviewControl();
        ReloadLibs();
        GetIEditor()->SetModifiedFlag();
    }
}

void CDatabaseFrameWnd::OnAddItem()
{
}

void CDatabaseFrameWnd::OnRemoveItem()
{
    // When we have no set of selected items, it may be the case that we are
    // dealing with old fashioned selection. If such is the case, let's deal
    // with it using the old code system, which should be deprecated.
    if (m_cpoSelectedLibraryItems.empty())
    {
        if (m_pCurrentItem)
        {
            // Remove prototype from prototype manager and library.
            QString str = tr("Delete %1?").arg(m_pCurrentItem->GetName());
            if (QMessageBox::question(this, tr("Delete Confirmation"), str) == QMessageBox::Yes)
            {
                CUndo undo(tr("Remove library item").toUtf8());
                TSmartPtr<CBaseLibraryItem> pCurrent = m_pCurrentItem;
                DeleteItem(pCurrent);
                m_pLibraryItemTreeModel->Remove(m_pCurrentItem);
                GetIEditor()->SetModifiedFlag();
                SelectItem(0);
            }
        }
    }
    else // This is to be used when deleting multiple items...
    {
        QString strMessageString = tr("Delete the following items:\n");
        int nItemCount(0);
        std::set<CBaseLibraryItem*>::iterator   itCurrentIterator;
        std::set<CBaseLibraryItem*>::iterator   itEnd;

        itEnd = m_cpoSelectedLibraryItems.end();
        itCurrentIterator = m_cpoSelectedLibraryItems.begin();
        // For now, we have a maximum limit of 7 items per messagebox...
        for (nItemCount = 0; nItemCount < 7; ++nItemCount, itCurrentIterator++)
        {
            if (itCurrentIterator == itEnd)
            {
                // As there were less than 7 items selected, we got to put them all
                // into the formated string for the messagebox.
                break;
            }
            strMessageString += QStringLiteral("%1\n").arg((*itCurrentIterator)->GetName());
        }
        if (itCurrentIterator != itEnd)
        {
            strMessageString += QStringLiteral("...");
        }

        if (QMessageBox::question(this, tr("Delete Confirmation"), strMessageString) == QMessageBox::Yes)
        {
            for (itCurrentIterator = m_cpoSelectedLibraryItems.begin(); itCurrentIterator != itEnd; itCurrentIterator++)
            {
                CBaseLibraryItem* pCurrent = *itCurrentIterator;
                DeleteItem(pCurrent);
                m_pLibraryItemTreeModel->Remove(pCurrent);
            }
            m_cpoSelectedLibraryItems.clear();
            GetIEditor()->SetModifiedFlag();
            SelectItem(0);
        }
    }
}

void CDatabaseFrameWnd::OnRenameItem()
{
    if (m_pCurrentItem)
    {
        StringGroupDlg dlg;
        dlg.SetGroup(m_pCurrentItem->GetGroupName());
        dlg.SetString(m_pCurrentItem->GetShortName());
        if (dlg.exec() == QDialog::Accepted)
        {
            static bool warn = true;
            if (warn)
            {
                warn = false;
                QMessageBox::warning(this, tr("Warning"), tr("Levels referencing this archetype will need to be exported."));
            }

            CUndo undo(tr("Rename library item").toUtf8());
            TSmartPtr<CBaseLibraryItem> curItem = m_pCurrentItem;
            SetItemName(curItem, dlg.GetGroup(), dlg.GetString());
            ReloadItems();
            SelectItem(curItem, true);
            curItem->SetModified();
            //m_pCurrentItem->Update();
        }
        GetIEditor()->SetModifiedFlag();
    }
}

void CDatabaseFrameWnd::OnChangedLibrary()
{
    CBaseLibrary* pBaseLibrary = NULL;
    if (m_pLibraryListComboBox)
    {
        pBaseLibrary = m_pLibraryListComboBox->currentData(Qt::UserRole).value<CBaseLibrary*>();
    }

    if (pBaseLibrary && pBaseLibrary != m_pLibrary)
    {
        CUndo undo("Change database library");
        SelectLibrary(pBaseLibrary);
    }
}

void CDatabaseFrameWnd::OnExportLibrary()
{
    if (!m_pLibrary)
    {
        return;
    }

    QString filename;
    if (CFileUtil::SelectSaveFile("Library XML Files (*.xml)", "xml", (Path::GetEditingGameDataFolder() + "/Materials").c_str(), filename))
    {
        XmlNodeRef libNode = XmlHelpers::CreateXmlNode("MaterialLibrary");
        m_pLibrary->Serialize(libNode, false);
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), libNode, filename.toStdString().c_str());
    }
}

void CDatabaseFrameWnd::OnSave()
{
    if (m_bLibsLoaded)
    {
        m_pLibrary->SetModified(true);
        m_pItemManager->SaveAllLibs();
        m_pLibrary->SetModified(false);
    }
}

void CDatabaseFrameWnd::OnReloadLib()
{
    if (!m_pLibrary)
    {
        return;
    }

    QString libname = m_pLibrary->GetName();
    QString file = m_pLibrary->GetFilename();

    //If the file we want to reload does not exist (and isn't the level library), we can't reload it
    //Just display a warning to the user and stop the reload process
    if (!CFileUtil::Exists(file, false) && !m_pLibrary->IsLevelLibrary())
    {
        QString str = tr("Layer %1 does not exist on disk yet. Have you ever saved it?\nReloading layer is not possible!").
            arg(libname);
        if (QMessageBox::information(this, tr("Editor"), str))
        {
            return;
        }
    }

    if (m_pLibrary->IsModified())
    {
        QString str = tr("Layer %1 was modified.\nReloading layer will discard all modifications to this library!").
                arg(libname);
        if (QMessageBox::question(this, tr("Editor"), str) != QMessageBox::Yes)
        {
            return;
        }
    }

    //Don't try to delete/load level library from disk.
    //It's managed by the level and there doesn't seem to be a way to "reload" it
    if (!m_pLibrary->IsLevelLibrary())
    {
        m_pItemManager->DeleteLibrary(libname);
        m_pItemManager->LoadLibrary(file, true);
    }

    ReloadLibs();
    SelectLibrary(libname);
}

void CDatabaseFrameWnd::OnLoadLibrary()
{
    assert(m_pItemManager);
    CUndo undo(tr("Load Database Library").toUtf8());
    LoadLibrary();
}

void CDatabaseFrameWnd::LoadLibrary()
{
    AssetSelectionModel selection = GetAssetSelectionModel();

    AzToolsFramework::EditorRequests::Bus::Broadcast(
        &AzToolsFramework::EditorRequests::BrowseForAssets, 
        selection);
    if (selection.IsValid())
    {
        GetIEditor()->SuspendUndo();
        IDataBaseLibrary* matLib = m_pItemManager->LoadLibrary(Path::FullPathToGamePath(
            selection.GetResult()->GetFullPath().c_str()).c_str());
        GetIEditor()->ResumeUndo();
        ReloadLibs();
        if (matLib)
        {
            SelectLibrary((CBaseLibrary*)matLib);
        }
    }
}

bool CDatabaseFrameWnd::SetItemName(CBaseLibraryItem* item, const QString& groupName, const QString& itemName)
{
    assert(item);
    // Make prototype name.
    QString name;
    if (!groupName.isEmpty())
    {
        name = groupName + ".";
    }
    name += itemName;
    QString fullName = name;
    if (item->GetLibrary())
    {
        fullName = item->GetLibrary()->GetName() + "." + name;
    }
    IDataBaseItem* pOtherItem = m_pItemManager->FindItemByName(fullName);
    if (pOtherItem && pOtherItem != item)
    {
        // Ensure uniqness of name.
        Warning("Duplicate Item Name %s", name.toUtf8().data());
        return false;
    }
    else
    {
        item->SetName(name);
    }
    return true;
}

void CDatabaseFrameWnd::OnSelChangedItemTree(const QModelIndex& index)
{
    CBaseLibraryItem* item = index.data(Qt::UserRole).value<CBaseLibraryItem*>();

    if (item)
    {
        if (item->GetLibrary() != m_pLibrary && item->GetLibrary())
        {
            SelectLibrary((CBaseLibrary*)item->GetLibrary());
        }
    }

    m_pCurrentItem = item;

    if (item)
    {
        m_selectedGroup = item->GetGroupName();
    }
    else
    {
        m_selectedGroup = "";
    }

    m_pItemManager->SetSelectedItem(item);
}

bool CDatabaseFrameWnd::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress || watched != GetTreeCtrl())
    {
        return QMainWindow::eventFilter(watched, event);
    }

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    bool bCtrl = keyEvent->modifiers() == Qt::ControlModifier;

    if (keyEvent->matches(QKeySequence::Copy))
    {
        OnCopy(); // Ctrl+C
    }
    else if (keyEvent->matches(QKeySequence::Paste))
    {
        OnPaste(); // Ctrl+V
    }
    else if (keyEvent->matches(QKeySequence::Cut))
    {
        OnCut(); // Ctrl+X
    }
    else if (bCtrl && keyEvent->key() == Qt::Key_D)
    {
        OnClone(); // Ctrl+D
    }
    else if (keyEvent->matches(QKeySequence::Delete))
    {
        OnRemoveItem();
    }
    else if (keyEvent->key() == Qt::Key_F2)
    {
        OnRenameItem();
    }
    else if (keyEvent->key() == Qt::Key_Insert)
    {
        OnAddItem();
    }
    else
    {
        return QMainWindow::eventFilter(watched, event);
    }

    return true;
}

void CDatabaseFrameWnd::OnUndo()
{
    GetIEditor()->Undo();
}

void CDatabaseFrameWnd::OnRedo()
{
    GetIEditor()->Redo();
}

void CDatabaseFrameWnd::OnCut()
{
    if (m_pCurrentItem)
    {
        OnCopy();
        OnRemoveItem();
    }
}

void CDatabaseFrameWnd::OnClone()
{
    OnCopy();
    OnPaste();
}

void CDatabaseFrameWnd::DoesItemExist(const QString& itemName, bool& bOutExist) const
{
    for (int i = 0, iItemCount(m_pLibrary->GetItemCount()); i < iItemCount; ++i)
    {
        IDataBaseItem* pItem = m_pLibrary->GetItem(i);
        if (pItem == NULL)
        {
            continue;
        }
        if (pItem->GetName() == itemName)
        {
            bOutExist = true;
            return;
        }
    }
    bOutExist = false;
}

QString CDatabaseFrameWnd::MakeValidName(const QString& candidateName, Functor2<const QString&, bool&> FuncForCheck) const
{
    bool bCheck = false;
    FuncForCheck(candidateName, bCheck);
    if (!bCheck)
    {
        return candidateName;
    }

    int nCounter = 0;
    const int nEnoughBigNumber = 1000000;
    do
    {
        QString counterBuffer;
        counterBuffer = QString::number(nCounter);
        QString newName = candidateName + counterBuffer;
        FuncForCheck(newName, bCheck);
        if (!bCheck)
        {
            return newName;
        }
    } while (nCounter++ < nEnoughBigNumber);

    assert(0 && "CDatabaseFrameWnd::MakeValidName()");
    return candidateName;
}

void CDatabaseFrameWnd::DoesGroupExist(const QString& groupName, bool& bOutExist) const
{
    bOutExist = m_pLibraryItemTreeModel->DoesGroupExist(groupName);
}

QString CDatabaseFrameWnd::GetSelectedLibraryName() const
{
    return m_pLibrary ? m_pLibrary->GetName() : "";
}

void CDatabaseFrameWnd::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
        m_bLibsLoaded = false;
        // Clear all prototypes and libraries.
        SelectItem(0);
        if (m_pLibraryListComboBox)
        {
            m_pLibraryListComboBox->clear();
        }
        m_pLibrary = nullptr;
        break;

    case eNotify_OnEndSceneOpen:
        m_bLibsLoaded = false;
        ReloadLibs();
        break;

    case eNotify_OnCloseScene:
    {
        m_bLibsLoaded = false;
        CUndo undo(tr("Close Database Library").toUtf8());       
        GetTreeCtrl()->selectionModel()->clear();
        GetTreeCtrl()->clearSelection();
        m_pLibraryItemTreeModel->Clear();
        m_pCurrentItem = nullptr;
        m_cpoSelectedLibraryItems.clear();
    }
    break;

    case eNotify_OnDataBaseUpdate:
        if (m_pLibrary && m_pLibrary->IsModified())
        {
            ReloadItems();
        }
        break;
    }
}

void CDatabaseFrameWnd::SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo)
{
    ui->actionUndo->setEnabled(numUndo > 0);
    ui->actionRedo->setEnabled(numRedo > 0);
}

void CDatabaseFrameWnd::showEvent(QShowEvent* event)
{
    if (!m_initialized)
    {
        OnInitDialog();
        m_initialized = true;
    }

    QMainWindow::showEvent(event);
}

LibraryListModel::LibraryListModel(CBaseLibraryManager* itemManager, QObject* pParent)
    : QAbstractListModel(pParent)
    , m_pItemManager(itemManager)
{
}

int LibraryListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_pItemManager->GetLibraryCount();
}

QVariant LibraryListModel::data(const QModelIndex& index, int role) const
{
    const int row = index.row();

    if (row < 0 || row >= m_pItemManager->GetLibraryCount())
    {
        return {};
    }

    auto library = static_cast<CBaseLibrary*>(m_pItemManager->GetLibrary(row));

    switch (role)
    {
    case Qt::DisplayRole:
    {
        QString name = library->GetName();
        if (library->IsModified())
        {
            name += "*";
        }
        return name;
    }

    case Qt::UserRole:
        return QVariant::fromValue<CBaseLibrary*>(library);
    }

    return {};
}

void LibraryListModel::Reload()
{
    beginResetModel();

    const int count = m_pItemManager->GetLibraryCount();

    for (int i = 0; i < count; i++)
    {
        CBaseLibrary* pLibrary = static_cast<CBaseLibrary*>(m_pItemManager->GetLibrary(i));
        connect(pLibrary, &CBaseLibrary::Modified, this, &LibraryListModel::LibraryModified);
    }

    endResetModel();
}

void LibraryListModel::clear()
{
    beginResetModel();
    endResetModel();
}

void LibraryListModel::LibraryModified(bool)
{
    CBaseLibrary* library = qobject_cast<CBaseLibrary*>(sender());

    const int count = m_pItemManager->GetLibraryCount();

    for (int i = 0; i < count; i++)
    {
        CBaseLibrary* pLibrary = static_cast<CBaseLibrary*>(m_pItemManager->GetLibrary(i));
        if (pLibrary == library)
        {
            emit dataChanged(index(i, 0), index(i, 0));
        }
    }
}

LibraryItemTreeModel::LibraryItemTreeModel(CDatabaseFrameWnd* pParent)
    : QAbstractItemModel(pParent)
    , m_dialog(pParent)
{
}

Qt::ItemFlags LibraryItemTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::ItemFlags();
    }

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    flags |= Qt::ItemIsEditable;

    if (!index.internalPointer())
    {
        flags |= Qt::ItemIsDropEnabled;
    }
    else
    {
        flags |= Qt::ItemIsDragEnabled;
    }

    return flags;
}

int LibraryItemTreeModel::columnCount(const QModelIndex&) const
{
    return 1;
}

int LibraryItemTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
    {
        return m_groups.size();
    }
    else
    {
        if (!parent.internalPointer() && parent.row() >= 0 && parent.row() < m_groups.size())
        {
            auto entry = std::begin(m_groups);
            std::advance(entry, parent.row());
            return entry->second->second.size();
        }
    }

    return 0;
}

QModelIndex LibraryItemTreeModel::parent(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
    {
        auto group = static_cast<Group*>(index.internalPointer());
        auto entry = m_groups.find(group->first);
        return createIndex(std::distance(std::begin(m_groups), entry), 0, nullptr);
    }

    return {};
}

QModelIndex LibraryItemTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column > 0)
    {
        return {};
    }

    if (!parent.isValid())
    {
        if (row >= 0 && row < m_groups.size())
        {
            return createIndex(row, 0, nullptr);
        }
    }
    else
    {
        if (!parent.internalPointer() && parent.row() >= 0 && parent.row() < m_groups.size())
        {
            auto entry = std::begin(m_groups);
            std::advance(entry, parent.row());

            if (row >= 0 && row < entry->second->second.size())
            {
                return createIndex(row, 0, const_cast<Group*>(entry->second.get()));
            }
        }
    }

    return {};
}

QVariant LibraryItemTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return {};
    }

    if (!index.internalPointer())
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            auto entry = std::begin(m_groups);
            std::advance(entry, index.row());
            return entry->first;
        }
        case Qt::UserRole:
            return QVariant::fromValue<CBaseLibraryItem*>(nullptr);
        }
    }
    else
    {
        auto group = static_cast<Group*>(index.internalPointer());
        auto item = group->second[index.row()];

        switch (role)
        {
        case Qt::DisplayRole:
            return item->GetShortName();
        case Qt::UserRole:
            return QVariant::fromValue<CBaseLibraryItem*>(item);
        }
    }

    return {};
}

void LibraryItemTreeModel::RenameItem(CBaseLibraryItem* item, const QString& fullName)
{
    QString prevFullName = item->GetFullName();
    item->SetName(fullName);
    item->SetModified();
    emit itemRenamed(item, prevFullName);
}

bool LibraryItemTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
    {
        return false;
    }

    QString text = value.toString();

    if (text.isEmpty())
    {
        return false;
    }

    if (text.contains(QLatin1Char('.')))
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), tr("Warning"), tr("The name must not contain \".\""));
        return false;
    }

    if (!index.internalPointer())
    {
        // rename group

        auto sourceEntry = std::begin(m_groups);
        std::advance(sourceEntry, index.row());

        if (text == sourceEntry->first)
        {
            return false;
        }

        if (m_groups.find(text) != std::end(m_groups))
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), tr("Warning"), tr("The identical name exists."));
            return false;
        }

        int sourceRow = index.row();
        int targetRow = std::distance(std::begin(m_groups), m_groups.lower_bound(text));

        if (targetRow < sourceRow || targetRow > sourceRow + 1)
        {
            beginMoveRows({}, sourceRow, sourceRow, {}, targetRow);
        }

        auto targetEntry = m_groups.insert({ text, sourceEntry->second }).first;
        targetEntry->second->first = text;
        m_groups.erase(sourceEntry);

        if (targetRow < sourceRow || targetRow > sourceRow + 1)
        {
            endMoveRows();
        }
        else
        {
            emit dataChanged(index, index);
        }

        CUndo undo(tr("Rename FlareGroupItem").toUtf8());

        auto group = targetEntry->second.get();

        for (int row = 0; row < group->second.size(); row++)
        {
            auto item = group->second[row];
            QString name = QString("%1.%2").arg(text).arg(item->GetShortName());
            RenameItem(item, name);
        }
    }
    else
    {
        // rename single item

        auto group = static_cast<Group*>(index.internalPointer());
        auto& items = group->second;

        auto item = items[index.row()];

        if (text == item->GetShortName())
        {
            return false;
        }

        QString name = QString("%1.%2").arg(item->GetGroupName(), text);

        if (std::find_if(std::begin(items), std::end(items),
                [&](const CBaseLibraryItem* item)
                {
                    return item->GetName() == name;
                }) != std::end(items))
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), tr("Warning"), tr("The identical name exists."));
            return false;
        }

        CUndo undo(tr("Rename FlareGroupItem").toUtf8());
        RenameItem(item, name);

        emit dataChanged(index, index);
    }

    return true;
}

QModelIndex LibraryItemTreeModel::index(CBaseLibraryItem* item) const
{
    if (!item)
    {
        return {};
    }

    auto entry = m_groups.find(item->GetGroupName());
    if (entry == std::end(m_groups))
    {
        return {};
    }

    auto& items = entry->second->second;

    auto it = std::find(std::begin(items), std::end(items), item);
    if (it == std::end(items))
    {
        return {};
    }

    return createIndex(std::distance(std::begin(items), it), 0, const_cast<Group*>(entry->second.get()));
}

bool LibraryItemTreeModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (!parent.isValid())
    {
        beginRemoveRows({}, row, row + count - 1);

        auto first = std::begin(m_groups);
        std::advance(first, row);

        auto last = std::begin(m_groups);
        std::advance(last, row + count);

        m_groups.erase(first, last);

        endRemoveRows();
    }
    else
    {
        beginRemoveRows(parent, row, row + count - 1);

        auto entry = std::begin(m_groups);
        std::advance(entry, parent.row());

        auto& items = entry->second->second;
        items.erase(std::begin(items) + row, std::begin(items) + row + count);

        endRemoveRows();

        if (items.empty())
        {
            beginRemoveRows({}, parent.row(), parent.row());
            m_groups.erase(entry);
            endRemoveRows();
        }
    }

    return true;
}

static bool LibraryItemLess(const CBaseLibraryItem* left, const CBaseLibraryItem* right)
{
    QString leftName = left->GetName();
    QString rightName = right->GetName();
    return leftName.compare(rightName) < 0;
}

void LibraryItemTreeModel::Clear()
{
    beginResetModel();

    m_groups.clear();

    endResetModel();
}

void LibraryItemTreeModel::Reload(CBaseLibrary* pLibrary)
{
    beginResetModel();

    m_groups.clear();

    for (int i = 0; i < pLibrary->GetItemCount(); i++)
    {
        auto item = static_cast<CBaseLibraryItem*>(pLibrary->GetItem(i));

        QString itemName = item->GetName();
        QString groupName = item->GetGroupName();

        auto entry = m_groups.find(groupName);

        if (entry == std::end(m_groups))
        {
            entry = m_groups.insert({ groupName, std::make_shared<Group>(groupName, std::vector<CBaseLibraryItem*>()) }).first;
        }

        auto& items = entry->second->second;
        items.insert(std::upper_bound(std::begin(items), std::end(items), item, LibraryItemLess), item);
    }

    endResetModel();
}

QString LibraryItemTreeModel::GetFullName(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QString();
    }

    if (!index.internalPointer())
    {
        auto entry = std::begin(m_groups);
        std::advance(entry, index.row());
        return entry->first;
    }
    else
    {
        auto group = static_cast<Group*>(index.internalPointer());
        auto item = group->second[index.row()];
        return item->GetName();
    }
}

void LibraryItemTreeModel::Add(CBaseLibraryItem* item)
{
    QString groupName = item->GetGroupName();

    auto entry = m_groups.find(groupName);
    if (entry == std::end(m_groups))
    {
        int row = std::distance(std::begin(m_groups), m_groups.lower_bound(groupName));

        emit beginInsertRows({}, row, row);
        entry = m_groups.insert({ groupName, std::make_shared<Group>(groupName, std::vector<CBaseLibraryItem*>()) }).first;
        emit endInsertRows();
    }

    QModelIndex parentIndex = createIndex(std::distance(std::begin(m_groups), entry), 0, nullptr);
    auto& items = entry->second->second;

    if (std::find(std::begin(items), std::end(items), item) == std::end(items))
    {
        auto it = std::upper_bound(std::begin(items), std::end(items), item, LibraryItemLess);
        int row = std::distance(std::begin(items), it);

        emit beginInsertRows(parentIndex, row, row);
        items.insert(it, item);
        emit endInsertRows();
    }
}

std::vector<CBaseLibraryItem*> LibraryItemTreeModel::ChildItems(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return {};
    }

    if (!index.internalPointer())
    {
        auto entry = std::begin(m_groups);
        std::advance(entry, index.row());
        return entry->second->second;
    }
    else
    {
        auto group = static_cast<Group*>(index.internalPointer());
        return {
                   group->second[index.row()]
        };
    }
}

bool LibraryItemTreeModel::Remove(CBaseLibraryItem* pItem)
{
    QModelIndex itemIndex = index(pItem);
    if (!itemIndex.isValid())
    {
        return false;
    }
    return removeRow(itemIndex.row(), itemIndex.parent());
}

void LibraryItemTreeModel::Rename(CBaseLibraryItem* item, const QString& groupName, const QString& shortName)
{
    if (groupName != item->GetGroupName())
    {
        auto entry = m_groups.find(groupName);

        if (entry == std::end(m_groups))
        {
            int row = std::distance(std::begin(m_groups), m_groups.lower_bound(groupName));

            emit beginInsertRows({}, row, row);
            entry = m_groups.insert({ groupName, std::make_shared<Group>(groupName, std::vector<CBaseLibraryItem*>()) }).first;
            emit endInsertRows();
        }

        MoveItem(item, createIndex(std::distance(std::begin(m_groups), entry), 0, nullptr));
    }

    QString name = QString("%1.%2").arg(groupName).arg(shortName);
    RenameItem(item, name);

    QModelIndex itemIndex = index(item);
    emit dataChanged(itemIndex, itemIndex);
}

QModelIndex LibraryItemTreeModel::FindLibraryItemByFullName(const QString& fullName) const
{
    for (auto& entry : m_groups)
    {
        auto& group = *entry.second;
        auto& items = group.second;

        auto it = std::find_if(std::begin(items), std::end(items),
                [&](CBaseLibraryItem* item)
                {
                    return item->GetName() == fullName;
                });

        if (it != std::end(items))
        {
            return createIndex(std::distance(std::begin(items), it), 0, &group);
        }
    }

    return {};
}

bool LibraryItemTreeModel::DoesGroupExist([[maybe_unused]] const QString& groupName) const
{
#if 0
    auto it = std::find_if(
            std::begin(m_root->m_children),
            std::end(m_root->m_children),
            [&](const TreeNode* node)
            {
                return node->m_text.compare(groupName, Qt::CaseInsensitive) == 0;
            });

    return it != std::end(m_root->m_children);
#else
    return false;
#endif
}

QStringList LibraryItemTreeModel::mimeTypes() const
{
    QStringList mimeTypes;
    mimeTypes << QStringLiteral("application/x-lumberyard-libraryitems");
    return mimeTypes;
}

QString LibraryItemTreeModel::MakeValidName(const Group& group, const QString& baseName) const
{
    auto& items = group.second;

    QString name = baseName;

    int counter = 0;

    while (std::find_if(std::begin(items), std::end(items),
               [&](CBaseLibraryItem* item)
               {
                   return item->GetShortName() == name;
               }) != std::end(items))
    {
        name = QString("%1%2").arg(baseName).arg(counter);
        ++counter;
    }

    return name;
}

bool LibraryItemTreeModel::MoveItem(CBaseLibraryItem* item, const QModelIndex& targetParent)
{
    if (!targetParent.isValid() || targetParent.internalPointer())
    {
        return false;
    }

    auto sourceEntry = m_groups.find(item->GetGroupName());

    auto targetEntry = std::begin(m_groups);
    std::advance(targetEntry, targetParent.row());

    if (sourceEntry == targetEntry)
    {
        return false;
    }

    auto& sourceItems = sourceEntry->second->second;
    auto sourceIt = std::find(std::begin(sourceItems), std::end(sourceItems), item);
    int sourceRow = std::distance(std::begin(sourceItems), sourceIt);

    auto& targetItems = targetEntry->second->second;
    auto targetIt = std::upper_bound(
            std::begin(targetItems),
            std::end(targetItems),
            item,
            [](CBaseLibraryItem* left, CBaseLibraryItem* right)
            {
                QString leftShortName = left->GetShortName();
                QString rightShortName = right->GetShortName();
                return leftShortName.compare(rightShortName) < 0;
            });
    int targetRow = std::distance(std::begin(targetItems), targetIt);

    QModelIndex sourceParent = createIndex(std::distance(std::begin(m_groups), sourceEntry), 0, nullptr);

    beginMoveRows(sourceParent, sourceRow, sourceRow, targetParent, targetRow);
    targetItems.insert(targetIt, item);
    sourceItems.erase(sourceIt);
    endMoveRows();

    if (sourceItems.empty())
    {
        removeRow(sourceParent.row(), {});
    }

    return true;
}

bool LibraryItemTreeModel::dropMimeData(const QMimeData* data, [[maybe_unused]] Qt::DropAction action, [[maybe_unused]] int row, [[maybe_unused]] int column, const QModelIndex& index)
{
    if (data->hasFormat("application/x-lumberyard-libraryitems"))
    {
        QByteArray array = data->data("application/x-lumberyard-libraryitems");

        QModelIndex targetParent = index;

        if (!targetParent.isValid())
        {
            return false;
        }

        if (targetParent.internalPointer())
        {
            targetParent = parent(targetParent);
        }

        int count = array.size() / sizeof(CBaseLibraryItem*);
        auto items = reinterpret_cast<CBaseLibraryItem**>(array.data());

        auto entry = std::begin(m_groups);
        std::advance(entry, targetParent.row());

        CUndo undo(tr("Copy/Cut & Paste for Lens Flare").toUtf8());

        for (int i = 0; i < count; i++)
        {
            auto item = items[i];

            QString shortName = MakeValidName(*entry->second, item->GetShortName());

            if (MoveItem(item, targetParent))
            {
                RenameItem(item, QString("%1.%2").arg(entry->first).arg(shortName));
            }
        }

        return true;
    }

    return false;
}

QMimeData* LibraryItemTreeModel::mimeData(const QModelIndexList& indexes) const
{
    QByteArray array;

    for (auto& index : indexes)
    {
        auto item = static_cast<CBaseLibraryItem*>(index.data(Qt::UserRole).value<CBaseLibraryItem*>());

        if (item)
        {
            array.append(reinterpret_cast<const char*>(&item), sizeof(CBaseLibraryItem*));
        }
    }

    if (array.isEmpty())
    {
        return nullptr;
    }

    QMimeData* data = new QMimeData;
    data->setData(QStringLiteral("application/x-lumberyard-libraryitems"), array);
    return data;
}

Qt::DropActions LibraryItemTreeModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions LibraryItemTreeModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

#include <moc_DatabaseFrameWnd.cpp>
