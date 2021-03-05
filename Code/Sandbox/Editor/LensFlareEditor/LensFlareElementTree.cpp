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

#include "LensFlareElementTree.h"

// Qt
#include <QMimeData>
#include <QMenu>
#include <QMessageBox>

// Editor
#include "Clipboard.h"
#include "LensFlareItem.h"
#include "LensFlareManager.h"
#include "LensFlareUndo.h"
#include "LensFlareEditor.h"
#include "LensFlareLibrary.h"


template <typename Predicate>
static CLensFlareElement* FindLensFlareElement(CLensFlareElement* pRoot, Predicate pred)
{
    if (pred(pRoot))
    {
        return pRoot;
    }

    for (int i = 0, childCount = pRoot->GetChildCount(); i < childCount; i++)
    {
        if (CLensFlareElement* pElement = FindLensFlareElement(pRoot->GetChildAt(i), pred))
        {
            return pElement;
        }
    }

    return nullptr;
}

CLensFlareElementTree::CLensFlareElementTree(QWidget* pParent)
    : QTreeView(pParent)
    , m_model(new LensFlareElementTreeModel(this))
{
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(DragDrop);

    setModel(m_model.data());

    connect(this, &QWidget::customContextMenuRequested, this, &CLensFlareElementTree::OnNotifyTreeRClick);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &CLensFlareElementTree::OnTvnSelchangedTree);
    connect(model(), &QAbstractItemModel::dataChanged, this, &CLensFlareElementTree::OnDataChanged);
    connect(model(), &QAbstractItemModel::rowsInserted, this, &CLensFlareElementTree::OnRowsInserted);
    connect(model(), &QAbstractItemModel::rowsRemoved, this, &CLensFlareElementTree::OnRowsRemoved);
    connect(model(), &QAbstractItemModel::modelReset, this, &QTreeView::expandAll);
}

void CLensFlareElementTree::OnInternalVariableChange(IVariable* pVar)
{
    CLensFlareElement::LensFlareElementPtr pCurrentElement = GetCurrentLensFlareElement();
    if (pCurrentElement == NULL)
    {
        return;
    }
    pCurrentElement->OnInternalVariableChange(pVar);
}

CLensFlareElement* CLensFlareElementTree::GetCurrentLensFlareElement() const
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return nullptr;
    }
    return selected.first().data(Qt::UserRole).value<CLensFlareElement*>();
}

void CLensFlareElementTree::OnNotifyTreeRClick()
{
    CLensFlareElement* pElement = GetCurrentLensFlareElement();

    // Copy, Cut and Clone can't be done about a Root item so those menus should be disable when the selected item is a root item.
    bool bGrayed = false;
    if (!pElement || pElement->GetOpticsType() == eFT_Root)
    {
        bGrayed = true;
    }

    QMenu menu;

    QAction* actionDBFlareAddGroup = menu.addAction(tr("Add Group"));
    connect(actionDBFlareAddGroup, &QAction::triggered, this, &CLensFlareElementTree::OnAddGroup);

    menu.addSeparator();

    QAction* actionDBFlareCopy = menu.addAction(tr("Copy"));
    connect(actionDBFlareCopy, &QAction::triggered, this, &CLensFlareElementTree::OnCopy);

    QAction* actionDBFlareCut = menu.addAction(tr("Cut"));
    actionDBFlareCut->setEnabled(!bGrayed);
    connect(actionDBFlareCut, &QAction::triggered, this, &CLensFlareElementTree::OnCut);

    QAction* actionDBFlarePaste = menu.addAction(tr("Paste"));
    connect(actionDBFlarePaste, &QAction::triggered, this, &CLensFlareElementTree::OnPaste);

    QAction* actionDBFlareClone = menu.addAction(tr("Clone"));
    connect(actionDBFlareClone, &QAction::triggered, this, &CLensFlareElementTree::OnClone);

    menu.addSeparator();

    QAction* actionDBFlareRename = menu.addAction(tr("Rename\tF2"));
    connect(actionDBFlareRename, &QAction::triggered, this, &CLensFlareElementTree::OnRenameItem);

    QAction* actionDBFlareRemove = menu.addAction(tr("Delete\tDel"));
    connect(actionDBFlareRemove, &QAction::triggered, this, &CLensFlareElementTree::OnRemoveItem);

    QAction* actionDBFlareRemoveAll = menu.addAction(tr("Delete All"));
    connect(actionDBFlareRemoveAll, &QAction::triggered, this, &CLensFlareElementTree::OnRemoveAll);

    menu.addSeparator();

    QAction* actionDBFlareItemUp = menu.addAction(tr("Up"));
    connect(actionDBFlareItemUp, &QAction::triggered, this, &CLensFlareElementTree::OnItemUp);

    QAction* actionDBFlareItemDown = menu.addAction(tr("Down"));
    connect(actionDBFlareItemDown, &QAction::triggered, this, &CLensFlareElementTree::OnItemDown);

    menu.exec(QCursor::pos());
}

