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
