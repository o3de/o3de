/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AudioControl.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <QATLControlsTreeModel.h>
#include <Undo/IUndoObject.h>

#include <QAbstractItemModel>
#include <QString>

class QStandardItem;

namespace AudioControls
{
    typedef AZStd::vector<int> TPath;

    //-------------------------------------------------------------------------------------------//
    class IUndoControlOperation
        : public IUndoObject
    {
    protected:
        IUndoControlOperation() {}
        void AddStoredControl();
        void RemoveStoredControl();

        TPath m_path;
        CID m_id;
        AZStd::shared_ptr<CATLControl> m_pStoredControl;
    };

    //-------------------------------------------------------------------------------------------//
    class CUndoControlAdd
        : public IUndoControlOperation
    {
    public:
        explicit CUndoControlAdd(CID id);
    protected:
        int GetSize() override { return sizeof(*this); }

        void Undo(bool bUndo) override;
        void Redo() override;
    };

    //-------------------------------------------------------------------------------------------//
    class CUndoControlRemove
        : public IUndoControlOperation
    {
    public:
        explicit CUndoControlRemove(AZStd::shared_ptr<CATLControl>& pControl);
    protected:
        int GetSize() override { return sizeof(*this); }

        void Undo(bool bUndo) override;
        void Redo() override;
    };

    //-------------------------------------------------------------------------------------------//
    class IUndoFolderOperation
        : public IUndoObject
    {
    protected:
        explicit IUndoFolderOperation(QStandardItem* pItem);
        void AddFolderItem();
        void RemoveItem();

        TPath m_path;
        QString m_sName;
    };

    //-------------------------------------------------------------------------------------------//
    class CUndoFolderRemove
        : public IUndoFolderOperation
    {
    public:
        explicit CUndoFolderRemove(QStandardItem* pItem);
    protected:
        int GetSize() override { return sizeof(*this); }

        void Undo(bool bUndo) override;
        void Redo() override;
    };

    //-------------------------------------------------------------------------------------------//
    class CUndoFolderAdd
        : public IUndoFolderOperation
    {
    public:
        explicit CUndoFolderAdd(QStandardItem* pItem);
    protected:
        int GetSize() override { return sizeof(*this); }

        void Undo(bool bUndo) override;
        void Redo() override;
    };

    //-------------------------------------------------------------------------------------------//
    class CUndoControlModified
        : public IUndoObject
    {
    public:
        explicit CUndoControlModified(CID id);
    protected:
        int GetSize() override { return sizeof(*this); }

        void SwapData();
        void Undo(bool bUndo) override;
        void Redo() override;

        CID m_id;
        AZStd::string m_name;
        AZStd::string m_scope;
        bool m_isAutoLoad;
        AZStd::vector<TConnectionPtr> m_connectedControls;
    };

    //-------------------------------------------------------------------------------------------//
    class CUndoItemMove
        : public IUndoObject
    {
    public:
        CUndoItemMove();

    protected:
        int GetSize() override { return sizeof(*this); }

        void Undo(bool bUndo) override;
        void Redo() override;
        void Copy(QStandardItem* pSource, QStandardItem* pDest);

        QATLTreeModel m_original;
        QATLTreeModel m_modified;

        bool bModifiedInitialised;
    };
} //namespace AudioControls
