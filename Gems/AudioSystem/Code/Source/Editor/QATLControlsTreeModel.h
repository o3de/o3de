/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ATLControlsModel.h>
#include <QStandardItemModel>

class QStandardItem;

namespace AudioControls
{
    class CATLControl;

    //-------------------------------------------------------------------------------------------//
    class QATLTreeModel
        : public QStandardItemModel
        , public IATLControlModelListener
    {
    public:
        QATLTreeModel();
        ~QATLTreeModel() override;
        void Initialize(CATLControlsModel* pControlsModel);
        QStandardItem* GetItemFromControlID(CID nID);

        CATLControl* CreateControl(EACEControlType eControlType, const AZStd::string_view sName, CATLControl* pParent = nullptr);
        QStandardItem* AddControl(CATLControl* pControl, QStandardItem* pParent, int nRow = 0);

        QStandardItem* CreateFolder(QStandardItem* pParent, const AZStd::string_view sName, int nRow = 0);

        void RemoveItem(QModelIndex index);
        void RemoveItems(QModelIndexList indexList);

        void SetItemAsDirty(QStandardItem* pItem);

    private:
        void DeleteInternalData(QModelIndex root);
        CATLControl* GetControlFromIndex(QModelIndex index);

        // ------------- QStandardItemModel ----------------------
        bool canDropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
        bool dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
        // -------------------------------------------------------

        // ------------- IATLControlModelListener ----------------
        void OnControlAdded([[maybe_unused]] CATLControl* pControl) override {}
        void OnControlModified(CATLControl* pControl) override;
        void OnControlRemoved([[maybe_unused]] CATLControl* pControl) override {}
        // -------------------------------------------------------

        CATLControlsModel* m_pControlsModel;
    };
} // namespace AudioControls