void CLensFlareElementTree::OnTvnSelchangedTree([[maybe_unused]] const QItemSelection& selected, const QItemSelection& deselected)
{
    QModelIndexList deselectedIndices = deselected.indexes();
    if (!deselectedIndices.isEmpty())
    {
        CLensFlareElement* pPrevLensFlareElement = deselectedIndices.first().data(Qt::UserRole).value<CLensFlareElement*>();
        ElementChanged(pPrevLensFlareElement);
    }

    CallChangeListeners();
}

void CLensFlareElementTree::ElementChanged(CLensFlareElement* pPrevLensFlareElement)
{
    if (!pPrevLensFlareElement)
    {
        return;
    }
    QString itemName;
    if (pPrevLensFlareElement->GetName(itemName))
    {
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoLensFlareElementSelection(m_model->GetLensFlareItem(), itemName));
        }
    }
}

void CLensFlareElementTree::OnDataChanged([[maybe_unused]] const QModelIndex& index)
{
    CallChangeListeners();
}

void CLensFlareElementTree::OnRowsInserted(const QModelIndex& parent, int first, [[maybe_unused]] int last)
{
    if (parent.isValid())
    {
        expand(parent);
        SelectItem(parent.model()->index(first, 0, parent));
        CallChangeListeners();
    }
}

void CLensFlareElementTree::OnRowsRemoved(const QModelIndex& parent, int first, [[maybe_unused]] int last)
{
    if (parent.isValid())
    {
        SelectItem(parent.model()->index(first, 0, parent));
        CallChangeListeners();
    }
}

void CLensFlareElementTree::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F2)
    {
        OnRenameItem();
        event->accept();
        return;
    }
    else if (event->matches(QKeySequence::Delete))
    {
        OnRemoveItem();
        event->accept();
        return;
    }

    QTreeView::keyPressEvent(event);
}

XmlNodeRef CLensFlareElementTree::CreateXML(const char* type) const
{
    std::vector<LensFlareUtil::SClipboardData> clipboardData;
    if (GetClipboardList(type, clipboardData))
    {
        return LensFlareUtil::CreateXMLFromClipboardData(type, "", false, clipboardData);
    }
    return NULL;
}

void CLensFlareElementTree::UpdateClipboard(const char* type, bool bPasteAtSameLevel)
{
    std::vector<LensFlareUtil::SClipboardData> clipboardData;
    if (GetClipboardList(type, clipboardData))
    {
        LensFlareUtil::UpdateClipboard(type, "", bPasteAtSameLevel, clipboardData);
    }
}

bool CLensFlareElementTree::GetClipboardList([[maybe_unused]] const char* type, std::vector<LensFlareUtil::SClipboardData>& outList) const
{
    CLensFlareElement::LensFlareElementPtr pElement = GetCurrentLensFlareElement();
    if (pElement == NULL)
    {
        return false;
    }
    QString name;
    if (!pElement->GetName(name))
    {
        return false;
    }

    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return false;
    }

    CLensFlareItem* pLensFlareItem = pEditor->GetSelectedLensFlareItem();
    if (pLensFlareItem == NULL)
    {
        return false;
    }

    outList.push_back(LensFlareUtil::SClipboardData(LENSFLARE_ELEMENT_TREE, pLensFlareItem->GetFullName(), name));

    return true;
}

void CLensFlareElementTree::OnAddGroup()
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }

    CUndo undo(tr("Add library item").toUtf8());
    m_model->AddElement(selected.first(), 0, eFT_Group);
    GetIEditor()->GetLensFlareManager()->Modified();
}

void CLensFlareElementTree::OnCopy()
{
    UpdateClipboard(FLARECLIPBOARDTYPE_COPY, false);
}

void CLensFlareElementTree::OnCut()
{
    Cut(false);
}

void CLensFlareElementTree::Cut(bool bPasteAtSameLevel)
{
    UpdateClipboard(FLARECLIPBOARDTYPE_CUT, bPasteAtSameLevel);
}

CLensFlareItem* CLensFlareElementTree::GetLensFlareItem() const
{
    return m_model->GetLensFlareItem();
}

void CLensFlareElementTree::OnPaste()
{
    CClipboard clipboard(this);
    if (clipboard.IsEmpty())
    {
        return;
    }

    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }

    XmlNodeRef xmlNode = clipboard.Get();
    if (xmlNode == NULL)
    {
        return;
    }

    CUndo undo(tr("Copy/Cut & Paste library item").toUtf8());
    m_model->Paste(selected.first(), xmlNode);
}

