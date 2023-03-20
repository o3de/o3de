/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "Undo/IUndoObject.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include "Objects/BaseObject.h"

//////////////////////////////////////////////////////////////////////////
//! Undo New Object
class CUndoBaseObjectNew
    : public IUndoObject
{
public:
    CUndoBaseObjectNew(CBaseObject* object);

protected:
    int GetSize() override { return sizeof(*this); } // Return size of xml state.
    QString GetObjectName() override { return m_object->GetName(); }

    void Undo(bool bUndo) override;
    void Redo() override;

private:
    CBaseObjectPtr m_object;
    TGUIDRemap m_remapping;
    XmlNodeRef m_redo;
};


//////////////////////////////////////////////////////////////////////////
//! Undo Delete Object
class CUndoBaseObjectDelete
    : public IUndoObject
{
public:
    CUndoBaseObjectDelete(CBaseObject* object);

protected:
    int GetSize() override { return sizeof(*this); } // Return size of xml state.
    QString GetObjectName() override { return m_object->GetName(); }

    void Undo(bool bUndo) override;
    void Redo() override;

private:
    CBaseObjectPtr m_object;
    XmlNodeRef m_undo;
    TGUIDRemap m_remapping;
    bool m_bSelected;
};

//////////////////////////////////////////////////////////////////////////
//! Undo Select Object
class CUndoBaseObjectSelect
    : public IUndoObject
{
public:
    CUndoBaseObjectSelect(CBaseObject* object);

    /**
    * This Undo command can be used for either Legacy or Component Entities, though for
    * performance reasons Component Entities are typically undone using CUndoBaseObjectBulkSelect
    *
    * @param object The object to perform the undo/redo operation on.
    * @param isSelect This is true if you are trying to undo a select operation, and false if
    *        trying to undo a deselect operation
    */
    CUndoBaseObjectSelect(CBaseObject* object, bool isSelect);

protected:
    void Release() override { delete this; }
    int GetSize() override { return sizeof(*this); } // Return size of xml state.
    QString GetObjectName() override; 

    void Undo(bool bUndo) override;
    void Redo() override;

private:
    GUID m_guid;
    bool m_bUndoSelect;
    bool m_bRedoSelect;
};

//////////////////////////////////////////////////////////////////////////
//! Undo Select for many Objects
class CUndoBaseObjectBulkSelect
    : public IUndoObject
{
public:
    /**
    * This Undo/Redo command is designed to improve performance of the standard CUndoBaseObjectSelect
    * command by passing all Select/Deselect commands through the proper Ebuses in one bulk operation instead
    * of individually.
    *
    * This only works with Component Entities. Legacy objects must still use the standard CUndoBaseObjectSelect.
    *
    * @param previousSelection the set of objects already selected. This is useful to ensure proper Undo/Redo
    *        when a user makes a second rectangular selection by holding ctrl
    * @param selectionGroup The items that will have their selection restored by either an Undo
    *        or Redo step
    */
    CUndoBaseObjectBulkSelect(const AZStd::unordered_set<const CBaseObject*>& previousSelection, const CSelectionGroup& selectionGroup);

protected:
    int GetSize() override { return sizeof(*this); } // Return size of xml state.

    /*
    * Deselects the objects
    */
    void Undo(bool bUndo) override;

    /*
    * Selects the objects
    */
    void Redo() override;

private:
    // The list of Entity Ids involved in the selection change
    AzToolsFramework::EntityIdList m_entityIdList;
};


//////////////////////////////////////////////////////////////////////////
//! Undo Clear Selection
class CUndoBaseObjectClearSelection
    : public IUndoObject
{
public:
    /**
    * This Undo/Redo command is designed to improve performance of the standard CUndoBaseObjectSelect
    * command by passing all Select/Deselect commands through the proper Ebuses.
    *
    * This only works with Component Entities. Legacy objects must still use the standard CUndoBaseObjectSelect.
    *
    * @param selectionGroup The items that will have their selection restored by either an Undo
    *        or Redo step
    */
    CUndoBaseObjectClearSelection(const CSelectionGroup& selectionGroup);

protected:
    int GetSize() override { return sizeof(*this); } // Return size of xml state.

    void Undo(bool bUndo) override;

    void Redo() override;

private:
    // The list of Entity Ids involved in the selection change
    AzToolsFramework::EntityIdList m_entityIdList;
};
