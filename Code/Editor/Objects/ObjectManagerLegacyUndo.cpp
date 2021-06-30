/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ObjectManagerLegacyUndo.h"

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

// Editor
#include "Include/IObjectManager.h"
#include "Objects/BaseObject.h"
#include "Objects/ObjectLoader.h"
#include "Objects/SelectionGroup.h"


//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectNew implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectNew::CUndoBaseObjectNew(CBaseObject* object)
{
    m_object = object;
}

void CUndoBaseObjectNew::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo = XmlHelpers::CreateXmlNode("Redo");
        // Save current object state.
        CObjectArchive ar(GetIEditor()->GetObjectManager(), m_redo, false);
        ar.bUndo = true;
        m_object->Serialize(ar);
    }

    // Delete this object.
    GetIEditor()->DeleteObject(m_object);
}

void CUndoBaseObjectNew::Redo()
{
    if (!m_redo)
    {
        return;
    }

    IObjectManager* objectManager = GetIEditor()->GetObjectManager();
    {
        CObjectArchive ar(objectManager, m_redo, true);
        ar.bUndo = true;
        ar.MakeNewIds(false);
        ar.LoadObject(m_redo, m_object);
    }

    objectManager->ClearSelection();
    objectManager->SelectObject(m_object);
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectDelete implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectDelete::CUndoBaseObjectDelete(CBaseObject* object)
{
    AZ_Assert(object, "Object does not exist");
    object->SetTransformDelegate(nullptr);
    m_object = object;

    // Save current object state.
    m_undo = XmlHelpers::CreateXmlNode("Undo");
    CObjectArchive ar(GetIEditor()->GetObjectManager(), m_undo, false);
    ar.bUndo = true;
    m_bSelected = m_object->IsSelected();
    m_object->Serialize(ar);
}

void CUndoBaseObjectDelete::Undo([[maybe_unused]] bool bUndo)
{
    IObjectManager* objectManager = GetIEditor()->GetObjectManager();
    {
        CObjectArchive ar(objectManager, m_undo, true);
        ar.bUndo = true;
        ar.MakeNewIds(false);
        ar.LoadObject(m_undo, m_object);
        m_object->ClearFlags(OBJFLAG_SELECTED);
    }

    if (m_bSelected)
    {
        objectManager->ClearSelection();
        objectManager->SelectObject(m_object);
    }
}

void CUndoBaseObjectDelete::Redo()
{
    // Delete this object.
    GetIEditor()->DeleteObject(m_object);
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectSelect implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectSelect::CUndoBaseObjectSelect(CBaseObject* object)
{
    AZ_Assert(object, "Object does not exist");
    m_guid = object->GetId();
    m_bUndoSelect = object->IsSelected();
}

CUndoBaseObjectSelect::CUndoBaseObjectSelect(CBaseObject* object, bool isSelect)
{
    AZ_Assert(object, "Object does not exist");
    m_guid = object->GetId();
    m_bUndoSelect = !isSelect;
}

QString CUndoBaseObjectSelect::GetObjectName()
{
    CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!object)
    {
        return "";
    }

    return object->GetName();
}

void CUndoBaseObjectSelect::Undo(bool bUndo)
{
    CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!object)
    {
        return;
    }

    if (bUndo)
    {
        m_bRedoSelect = object->IsSelected();
    }

    if (m_bUndoSelect)
    {
        GetIEditor()->GetObjectManager()->SelectObject(object);
    }
    else
    {
        GetIEditor()->GetObjectManager()->UnselectObject(object);
    }
}

void CUndoBaseObjectSelect::Redo()
{
    if (CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid))
    {
        if (m_bRedoSelect)
        {
            GetIEditor()->GetObjectManager()->SelectObject(object);
        }
        else
        {
            GetIEditor()->GetObjectManager()->UnselectObject(object);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectBulkSelect implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectBulkSelect::CUndoBaseObjectBulkSelect(const AZStd::unordered_set<const CBaseObject*>& previousSelection, const CSelectionGroup& selectionGroup)
{
    int numSelectedObjects = selectionGroup.GetCount();
    m_entityIdList.reserve(numSelectedObjects);

    // Populate list of entities to restore selection to 
    for (int objectIndex = 0; objectIndex < numSelectedObjects; ++objectIndex)
    {
        CBaseObject* object = selectionGroup.GetObject(objectIndex);

        // Don't track Undo/Redo for Legacy Objects or entities that were not selected in this step
        if (object->GetType() != OBJTYPE_AZENTITY || previousSelection.find(object) != previousSelection.end())
        {
            continue;
        }

        AZ::EntityId id;
        AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(
            id, 
            object, 
            &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

        m_entityIdList.push_back(id);
    }
}

void CUndoBaseObjectBulkSelect::Undo(bool bUndo)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    if (!bUndo)
    {
        return;
    }

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::MarkEntitiesDeselected,
        m_entityIdList);
}

void CUndoBaseObjectBulkSelect::Redo()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::MarkEntitiesSelected,
        m_entityIdList);
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectClearSelection implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectClearSelection::CUndoBaseObjectClearSelection(const CSelectionGroup& selectionGroup)
{
    int numSelectedObjects = selectionGroup.GetCount();
    m_entityIdList.reserve(numSelectedObjects);

    // Populate list of entities to restore selection to 
    for (int objectIndex = 0; objectIndex < numSelectedObjects; ++objectIndex)
    {
        CBaseObject* object = selectionGroup.GetObject(objectIndex);

        // Don't track Undo/Redo for Legacy Objects
        if (object->GetType() != OBJTYPE_AZENTITY)
        {
            continue;
        }

        AZ::EntityId id;
        AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(
            id, 
            object, 
            &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

        m_entityIdList.push_back(id);
    }
}

void CUndoBaseObjectClearSelection::Undo(bool bUndo)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    if (!bUndo)
    {
        return;
    }

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
        m_entityIdList);
}

void CUndoBaseObjectClearSelection::Redo() 
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
        AzToolsFramework::EntityIdList());
}