void CLensFlareElementTree::OnClone()
{
    OnCopy();
    OnPaste();
}

void CLensFlareElementTree::OnRenameItem()
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }
    edit(selected.first());
}

void CLensFlareElementTree::OnRemoveItem()
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }

    QModelIndex index = selected.first();

    // A root item must not be removed.
    if (!index.parent().isValid())
    {
        return;
    }

    CLensFlareElement::LensFlareElementPtr pCurrentElement = m_model->GetLensFlareElement(index);
    if (pCurrentElement == NULL)
    {
        return;
    }

    QString name;
    pCurrentElement->GetName(name);

    QString str = tr("Delete %1?").arg(name);
    if (QMessageBox::question(this, tr("Delete Confirmation"), str) == QMessageBox::Yes)
    {
        CUndo undo(tr("Remove an optics element").toUtf8());
        m_model->removeRow(index.row(), index.parent());
    }
}

void CLensFlareElementTree::OnRemoveAll()
{
    CUndo undo(tr("Remove All in FlareTreeCtrl").toUtf8());

    if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Do you want delete all?")) == QMessageBox::Yes)
    {
        m_model->RemoveAllElements();
        GetIEditor()->GetLensFlareManager()->Modified();
    }
}

void CLensFlareElementTree::OnItemUp()
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }

    QModelIndex index = selected.first();
    int row = index.row();
    if (row == 0)
    {
        return;
    }

    CUndo undo(tr("Copy/Cut & Paste library item").toUtf8());

    CLensFlareElement* pElement = index.data(Qt::UserRole).value<CLensFlareElement*>();
    m_model->MoveElement(pElement, row - 1, index.parent());
}

void CLensFlareElementTree::OnItemDown()
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }

    QModelIndex index = selected.first();
    int row = index.row();
    if (row == m_model->rowCount(index.parent()) - 1)
    {
        return;
    }

    CUndo undo(tr("Copy/Cut & Paste library item").toUtf8());

    CLensFlareElement* pElement = index.data(Qt::UserRole).value<CLensFlareElement*>();
    m_model->MoveElement(pElement, row + 2, index.parent());
}

void CLensFlareElementTree::SelectItem(const QModelIndex& index)
{
    if (index.isValid())
    {
        expand(index.parent());
        selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    }
    else
    {
        selectionModel()->clear();
    }
}

void CLensFlareElementTree::SelectTreeItemByName(const QString& name)
{
    SelectItem(m_model->GetTreeItemByName(name));
}

void CLensFlareElementTree::CallChangeListeners()
{
    CLensFlareElement* pElement = GetCurrentLensFlareElement();
    for (int i = 0, iSize(m_LensFlaresElementListeners.size()); i < iSize; ++i)
    {
        m_LensFlaresElementListeners[i]->OnLensFlareChangeElement(pElement);
    }
}

void CLensFlareElementTree::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        CUndo undo(tr("Changed Lens flares element").toUtf8());
        QTreeView::mousePressEvent(event);
    }
    else
    {
        QTreeView::mousePressEvent(event);
    }
}

void CLensFlareElementTree::InvalidateLensFlareItem()
{
    m_model->InvalidateLensFlareItem();
}

LensFlareElementTreeModel::LensFlareElementTreeModel(QObject* pParent)
    : QAbstractItemModel(pParent)
    , m_pRootElement(new CLensFlareElement)
{
    CLensFlareEditor::GetLensFlareEditor()->RegisterLensFlareItemChangeListener(this);
}

LensFlareElementTreeModel::~LensFlareElementTreeModel()
{
    CLensFlareEditor::GetLensFlareEditor()->UnregisterLensFlareItemChangeListener(this);
}

int LensFlareElementTreeModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant LensFlareElementTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return {};
    }

    auto pElement = static_cast<CLensFlareElement*>(index.internalPointer());

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        QString shortName;
        pElement->GetShortName(shortName);
        return shortName;
    }

    case Qt::CheckStateRole:
        return pElement->IsEnable() ? Qt::Checked : Qt::Unchecked;

    case Qt::UserRole:
        return QVariant::fromValue<CLensFlareElement*>(pElement);
    }

    return {};
}

void LensFlareElementTreeModel::EnableElement(CLensFlareElement* pLensFlareElement, bool bEnable)
{
    StoreUndo();
    pLensFlareElement->SetEnable(bEnable);
}

void LensFlareElementTreeModel::RenameElement(CLensFlareElement* pLensFlareElement, const QString& newName)
{
    StoreUndo();
    IOpticsElementBasePtr pOptics = pLensFlareElement->GetOpticsElement();
    if (pOptics)
    {
        pOptics->SetName(newName.toUtf8().data());
        LensFlareUtil::UpdateOpticsName(pOptics);
    }
}

