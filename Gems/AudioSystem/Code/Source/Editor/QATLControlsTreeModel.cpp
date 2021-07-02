/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <QATLControlsTreeModel.h>

#include <ACEEnums.h>
#include <AudioControl.h>
#include <AudioControlsEditorUndo.h>
#include <IEditor.h>
#include <QAudioControlTreeWidget.h>

#include <QStandardItem>
#include <QMessageBox>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    QATLTreeModel::QATLTreeModel()
        : m_pControlsModel(nullptr)
    {
    }

    //-------------------------------------------------------------------------------------------//
    QATLTreeModel::~QATLTreeModel()
    {
        if (m_pControlsModel)
        {
            m_pControlsModel->RemoveListener(this);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QATLTreeModel::Initialize(CATLControlsModel* pControlsModel)
    {
        m_pControlsModel = pControlsModel;
        m_pControlsModel->AddListener(this);
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* QATLTreeModel::GetItemFromControlID(CID nID)
    {
        QModelIndexList indexes = match(index(0, 0, QModelIndex()), eDR_ID, nID, 1, Qt::MatchRecursive);
        if (!indexes.empty())
        {
            return itemFromIndex(indexes.at(0));
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* QATLTreeModel::AddControl(CATLControl* pControl, QStandardItem* pParent, int nRow)
    {
        if (pControl && pParent)
        {
            QStandardItem* pItem = new QAudioControlItem(QString(pControl->GetName().c_str()), pControl);
            if (pItem)
            {
                pParent->insertRow(nRow, pItem);
                SetItemAsDirty(pItem);
            }
            return pItem;
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* QATLTreeModel::CreateFolder(QStandardItem* pParent, const AZStd::string_view sName, int nRow)
    {
        if (pParent)
        {
            // Make a valid name for the folder (avoid having folders with the same name under same root)
            QString sRootName(sName.data());
            QString sFolderName(sRootName);
            int number = 1;
            bool bFoundName = false;
            while (!bFoundName)
            {
                bFoundName = true;
                const int size = pParent->rowCount();
                for (int i = 0; i < size; ++i)
                {
                    QStandardItem* pItem = pParent->child(i);
                    if (pItem && (pItem->data(eDR_TYPE) == eIT_FOLDER) && (QString::compare(sFolderName, pItem->text(), Qt::CaseInsensitive) == 0))
                    {
                        bFoundName = false;
                        sFolderName = sRootName + "_" + QString::number(number);
                        ++number;
                        break;
                    }
                }
            }

            if (!sFolderName.isEmpty())
            {
                if (QStandardItem* pFolderItem = new QFolderItem(sFolderName))
                {
                    SetItemAsDirty(pFolderItem);
                    pParent->insertRow(nRow, pFolderItem);
                    if (!CUndo::IsSuspended())
                    {
                        CUndo undo("Audio Folder Created");
                        CUndo::Record(new CUndoFolderAdd(pFolderItem));
                    }
                    return pFolderItem;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    void QATLTreeModel::RemoveItem(QModelIndex index)
    {
        QModelIndexList indexList;
        indexList.push_back(index);
        RemoveItems(indexList);
    }

    //-------------------------------------------------------------------------------------------//
    void QATLTreeModel::RemoveItems(QModelIndexList indexList)
    {
        // Sort the controls by the level they are inside the tree
        // (the deepest in the tree first) and then by their row number.
        // This way we guarantee we don't delete the parent of a
        // control before its children
        struct STreeIndex
        {
            QPersistentModelIndex m_index;
            int m_level;

            STreeIndex(QPersistentModelIndex index, int level)
                : m_index(index)
                , m_level(level) {}

            bool operator< (const STreeIndex& index) const
            {
                if (m_level == index.m_level)
                {
                    return m_index.row() > index.m_index.row();
                }
                return m_level > index.m_level;
            }
        };

        AZStd::vector<STreeIndex> sortedIndexList;

        const int size = indexList.length();
        for (int i = 0; i < size; ++i)
        {
            int level = 0;
            QModelIndex index = indexList[i];
            while (index.isValid())
            {
                ++level;
                index = index.parent();
            }
            sortedIndexList.push_back(STreeIndex(indexList[i], level));
        }
        std::sort(sortedIndexList.begin(), sortedIndexList.end());

        for (int i = 0; i < size; ++i)
        {
            QModelIndex index = sortedIndexList[i].m_index;
            if (index.isValid())
            {
                DeleteInternalData(index);

                // Mark parent as modified
                QModelIndex parent = index.parent();
                if (parent.isValid())
                {
                    SetItemAsDirty(itemFromIndex(parent));
                }
                removeRow(index.row(), index.parent());
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QATLTreeModel::DeleteInternalData(QModelIndex root)
    {
        // Delete children first and in reverse order
        // of their row (so that we can undo them in the same order)
        AZStd::vector<QModelIndex> childs;
        QModelIndex child = index(0, 0, root);
        for (int i = 1; child.isValid(); ++i)
        {
            childs.push_back(child);
            child = index(i, 0, root);
        }

        const size_t size = childs.size();
        for (size_t i = 0; i < size; ++i)
        {
            DeleteInternalData(childs[(size - 1) - i]);
        }

        if (root.data(eDR_TYPE) == eIT_AUDIO_CONTROL)
        {
            m_pControlsModel->RemoveControl(root.data(eDR_ID).toUInt());
        }
        else
        {
            if (!CUndo::IsSuspended())
            {
                CUndo::Record(new CUndoFolderRemove(itemFromIndex(root)));
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* QATLTreeModel::GetControlFromIndex(QModelIndex index)
    {
        if (m_pControlsModel && index.isValid() && (index.data(eDR_TYPE) == eIT_AUDIO_CONTROL))
        {
            return m_pControlsModel->GetControlByID(index.data(eDR_ID).toUInt());
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    void QATLTreeModel::OnControlModified(CATLControl* pControl)
    {
        if (pControl)
        {
            if (QStandardItem* pItem = GetItemFromControlID(pControl->GetId()))
            {
                QString sNewName(pControl->GetName().c_str());
                if (pItem->text() != sNewName)
                {
                    pItem->setText(sNewName);
                }
                SetItemAsDirty(pItem);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QATLTreeModel::SetItemAsDirty(QStandardItem* pItem)
    {
        if (pItem)
        {
            blockSignals(true);
            pItem->setData(true, eDR_MODIFIED);
            blockSignals(false);

            SetItemAsDirty(pItem->parent());
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* QATLTreeModel::CreateControl(EACEControlType eControlType, const AZStd::string_view sName, CATLControl* pParent)
    {
        AZStd::string sFinalName = m_pControlsModel->GenerateUniqueName(sName, eControlType, pParent ? pParent->GetScope() : "", pParent);
        return m_pControlsModel->CreateControl(sFinalName, eControlType, pParent);
    }

    //-------------------------------------------------------------------------------------------//
    bool QATLTreeModel::dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int column, const QModelIndex& parent)
    {
        // LY-17684
        QStandardItem* rootItem = invisibleRootItem();
        QStandardItem* targetItem = rootItem;
        if (parent.isValid())
        {
            targetItem = itemFromIndex(parent);
        }

        if (targetItem)
        {
            if (targetItem && (targetItem->data(eDR_TYPE) == eIT_FOLDER || targetItem == rootItem))
            {
                const QString format = "application/x-qabstractitemmodeldatalist";
                if (mimeData->hasFormat(format))
                {
                    QByteArray encoded = mimeData->data(format);
                    QDataStream stream(&encoded, QIODevice::ReadOnly);
                    while (!stream.atEnd())
                    {
                        int streamRow, streamCol;
                        QMap<int, QVariant> roleDataMap;
                        stream >> streamRow >> streamCol >> roleDataMap;
                        if (!roleDataMap.isEmpty())
                        {
                            // If dropping a folder, make sure that folder name doesn't already exist where it is being dropped
                            if (roleDataMap[eDR_TYPE] == eIT_FOLDER)
                            {
                                // Make sure the target folder doesn't have a folder with the same name
                                QString droppedFolderName = roleDataMap[Qt::DisplayRole].toString();
                                const int size = targetItem->rowCount();
                                for (int i = 0; i < size; ++i)
                                {
                                    QStandardItem* pItem = targetItem->child(i);
                                    if (pItem && (pItem->data(eDR_TYPE) == eIT_FOLDER) && (QString::compare(droppedFolderName, pItem->text(), Qt::CaseInsensitive) == 0))
                                    {
                                        QMessageBox messageBox;
                                        messageBox.setStandardButtons(QMessageBox::Ok);
                                        messageBox.setWindowTitle("Audio Controls Editor");
                                        messageBox.setText("This destination already contains a folder named '" + droppedFolderName + "'.");
                                        messageBox.exec();
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (mimeData && action == Qt::MoveAction)
        {
            if (!CUndo::IsSuspended())
            {
                CUndo undo("Audio Control Moved");
                CUndo::Record(new CUndoItemMove());
            }
        }

        return QStandardItemModel::dropMimeData(mimeData, action, row, column, parent);
    }

    //-------------------------------------------------------------------------------------------//
    bool QATLTreeModel::canDropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
    {
        if (!parent.isValid())
        {
            // Prevent moving controls to the root (outside a folder)
            const QString format = "application/x-qabstractitemmodeldatalist";
            if (mimeData->hasFormat(format))
            {
                QByteArray data = mimeData->data(format);
                QDataStream stream(&data, QIODevice::ReadOnly);
                int streamRow, streamCol;
                QMap<int, QVariant> roleDataMap;
                stream >> streamRow >> streamCol >> roleDataMap;
                if (!roleDataMap.isEmpty() && roleDataMap[eDR_TYPE] != eIT_FOLDER)
                {
                    return false;
                }
            }
        }
        else if (parent.data(eDR_TYPE) == eIT_AUDIO_CONTROL)
        {
            // Prevent dropping on switches
            CID nID = parent.data(eDR_ID).toUInt();
            if (CATLControl* pControl = m_pControlsModel->GetControlByID(nID))
            {
                EACEControlType eType = pControl->GetType();
                if (eType == eACET_SWITCH || eType == eACET_SWITCH_STATE)
                {
                    return false;
                }
            }
        }

        return QStandardItemModel::canDropMimeData(mimeData, action, row, column, parent);
    }

} // namespace AudioControls