QModelIndex LensFlareElementTreeModel::GetRootItem() const
{
    if (m_pRootElement->GetChildCount() == 0)
    {
        return {};
    }

    auto pRootElement = m_pRootElement->GetChildAt(0);
    assert(pRootElement->GetOpticsType() == eFT_Root);

    return createIndex(0, 0, pRootElement);
}

bool LensFlareElementTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
    {
        return false;
    }

    auto pElement = static_cast<CLensFlareElement*>(index.internalPointer());

    switch (role)
    {
    case Qt::CheckStateRole:
    {
        CUndo undo(tr("Update an enable checkbox for tree ctrl.").toUtf8());
        EnableElement(pElement, value.toInt() == Qt::Checked);
        emit dataChanged(index, index);
        return true;
    }

    case Qt::EditRole:
    {
        QString text = value.toString();

        CUndo undo(tr("Rename library item").toUtf8());

        if (IsExistElement(text))
        {
            QMessageBox::warning(0, tr("Warning"), tr("The identical name exists in a database"));
            return false;
        }
        else if (text.contains(QLatin1Char('.')))
        {
            QMessageBox::warning(0, tr("Warning"), tr("The name must not contain \".\""));
            return false;
        }
        else
        {
            CLensFlareElement* pLensFlareElement = GetLensFlareElement(index);

            QString prevName;
            if (pLensFlareElement && pLensFlareElement->GetName(prevName))
            {
                QString parentName(prevName);
                int offset = parentName.lastIndexOf('.');
                if (offset == -1)
                {
                    parentName = "";
                }
                else
                {
                    parentName.remove(offset + 1, parentName.length() - offset);
                }

                if (index == GetRootItem())
                {
                    CLensFlareEditor* pLensFlareEditor = CLensFlareEditor::GetLensFlareEditor();
                    if (pLensFlareEditor)
                    {
                        CLensFlareItem* pLensFlareItem = pLensFlareEditor->GetSelectedLensFlareItem();
                        if (pLensFlareItem)
                        {
                            QString candidateName = LensFlareUtil::ReplaceLastName(pLensFlareItem->GetName(), text);
                            if (pLensFlareEditor->IsExistTreeItem(candidateName, true))
                            {
                                QMessageBox::warning(0, tr("Warning"), tr("The identical name exists in a database"));
                                return false;
                            }
                            pLensFlareEditor->RenameLensFlareItem(pLensFlareItem, pLensFlareItem->GetGroupName(), text);
                        }
                    }
                    else
                    {
                        QMessageBox::warning(0, tr("Warning"), tr("Renaming is not possible."));
                        return false;
                    }
                }

                RenameElement(pLensFlareElement, parentName + text);
                emit dataChanged(index, index);

                GetIEditor()->GetLensFlareManager()->Modified();
            }

            return true;
        }
    }
    }

    return false;
}

Qt::ItemFlags LensFlareElementTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::ItemFlags();
    }

    auto pElement = index.data(Qt::UserRole).value<CLensFlareElement*>();

    Qt::ItemFlags flags = QAbstractItemModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled;

    if (pElement->GetOpticsType() != eFT_Root)
    {
        flags |= Qt::ItemIsDragEnabled;
    }

    return flags;
}

QModelIndex LensFlareElementTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return {};
    }

    auto parentNode = GetLensFlareElement(parent);
    auto childNode = parentNode->GetChildAt(row);

    if (childNode)
    {
        return createIndex(row, column, childNode);
    }
    else
    {
        return {};
    }
}

QModelIndex LensFlareElementTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return {};
    }

    auto childNode = static_cast<CLensFlareElement*>(index.internalPointer());
    auto parentNode = childNode->GetParent();

    if (parentNode == m_pRootElement.get())
    {
        return {};
    }

    return createIndex(parentNode->GetRow(), 0, parentNode);
}

int LensFlareElementTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
    {
        return 0;
    }
    return GetLensFlareElement(parent)->GetChildCount();
}

bool LensFlareElementTreeModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (!parent.isValid())
    {
        return false;
    }

    auto pParent = static_cast<CLensFlareElement*>(parent.internalPointer());

    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++)
    {
        CLensFlareElement* pChild = pParent->GetChildAt(row);

        IOpticsElementBasePtr pOptics = pChild->GetOpticsElement();
        if (pOptics)
        {
            LensFlareUtil::RemoveOptics(pOptics);
        }

        pParent->RemoveChild(row);
    }

    endRemoveRows();

    GetIEditor()->GetLensFlareManager()->Modified(); // XXX

    if (m_pLensFlareItem)
    {
        m_pLensFlareItem->UpdateLights();
    }

    return true;
}

QStringList LensFlareElementTreeModel::mimeTypes() const
{
    QStringList types;
    types << QStringLiteral("application/x-lumberyard-flareelements");
    types << QStringLiteral("application/x-lumberyard-flaretypes");
    return types;
}

QMimeData* LensFlareElementTreeModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* data = new QMimeData();

    QByteArray array;

    for (auto& index : indexes)
    {
        auto pElement = static_cast<CLensFlareElement*>(index.data(Qt::UserRole).value<CLensFlareElement*>());
        array.append(reinterpret_cast<const char*>(&pElement), sizeof(CLensFlareElement*));
    }

    data->setData(QStringLiteral("application/x-lumberyard-flareelements"), array);

    return data;
}

void LensFlareElementTreeModel::RemoveAllElements()
{
    QModelIndex index = GetRootItem();
    if (!index.isValid())
    {
        return;
    }

    auto pElement = index.data(Qt::UserRole).value<CLensFlareElement*>();

    beginResetModel();

    pElement->RemoveAllChildren();
    pElement->GetOpticsElement()->RemoveAll();

    endResetModel();

    if (m_pLensFlareItem)
    {
        m_pLensFlareItem->UpdateLights();
    }
}

bool LensFlareElementTreeModel::AddElement(const QModelIndex& parentIndex, int row, EFlareType flareType)
{
    if (!parentIndex.isValid())
    {
        return false;
    }

    auto pElement = GetLensFlareElement(parentIndex);

    int nIndex;

    if (LensFlareUtil::IsGroup(pElement->GetOpticsType()))
    {
        if (row != -1)
        {
            nIndex = row;
        }
        else
        {
            nIndex = pElement->GetChildCount();
        }
    }
    else
    {
        CLensFlareElement* pParentElement = GetLensFlareElement(parentIndex.parent());
        if (!pParentElement)
        {
            return false;
        }

        if (!LensFlareUtil::IsGroup(pParentElement->GetOpticsType()))
        {
            return false;
        }

        nIndex = LensFlareUtil::FindOpticsIndexUnderParentOptics(pElement->GetOpticsElement(), pParentElement->GetOpticsElement());
        if (nIndex == -1)
        {
            return false;
        }

        pElement = pParentElement;
    }

    CLensFlareElement::LensFlareElementPtr pNewElement = InsertAtomicElement(nIndex, flareType, pElement);
    if (!pNewElement)
    {
        return false;
    }

    if (m_pLensFlareItem)
    {
        m_pLensFlareItem->UpdateLights();
    }

    return true;
}

CLensFlareElement::LensFlareElementPtr LensFlareElementTreeModel::InsertAtomicElement(int nIndex, EFlareType flareType, CLensFlareElement* pParentElement)
{
    IOpticsElementBasePtr pParent = pParentElement->GetOpticsElement();
    if (pParent == NULL)
    {
        return NULL;
    }

    IOpticsElementBasePtr pNewOptics = gEnv->pOpticsManager->Create(flareType);
    if (pNewOptics == NULL)
    {
        return NULL;
    }

    if (nIndex < 0)
    {
        return NULL;
    }

    CLensFlareElement::LensFlareElementPtr pElement = AddElement(pNewOptics, pParentElement);
    if (pElement == NULL)
    {
        return NULL;
    }

    pParent->InsertElement(nIndex, pNewOptics);

    emit beginInsertRows(createIndex(pParentElement->GetRow(), 0, pParentElement), nIndex, nIndex);
    pParentElement->InsertChild(nIndex, pElement);
    emit endInsertRows();

    return pElement;
}

void LensFlareElementTreeModel::MakeValidElementName(const QString& seedName, QString& outValidName) const
{
    if (!IsExistElement(seedName))
    {
        outValidName = seedName;
        return;
    }

    int counter = 0;
    do
    {
        QString numberString;
        numberString = QString::number(counter);
        QString candidateName = seedName + numberString;
        if (!IsExistElement(candidateName))
        {
            outValidName = candidateName;
            return;
        }
    } while (++counter < 100000); // prevent from infinite loop
}

bool LensFlareElementTreeModel::IsExistElement(const QString& name) const
{
    const auto predicate = [&](CLensFlareElement* pElement)
    {
        QString flareName;
        return pElement->GetName(flareName) && !QString::compare(flareName, name, Qt::CaseInsensitive);
    };
    auto pElement = ::FindLensFlareElement(m_pRootElement.get(), predicate);
    return pElement != nullptr;
}

CLensFlareElement::LensFlareElementPtr LensFlareElementTreeModel::AddElement(IOpticsElementBasePtr pOptics, CLensFlareElement* pParentElement)
{
    if (pOptics == NULL)
    {
        return NULL;
    }

    IOpticsElementBasePtr pParent = pParentElement->GetOpticsElement();

    CLensFlareElement* pLensFlareElement = NULL;

    if (pParent == NULL)
    {
        if (pOptics->GetType() != eFT_Root)
        {
            assert(0 && "CFlareManager::AddItem() - pOptics must be a root optics if the parent doesn't exist.");
            return NULL;
        }
    }
    else
    {
        if (!LensFlareUtil::IsGroup(pParent->GetType()))
        {
            return NULL;
        }

        QString fullElementName;
        if (!pParentElement->GetName(fullElementName))
        {
            return NULL;
        }
        if (!fullElementName.isEmpty())
        {
            fullElementName += ".";
        }
        fullElementName += LensFlareUtil::GetShortName(pOptics->GetName().c_str()).toLower();

        QString validName;
        MakeValidElementName(fullElementName, validName);
        pOptics->SetName(validName.toUtf8().data());
    }

    pLensFlareElement = CreateElement(pOptics);

    if (LensFlareUtil::IsGroup(pOptics->GetType()))
    {
        for (int i = 0, iElementCount(pOptics->GetElementCount()); i < iElementCount; ++i)
        {
            CLensFlareElement::LensFlareElementPtr pChild = AddElement(pOptics->GetElementAt(i), pLensFlareElement);
            pLensFlareElement->AddChild(pChild);
        }
    }

    GetIEditor()->GetLensFlareManager()->Modified();
    if (m_pLensFlareItem)
    {
        m_pLensFlareItem->UpdateLights();
    }

    return pLensFlareElement;
}

bool LensFlareElementTreeModel::MoveElement(CLensFlareElement* pElement, int row, const QModelIndex& index)
{
    if (!index.isValid())
    {
        return false;
    }

    QModelIndex sourceParentIndex = parent(createIndex(0, 0, pElement));
    if (!sourceParentIndex.isValid())
    {
        return false;
    }

    QModelIndex targetParentIndex = index;
    CLensFlareElement* pTargetParent = targetParentIndex.data(Qt::UserRole).value<CLensFlareElement*>();

    QString fullElementName;
    if (!pTargetParent->GetName(fullElementName))
    {
        return false;
    }
    if (!fullElementName.isEmpty())
    {
        fullElementName += ".";
    }
    fullElementName += LensFlareUtil::GetShortName(pElement->GetOpticsElement()->GetName().c_str()).toLower();

    QString validName;
    MakeValidElementName(fullElementName, validName);

    int targetRow;

    if (!LensFlareUtil::IsGroup(pTargetParent->GetOpticsElement()->GetType()))
    {
        targetParentIndex = targetParentIndex.parent();
        pTargetParent = targetParentIndex.data(Qt::UserRole).value<CLensFlareElement*>();
        targetRow = index.row();
    }
    else
    {
        if (row != -1)
        {
            targetRow = row;
        }
        else
        {
            targetRow = pTargetParent->GetChildCount();
        }
    }

    CLensFlareElement* pSourceParent = pElement->GetParent();
    int sourceRow = pElement->GetRow();

    if (!beginMoveRows(sourceParentIndex, sourceRow, sourceRow, targetParentIndex, targetRow))
    {
        return false;
    }

    StoreUndo();

    // insert before removing to increase reference count

    pTargetParent->InsertChild(targetRow, pElement);
    pTargetParent->GetOpticsElement()->InsertElement(targetRow, pElement->GetOpticsElement());

    if (pTargetParent == pSourceParent && targetRow < sourceRow)
    {
        ++sourceRow;
    }

    pSourceParent->RemoveChild(sourceRow);
    pSourceParent->GetOpticsElement()->Remove(sourceRow);

    pElement->GetOpticsElement()->SetName(validName.toUtf8().data());
    LensFlareUtil::UpdateOpticsName(pElement->GetOpticsElement());

    endMoveRows();

    return true;
}

bool LensFlareElementTreeModel::dropMimeData(const QMimeData* data, [[maybe_unused]] Qt::DropAction action, int row, [[maybe_unused]] int column, const QModelIndex& parent)
{
    if (data->hasFormat(QStringLiteral("application/x-lumberyard-flaretypes")))
    {
        // drop from atomic list

        QByteArray encoded = data->data(QStringLiteral("application/x-lumberyard-flaretypes"));
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
            int flareType;
            stream >> flareType;

            CUndo undo(tr("Add Atomic Lens Flare Item").toUtf8());
            AddElement(parent, row, static_cast<EFlareType>(flareType));
        }

        return true;
    }
    else if (data->hasFormat(QStringLiteral("application/x-lumberyard-flareelements")))
    {
        QByteArray array = data->data("application/x-lumberyard-flareelements");

        int count = array.size() / sizeof(CLensFlareElement*);
        auto ppElements = reinterpret_cast<CLensFlareElement**>(array.data());

        CUndo undo(tr("Copy/Cut & Paste library item").toUtf8());

        for (int i = 0; i < count; i++)
        {
            MoveElement(ppElements[i], row, parent);
        }

        return true;
    }
    else
    {
        return false;
    }
}

CLensFlareElement* LensFlareElementTreeModel::FindLensFlareElement(IOpticsElementBasePtr pOptics) const
{
    return ::FindLensFlareElement(m_pRootElement.get(), [&](CLensFlareElement* pElement)
        {
            return pElement->GetOpticsElement() == pOptics;
        });
}

Qt::DropActions LensFlareElementTreeModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions LensFlareElementTreeModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void LensFlareElementTreeModel::UpdateLensFlareItem(CLensFlareItem* pLensFlareItem)
{
    m_pLensFlareItem = pLensFlareItem;

    m_pRootElement.reset(new CLensFlareElement);

    if (pLensFlareItem != NULL)
    {
        auto pChild = UpdateLensFlareElementsRecusively(m_pLensFlareItem->GetOptics());
        if (pChild)
        {
            m_pRootElement->AddChild(pChild);
        }
    }
}

CLensFlareElement* LensFlareElementTreeModel::UpdateLensFlareElementsRecusively(IOpticsElementBasePtr pOptics)
{
    auto pElement = new CLensFlareElement;
    pElement->SetOpticsElement(pOptics);

    for (int i = 0, iCount(pOptics->GetElementCount()); i < iCount; i++)
    {
        auto pChild = UpdateLensFlareElementsRecusively(pOptics->GetElementAt(i));
        if (pElement)
        {
            pElement->AddChild(pChild);
        }
    }

    return pElement;
}

CLensFlareElement* LensFlareElementTreeModel::CreateElement(IOpticsElementBasePtr pOptics)
{
    StoreUndo();
    CLensFlareElement* pElement = new CLensFlareElement;
    pElement->SetOpticsElement(pOptics);
    return pElement;
}

void LensFlareElementTreeModel::OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem)
{
    beginResetModel();
    UpdateLensFlareItem(pLensFlareItem);
    endResetModel();
}

void LensFlareElementTreeModel::OnLensFlareDeleteItem([[maybe_unused]] CLensFlareItem* pLensFlareItem)
{
}

CLensFlareElement* LensFlareElementTreeModel::GetLensFlareElement(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return m_pRootElement.get();
    }
    else
    {
        return static_cast<CLensFlareElement*>(index.internalPointer());
    }
}

void LensFlareElementTreeModel::Paste(const QModelIndex& hItem, XmlNodeRef xmlNode)
{
    if (!hItem.isValid())
    {
        return;
    }

    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return;
    }

    QString type;
    xmlNode->getAttr("Type", type);

    bool bPasteAtSameLevel = false;
    xmlNode->getAttr("PasteAtSameLevel", bPasteAtSameLevel);

    QModelIndex hParentItem;

    IOpticsElementBasePtr pSelectedOptics = GetOpticsElementByTreeItem(hItem);
    if (pSelectedOptics == NULL)
    {
        return;
    }

    if (!LensFlareUtil::IsGroup(pSelectedOptics->GetType()))
    {
        hParentItem = hItem.parent();
    }
    else
    {
        if (bPasteAtSameLevel)
        {
            hParentItem = hItem.parent();
        }
        else
        {
            hParentItem = hItem;
        }
    }

    IOpticsElementBasePtr pParentOptics = GetOpticsElementByTreeItem(hParentItem);
    if (pParentOptics == NULL)
    {
        return;
    }

    for (int i = 0, iChildCount(xmlNode->getChildCount()); i < iChildCount; ++i)
    {
        LensFlareUtil::SClipboardData clipboardData;
        clipboardData.FillThisFromXmlNode(xmlNode->getChild(i));

        if (clipboardData.m_LensFlareFullPath == m_pLensFlareItem->GetFullName() && clipboardData.m_From == LENSFLARE_ITEM_TREE)
        {
            QString msg;
            msg = tr("[%1] lens item can be pasted into the same item").arg(clipboardData.m_LensFlareFullPath);
            CryMessageBox(msg.toUtf8().data(), "Warning", MB_OK);
            continue;
        }

        IOpticsElementBasePtr pSourceOptics = pEditor->FindOptics(clipboardData.m_LensFlareFullPath, clipboardData.m_LensOpticsPath);
        if (pSourceOptics == NULL)
        {
            continue;
        }

        CLensFlareItem* pSourceLensFlareItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(clipboardData.m_LensFlareFullPath);
        if (pSourceLensFlareItem == NULL)
        {
            continue;
        }

        if (type == FLARECLIPBOARDTYPE_CUT)
        {
            if (m_pLensFlareItem->GetFullName() == clipboardData.m_LensFlareFullPath && LensFlareUtil::FindOptics(pSourceOptics, pSelectedOptics->GetName().c_str()))
            {
                QMessageBox::warning(0, tr("Warning"), tr("You can't paste this item here."));
                return;
            }
        }

        // if the copied optics type is root, the type should be converted to group type.
        bool bForceConvertType = false;
        if (pSourceOptics->GetType() == eFT_Root)
        {
            bForceConvertType = true;
        }

        IOpticsElementBasePtr pNewOptics = LensFlareUtil::CreateOptics(pSourceOptics, bForceConvertType);
        if (pNewOptics == NULL)
        {
            return;
        }

        if (type == FLARECLIPBOARDTYPE_CUT)
        {
            if (pSourceLensFlareItem == m_pLensFlareItem)
            {
                QModelIndex hSourceItem = GetTreeItemByOpticsElement(pSourceOptics);
                if (hSourceItem.isValid())
                {
                    removeRow(hSourceItem.row(), hSourceItem.parent());
                }
            }
            else if (clipboardData.m_From == LENSFLARE_ITEM_TREE)
            {
                CLensFlareLibrary* pLibrary = pEditor->GetCurrentLibrary();
                if (pLibrary)
                {
                    QString lensFlareFullName = m_pLensFlareItem ? m_pLensFlareItem->GetFullName() : "";
                    pLibrary->RemoveItem(pSourceLensFlareItem);
                    pEditor->ReloadItems();
                    pSourceLensFlareItem->UpdateLights();
                    pEditor->UpdateLensOpticsNames(pSourceLensFlareItem->GetFullName(), "");
                    if (!lensFlareFullName.isEmpty())
                    {
                        pEditor->SelectItemByName(lensFlareFullName);
                    }
                }

                LensFlareUtil::RemoveOptics(pSourceOptics);
            }
        }

        CLensFlareElement* pParentElement = FindLensFlareElement(pParentOptics);

        // The children optics items were already added in a creation phase so we don't need to update the optics object.
        CLensFlareElement::LensFlareElementPtr pNewElement = AddElement(pNewOptics, pParentElement);
        assert(pNewElement);
        if (!pNewElement)
        {
            return;
        }

        int nInsertedPos;
        if (hParentItem == hItem)
        {
            nInsertedPos = 0;
        }
        else
        {
            nInsertedPos = LensFlareUtil::FindOpticsIndexUnderParentOptics(pSelectedOptics, pParentOptics);
            if (nInsertedPos > pParentOptics->GetElementCount() || nInsertedPos == -1)
            {
                nInsertedPos = pParentOptics->GetElementCount();
            }
        }

        pParentOptics->InsertElement(nInsertedPos, pNewOptics);
        LensFlareUtil::UpdateOpticsName(pNewOptics);

        emit beginInsertRows(createIndex(pParentElement->GetRow(), 0, pParentElement), nInsertedPos, nInsertedPos);
        pParentElement->InsertChild(nInsertedPos, pNewElement);
        emit endInsertRows();

        // pSelectedLensFlareItem->UpdateLights();
    }
}

IOpticsElementBasePtr LensFlareElementTreeModel::GetOpticsElementByTreeItem(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }
    return GetLensFlareElement(index)->GetOpticsElement();
}

QModelIndex LensFlareElementTreeModel::GetTreeItemByName(const QString& name) const
{
    auto pElement = ::FindLensFlareElement(m_pRootElement.get(), [&](CLensFlareElement* pElement)
            {
                QString flareName;
                return pElement->GetName(flareName) && flareName == name;
            });

    if (!pElement)
    {
        return {};
    }

    return createIndex(pElement->GetRow(), 0, pElement);
}

QModelIndex LensFlareElementTreeModel::GetTreeItemByOpticsElement(const IOpticsElementBasePtr pOptics) const
{
    auto pElement = FindLensFlareElement(pOptics);
    if (!pElement)
    {
        return {};
    }
    return createIndex(pElement->GetRow(), 0, pElement);
}

void LensFlareElementTreeModel::InvalidateLensFlareItem()
{
    OnLensFlareChangeItem(nullptr);
}

void LensFlareElementTreeModel::StoreUndo(const QString& undoDescription)
{
    if (CUndo::IsRecording())
    {
        if (undoDescription.isEmpty())
        {
            CUndo::Record(new CUndoLensFlareItem(GetLensFlareItem()));
        }
        else
        {
            CUndo::Record(new CUndoLensFlareItem(GetLensFlareItem(), undoDescription));
        }
    }
}

#include <LensFlareEditor/moc_LensFlareElementTree.cpp>
